#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <wayland-client.h>
#include <wayland-client-protocol.h>
#include <wayland-egl.h>
#include <wayland-util.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "xdg-output-unstable-v1-client-protocol.h"
#include "loaders/loaders.h"
#include "mvmath/mvmath.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define CLIENT_TITLE "background"

struct client {
	struct wl_display *display;
	struct wl_registry *registry;
	struct wl_compositor *compositor;
	struct zwlr_layer_shell_v1 *layer_shell;
	struct zxdg_output_manager_v1 *xdg_output_manager;
	char *wallpaper_path;
	unsigned char *wallpaper_buffer;
	int wallpaper_width;
	int wallpaper_height;
	int wallpaper_channels;
	bool running;
	struct wl_list backgrounds;
};

struct background {
	struct client *client;
	struct wl_output *output;
	struct zxdg_output_v1 *xdg_output;
	struct wl_surface *surface;
	struct zwlr_layer_surface_v1 *layer_surface;
	struct wl_egl_window *egl_window;
	EGLDisplay egl_display;
	EGLConfig egl_config;
	EGLSurface egl_surface;
	EGLContext egl_context;
	unsigned int programs[1];
	unsigned int textures[1];
	unsigned int buffers[3];
	unsigned int vertex_arrays[1];
	int id;
	mat4 mvp;
	int width;
	int height;
	int scale;
	char *name;
	bool running;
	struct wl_list link;
};

void init_egl(struct background *background)
{
	EGLint major;
	EGLint minor;
	EGLint count;
	EGLint n;
	EGLint size;
	EGLConfig *configs;
	EGLint config_attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
		EGL_NONE
	};

	EGLint context_attribs[] = {
		EGL_CONTEXT_MAJOR_VERSION, 3,
		EGL_NONE
	};

	background->egl_display = eglGetDisplay(
		(EGLNativeDisplayType)background->client->display
	);
	if (background->egl_display == EGL_NO_DISPLAY) {
		fprintf(stderr, "Cannot get egl display.\n");
		exit(EXIT_FAILURE);
	}
	if (!eglInitialize(background->egl_display, &major, &minor)) {
		fprintf(stderr, "Cannot initialize egl.\n");
		exit(EXIT_FAILURE);
	}
	eglBindAPI(EGL_OPENGL_ES_API);
#ifdef __DEBUG__
	printf("EGL version: major: %d, minor: %d.\n", major, minor);
#endif

	eglGetConfigs(background->egl_display, NULL, 0, &count);
	configs = calloc(count, sizeof(*configs));
	eglChooseConfig(
		background->egl_display,
		config_attribs,
		configs,
		count,
		&n
	);
	for (int i = 0; i < n; ++i) {
#ifdef __DEBUG__
		eglGetConfigAttrib(
			background->egl_display,
			configs[i],
			EGL_RED_SIZE,
			&size
		);
		printf("Config %d: EGL_RED_SIZE: %d.\n", i, size);
#endif
#ifdef __DEBUG__
		eglGetConfigAttrib(
			background->egl_display,
			configs[i],
			EGL_GREEN_SIZE,
			&size
		);
		printf("Config %d: EGL_GREEN_SIZE: %d.\n", i, size);
#endif
#ifdef __DEBUG__
		eglGetConfigAttrib(
			background->egl_display,
			configs[i],
			EGL_BLUE_SIZE,
			&size
		);
		printf("Config %d: EGL_BLUE_SIZE: %d.\n", i, size);
#endif
		eglGetConfigAttrib(
			background->egl_display,
			configs[i],
			EGL_BUFFER_SIZE,
			&size
		);
		if (size == 3 * 8) {
			background->egl_config = configs[i];
			break;
		}
	}
	free(configs);

	background->egl_context = eglCreateContext(
		background->egl_display,
		background->egl_config,
		EGL_NO_CONTEXT,
		context_attribs
	);
	if (background->egl_context == EGL_NO_CONTEXT) {
		fprintf(stderr, "Cannot create egl context.\n");
		exit(EXIT_FAILURE);
	}

	background->egl_window = wl_egl_window_create(
		background->surface,
		background->width,
		background->height
	);
	if (background->egl_window == EGL_NO_SURFACE) {
		fprintf(stderr, "Cannot create egl window.\n");
		exit(EXIT_FAILURE);
	}

	background->egl_surface = eglCreateWindowSurface(
		background->egl_display,
		background->egl_config,
		background->egl_window,
		NULL
	);
	if (!eglMakeCurrent(
		background->egl_display,
		background->egl_surface,
		background->egl_surface,
		background->egl_context
	)) {
		fprintf(stderr, "Cannot make current.\n");
		exit(EXIT_FAILURE);
	}
	eglSwapInterval(background->egl_display, 0);
}

void init_gl(struct background *background)
{
	background->programs[0] = load_program(
		"../shaders/background.v.glsl",
		"../shaders/background.f.glsl"
	);

	glGenBuffers(3, background->buffers);

	const float vertices[] = {
		// x, y
		// bottom left
		-1.0f, -1.0f,
		// top left
		-1.0f, 1.0f,
		// bottom right
		1.0f, -1.0f,
		// top right
		1.0f, 1.0f
	};
	glBindBuffer(GL_ARRAY_BUFFER, background->buffers[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	const float coordinates[] = {
		// u, v
		// bottom left
		0.0f, 1.0f,
		// top left
		0.0f, 0.0f,
		// bottom right
		1.0f, 1.0f,
		// top right
		1.0f, 0.0f
	};
	glBindBuffer(GL_ARRAY_BUFFER, background->buffers[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(coordinates), coordinates, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	const unsigned int indices[] = {
		0, 1, 2,
		3, 2, 1
	};
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, background->buffers[2]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glGenVertexArrays(1, background->vertex_arrays);
	
	glBindVertexArray(background->vertex_arrays[0]);

	glBindBuffer(GL_ARRAY_BUFFER, background->buffers[0]);
	glVertexAttribPointer(
		glGetAttribLocation(background->programs[0], "in_position"),
		2,
		GL_FLOAT,
		GL_FALSE,
		2 * sizeof(*vertices),
		0
	);
	glEnableVertexAttribArray(
		glGetAttribLocation(background->programs[0], "in_position")
	);

	glBindBuffer(GL_ARRAY_BUFFER, background->buffers[1]);
	glVertexAttribPointer(
		glGetAttribLocation(background->programs[0], "in_coordinate"),
		2,
		GL_FLOAT,
		GL_FALSE,
		2 * sizeof(*coordinates),
		0
	);
	glEnableVertexAttribArray(
		glGetAttribLocation(background->programs[0], "in_coordinate")
	);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, background->buffers[2]);

	glBindVertexArray(0);

	// 2D program does not need those.
	glDisable(GL_DEPTH_TEST);
	glDepthMask(false);
	glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);

	glEnable(GL_TEXTURE_2D);
	
	glGenTextures(1, background->textures);
	glBindTexture(GL_TEXTURE_2D, background->textures[0]);
	glTexParameteri(
		GL_TEXTURE_2D,
		GL_TEXTURE_WRAP_S,
		GL_MIRRORED_REPEAT
	);
	glTexParameteri(
		GL_TEXTURE_2D,
		GL_TEXTURE_WRAP_T,
		GL_MIRRORED_REPEAT
	);
	glTexParameteri(
		GL_TEXTURE_2D,
		GL_TEXTURE_MIN_FILTER,
		GL_LINEAR_MIPMAP_LINEAR
	);
	glTexParameteri(
		GL_TEXTURE_2D,
		GL_TEXTURE_MAG_FILTER,
		GL_LINEAR
	);
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGB,
		background->client->wallpaper_width,
		background->client->wallpaper_height,
		0,
		GL_RGB,
		GL_UNSIGNED_BYTE,
		background->client->wallpaper_buffer
	);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	glViewport(0, 0, background->width, background->height);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

void finish_gl(struct background *background)
{
	glDeleteBuffers(3, background->buffers);
	glDeleteVertexArrays(1, background->vertex_arrays);
	free_program(background->programs[0]);
}

void finish_egl(struct background *background)
{
	eglDestroyContext(background->egl_display, background->egl_context);
	eglDestroySurface(background->egl_display, background->egl_surface);
	wl_egl_window_destroy(background->egl_window);
	eglTerminate(background->egl_display);
	eglReleaseThread();
}

// Only support center cropping.
void background_update_matrix(struct background *background)
{
	struct client *client = background->client;
#ifdef __DEBUG__
	printf("Wallpaper size: %dx%d\n", client->wallpaper_width, client->wallpaper_height);
	printf("Background size: %dx%d\n", background->width, background->height);
#endif
	float wallpaper_ratio =
		(float)client->wallpaper_width / client->wallpaper_height;
	float output_ratio = (float)background->width / background->height;
	if (wallpaper_ratio >= output_ratio) {
#ifdef __DEBUG__
		printf("X-Cropping wallpaper.\n");
#endif
		background->mvp = m4scale(v3s(
			(
				(float)client->wallpaper_width / client->wallpaper_height
			) / (
				(float)background->width / background->height
			),
			1,
			1
		));
	} else {
#ifdef __DEBUG__
		printf("Y-Cropping wallpaper.\n");
#endif
		background->mvp = m4scale(v3s(
			1,
			(
				(float)client->wallpaper_height / client->wallpaper_width
			) / (
				(float)background->height / background->width
			),
			1
		));
	}
}

void background_render(struct background *background)
{
#ifdef __DEBUG__
	printf("Render.\n");
#endif
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(background->programs[0]);
	glBindVertexArray(background->vertex_arrays[0]);
	glBindTexture(GL_TEXTURE_2D, background->textures[0]);
	glUniformMatrix4fv(
		glGetUniformLocation(background->programs[0], "mvp"),
		1,
		false,
		MARRAY(background->mvp)
	);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
	glUseProgram(0);
	eglSwapBuffers(background->egl_display, background->egl_surface);
}

void background_destroy(struct background *background)
{
#ifdef __DEBUG__
	printf("Destroying background %s.\n", background->name);
#endif
	if (background->layer_surface)
		zwlr_layer_surface_v1_destroy(background->layer_surface);
	if (background->surface)
		wl_surface_destroy(background->surface);
	zxdg_output_v1_destroy(background->xdg_output);
	wl_output_destroy(background->output);
	free(background->name);
	free(background);
}

static void layer_surface_configure(
	void *data,
	struct zwlr_layer_surface_v1 *layer_surface,
	uint32_t serial,
	uint32_t width,
	uint32_t height
)
{
	struct background *background = data;
#ifdef __DEBUG__
	printf("Configure.\n");
#endif
	background->width = width;
	background->height = height;
	zwlr_layer_surface_v1_ack_configure(layer_surface, serial);
	// Resize after configure.
	if (background->running) {
		wl_egl_window_resize(
			background->egl_window,
			background->width,
			background->height,
			0,
			0
		);
		glViewport(0, 0, width, height);
		background_update_matrix(background);
		background_render(background);
	}
}

static void layer_surface_closed(
	void *data,
	struct zwlr_layer_surface_v1 *layer_surface
)
{
	struct background *background = data;
	wl_list_remove(&background->link);
	background_destroy(background);
}

static struct zwlr_layer_surface_v1_listener layer_surface_listener = {
	.configure = layer_surface_configure,
	.closed = layer_surface_closed
};

static void output_geometry(
	void *data,
	struct wl_output *output,
	int32_t x,
	int32_t y,
	int32_t width_mm,
	int32_t height_mm,
	int32_t subpixel,
	const char *make,
	const char *model,
	int32_t transform
)
{

}

static void output_mode(
	void *data,
	struct wl_output *output,
	uint32_t flags,
	int32_t width,
	int32_t height,
	int32_t refresh
)
{

}

static void output_scale(void *data, struct wl_output *output, int32_t scale)
{
	struct background *background = data;
	background->scale = scale;
	if (background->running)
		background_render(background);
}

static void output_done(void *data, struct wl_output *output)
{

}

static struct wl_output_listener output_listener = {
	.geometry = output_geometry,
	.mode = output_mode,
	.scale = output_scale,
	.done = output_done
};

static void xdg_output_logical_position(
	void *data,
	struct zxdg_output_v1 *xdg_output,
	int32_t x,
	int32_t y
)
{

}

static void xdg_output_logical_size(
	void *data,
	struct zxdg_output_v1 *xdg_output,
	int32_t width,
	int32_t height
)
{

}

static void xdg_output_name(
	void *data,
	struct zxdg_output_v1 *xdg_output,
	const char *name
)
{
	struct background *background = data;
	background->name = strdup(name);
}

static void xdg_output_description(
	void *data,
	struct zxdg_output_v1 *xdg_output,
	const char *description
)
{

}

static void xdg_output_done(void *data, struct zxdg_output_v1 *xdg_output)
{
	struct background *background = data;
	struct client *client = background->client;
	if (background->layer_surface == NULL) {
#ifdef __DEBUG__
		printf(
			"Creating background layer for output %s.\n",
			background->name
		);
#endif
		background->surface = wl_compositor_create_surface(
			client->compositor
		);
		if (background->surface == NULL) {
			fprintf(stderr, "Failed to create surface.\n");
			exit(EXIT_FAILURE);
		}

		struct wl_region *empty_region = wl_compositor_create_region(
			client->compositor
		);
		if (empty_region == NULL) {
			fprintf(stderr, "Failed to create region.\n");
			exit(EXIT_FAILURE);
		}
		wl_surface_set_input_region(background->surface, empty_region);
		wl_region_destroy(empty_region);

		background->layer_surface = zwlr_layer_shell_v1_get_layer_surface(
			client->layer_shell,
			background->surface,
			background->output,
			ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND,
			CLIENT_TITLE
		);
		if (background->layer_surface == NULL) {
			fprintf(stderr, "Failed to get layer_surface.\n");
			exit(EXIT_FAILURE);
		}

		zwlr_layer_surface_v1_set_size(background->layer_surface, 0, 0);
		zwlr_layer_surface_v1_set_anchor(
			background->layer_surface,
			ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
			ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT |
			ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
			ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT
		);
		zwlr_layer_surface_v1_set_exclusive_zone(
			background->layer_surface,
			-1
		);
		zwlr_layer_surface_v1_add_listener(
			background->layer_surface,
			&layer_surface_listener,
			background
		);

		wl_surface_commit(background->surface);
		wl_display_roundtrip(client->display);

		init_egl(background);
		init_gl(background);
		// Render initial frame.
		background_update_matrix(background);
		background_render(background);
		background->running = true;
	}
}

static struct zxdg_output_v1_listener xdg_output_listener = {
	.logical_position = xdg_output_logical_position,
	.logical_size = xdg_output_logical_size,
	.name = xdg_output_name,
	.description = xdg_output_description,
	.done = xdg_output_done
};

static void registry_global(
	void *data,
	struct wl_registry *registry,
	uint32_t id,
	const char *interface,
	uint32_t version
)
{
	struct client *client = data;
#ifdef __DEBUG__
	printf(
		"Got interface: %s, id: %d, version: %d.\n",
		interface,
		id,
		version
	);
#endif
	// Bind global to client.
	if (strcmp(interface, wl_compositor_interface.name) == 0) {
		client->compositor = wl_registry_bind(
			registry,
			id,
			&wl_compositor_interface,
			4
		);
	} else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
		client->layer_shell = wl_registry_bind(
			registry,
			id,
			&zwlr_layer_shell_v1_interface,
			1
		);
	} else if (strcmp(interface, zxdg_output_manager_v1_interface.name) == 0) {
		client->xdg_output_manager = wl_registry_bind(
			registry,
			id,
			&zxdg_output_manager_v1_interface,
			2
		);
	} else if (strcmp(interface, wl_output_interface.name) == 0) {
		struct background *background = malloc(sizeof(*background));
		background->client = client;
		background->running = false;
		background->scale = 1;
		background->id = id;
		background->layer_surface = NULL;
		background->output = wl_registry_bind(
			registry,
			id,
			&wl_output_interface,
			3
		);
		wl_output_add_listener(
			background->output,
			&output_listener,
			background
		);
		wl_list_insert(&client->backgrounds, &background->link);
		background->xdg_output = zxdg_output_manager_v1_get_xdg_output(
			client->xdg_output_manager,
			background->output
		);
		zxdg_output_v1_add_listener(
			background->xdg_output,
			&xdg_output_listener,
			background
		);
	}
}

static void registry_global_remove(
	void *data,
	struct wl_registry *registry,
	uint32_t id
)
{
	struct client *client = data;
	struct background *background;
	struct background *tmp;
	wl_list_for_each_safe(background, tmp, &client->backgrounds, link) {
		if (background->id == id) {
			wl_list_remove(&background->link);
			background_destroy(background);
			break;
		}
	}
}

static struct wl_registry_listener registry_listener = {
	registry_global,
	registry_global_remove
};

void init_wl(struct client *client)
{
	client->display = NULL;
	client->compositor = NULL;
	client->running = true;
	client->layer_shell = NULL;
	wl_list_init(&client->backgrounds);

	client->display = wl_display_connect(NULL);
	if (client->display == NULL) {
		fprintf(stderr, "Cannot connect to display.\n");
		exit(EXIT_FAILURE);
	}

	client->registry = wl_display_get_registry(client->display);
	wl_registry_add_listener(client->registry, &registry_listener, client);
	// Send request and wait for globals.
	wl_display_roundtrip(client->display);

	if (client->compositor == NULL) {
		fprintf(stderr, "Cannot get compositor.\n");
		exit(EXIT_FAILURE);
	}

	if (client->layer_shell == NULL) {
		fprintf(stderr, "Cannot get layer_shell.\n");
		exit(EXIT_FAILURE);
	}

	if (client->xdg_output_manager == NULL) {
		fprintf(stderr, "Cannot get xdg_output_manager.\n");
		exit(EXIT_FAILURE);
	}
}

void finish_wl(struct client *client)
{
	struct background *background;
	struct background *tmp;
	wl_list_for_each_safe(background, tmp, &client->backgrounds, link) {
		wl_list_remove(&background->link);
		background_destroy(background);
	}
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <wallpaper file path>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	struct client client;
	client.wallpaper_path = argv[1];
	client.wallpaper_buffer = NULL;
	int channels;
	// Force 3 channel because we only need RGB for wallpapers.
	client.wallpaper_buffer = stbi_load(
		client.wallpaper_path,
		&client.wallpaper_width,
		&client.wallpaper_height,
		&channels,
		3
	);
	if (client.wallpaper_buffer == NULL) {
		fprintf(stderr, "Failed to load wallpaper %s!\n", client.wallpaper_path);
		exit(EXIT_FAILURE);
	}
	init_wl(&client);

	while (client.running && wl_display_dispatch(client.display) != -1) {

	}

	finish_wl(&client);
	stbi_image_free(client.wallpaper_buffer);

	return 0;
}