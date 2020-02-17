// TODO: Add more comments and debug output.
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <wayland-client.h>
#include <wayland-client-protocol.h>
#include <wayland-egl.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include "loaders/loaders.h"
#include "mvmath/mvmath.h"
#include "xdg-shell-client-protocol.h"

#define CLIENT_TITLE "egl"
#define CLIENT_WIDTH 800
#define CLIENT_HEIGHT 600

struct client {
	struct wl_display *display;
	struct wl_registry *registry;
	struct wl_compositor *compositor;
	struct xdg_wm_base *xdg_wm_base;
	struct wl_surface *surface;
	struct xdg_surface *xdg_surface;
	struct xdg_toplevel *xdg_toplevel;
	struct wl_egl_window *egl_window;
	EGLDisplay egl_display;
	EGLConfig egl_config;
	EGLSurface egl_surface;
	EGLContext egl_context;
	unsigned int program;
	unsigned int buffers[3];
	unsigned int vertex_arrays[1];
	int width;
	int height;
	bool running;
};

static void render(void *data, struct wl_callback *callback, uint32_t time);
static struct wl_callback_listener frame_listener = {
	render
};
static void render(void *data, struct wl_callback *callback, uint32_t time)
{
	if (callback != NULL)
		wl_callback_destroy(callback);

	struct client *client = data;
	
	static int self_angle = 0;
	static int angle = 0;

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	mat4 view = m4camera(
		// My position.
		v3s(0.0f, 0.0f, 0.0f),
		// My direction.
		v3add(v3s(0.0f, 0.0f, 0.0f), v3s(0.0f, 0.0f, 1.0f)),
		// My up.
		v3s(0.0f, 1.0f, 0.0f)
	);
	mat4 projection = m4perspective(
		sradians(90.0f),
		(float)client->width / client->height,
		0.1f,
		100.0f
	);
	mat4 model = m4multiply(
		// Position.
		m4translate(v3s(0.0f, 0.0f, 5.0f)),
		m4multiply(
			// Rotation
			m4rotate(v3s(0.0f, 1.0f, 0.0f), sradians(angle)),
			m4multiply(
				// Rotation range.
				m4translate(v3s(0.0f, 0.0f, 2.0f)),
				m4multiply(
					// Self rotation.
					m4rotate(
						v3s(0.0f, 1.0f, 0.0f),
						sradians(self_angle)
					),
					// Size.
					m4scale(v3s(1.0f, 1.0f, 1.0f))
				)
			)
		)
	);
	mat4 mvp = m4multiply(projection, m4multiply(view, model));

	glUseProgram(client->program);
	glBindVertexArray(client->vertex_arrays[0]);
	glUniformMatrix4fv(
		glGetUniformLocation(client->program, "mvp"),
		1,
		false,
		MARRAY(mvp)
	);
	glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
	glUseProgram(0);

	angle = (angle + 1) % 360;
	self_angle = (self_angle + 1) % 360;

	// Trigger next render.	
	wl_callback_add_listener(
		wl_surface_frame(client->surface),
		&frame_listener,
		client
	);

	// This must under `wl_callback_add_listener()`.
	// It is async and can work as damage and commit.
	eglSwapBuffers(client->egl_display, client->egl_surface);
}

static void xdg_surface_configure(
	void *data,
	struct xdg_surface *xdg_surface,
	uint32_t serial
)
{
	struct client *client = data;
	xdg_surface_ack_configure(client->xdg_surface, serial);
}

static void xdg_toplevel_configure(
	void *data,
	struct xdg_toplevel *xdg_toplevel,
	int32_t width,
	int32_t height,
	struct wl_array *state
)
{

}

static void xdg_toplevel_close(
	void *data,
	struct xdg_toplevel *xdg_toplevel
)
{
	struct client *client = data;
	client->running = false;
}

static void xdg_wm_base_ping(
	void *data,
	struct xdg_wm_base *xdg_wm_base,
	uint32_t serial
)
{
	xdg_wm_base_pong(xdg_wm_base, serial);
}

static struct xdg_wm_base_listener xdg_wm_base_listener = {
	xdg_wm_base_ping,
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
	printf("Got interface: %s, id: %d.\n", interface, id);
#endif
	// Bind global to client.
	if (strcmp(interface, "wl_compositor") == 0) {
		client->compositor = wl_registry_bind(
			registry,
			id,
			&wl_compositor_interface,
			1
		);
	} else if (strcmp(interface, "xdg_wm_base") == 0) {
		client->xdg_wm_base = wl_registry_bind(
			registry,
			id,
			&xdg_wm_base_interface,
			1
		);
		xdg_wm_base_add_listener(
			client->xdg_wm_base,
			&xdg_wm_base_listener,
			client
		);
	}
}

static void registry_global_remove(
	void *data,
	struct wl_registry *registry,
	uint32_t id
)
{

}

static struct wl_registry_listener registry_listener = {
	registry_global,
	registry_global_remove
};
// TODO: What is this?
static struct xdg_surface_listener xdg_surface_listener = {
	xdg_surface_configure
};
static struct xdg_toplevel_listener xdg_toplevel_listener = {
	xdg_toplevel_configure,
	xdg_toplevel_close
};
void init_wl(struct client *client)
{
	client->display = NULL;
	client->compositor = NULL;
	client->running = true;
	client->width = CLIENT_WIDTH;
	client->height = CLIENT_HEIGHT;

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

	client->surface = wl_compositor_create_surface(client->compositor);
	if (client->surface == NULL) {
		fprintf(stderr, "Cannot create surface.\n");
		exit(EXIT_FAILURE);
	}

	client->xdg_surface = xdg_wm_base_get_xdg_surface(
		client->xdg_wm_base,
		client->surface
	);
	if (client->xdg_surface == NULL) {
		fprintf(stderr, "Cannot get xdg surface.\n");
		exit(EXIT_FAILURE);
	}
	xdg_surface_add_listener(
		client->xdg_surface,
		&xdg_surface_listener,
		client
	);

	client->xdg_toplevel = xdg_surface_get_toplevel(client->xdg_surface);
	if (client->xdg_toplevel == NULL) {
		fprintf(stderr, "Cannot get xdg toplevel.\n");
		exit(EXIT_FAILURE);
	}
	xdg_toplevel_set_title(client->xdg_toplevel, CLIENT_TITLE);
	xdg_toplevel_add_listener(
		client->xdg_toplevel,
		&xdg_toplevel_listener,
		client
	);

	wl_surface_commit(client->surface);
	wl_display_roundtrip(client->display);
}

void init_egl(struct client *client)
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
		EGL_ALPHA_SIZE, 8,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
		EGL_NONE
	};

	EGLint context_attribs[] = {
		EGL_CONTEXT_MAJOR_VERSION, 3,
		EGL_NONE
	};

	client->egl_display = eglGetDisplay(
		(EGLNativeDisplayType)client->display
	);
	if (client->egl_display == EGL_NO_DISPLAY) {
		fprintf(stderr, "Cannot get egl display.\n");
		exit(EXIT_FAILURE);
	}
	if (!eglInitialize(client->egl_display, &major, &minor)) {
		fprintf(stderr, "Cannot initialize egl.\n");
		exit(EXIT_FAILURE);
	}
	eglBindAPI(EGL_OPENGL_ES_API);
#ifdef __DEBUG__
	printf("EGL version: major: %d, minor: %d.\n", major, minor);
#endif

	eglGetConfigs(client->egl_display, NULL, 0, &count);
	configs = calloc(count, sizeof(*configs));
	eglChooseConfig(
		client->egl_display,
		config_attribs,
		configs,
		count,
		&n
	);
	for (int i = 0; i < n; ++i) {
#ifdef __DEBUG__
		eglGetConfigAttrib(
			client->egl_display,
			configs[i],
			EGL_RED_SIZE,
			&size
		);
		printf("Config %d: EGL_RED_SIZE: %d.\n", i, size);
#endif
#ifdef __DEBUG__
		eglGetConfigAttrib(
			client->egl_display,
			configs[i],
			EGL_GREEN_SIZE,
			&size
		);
		printf("Config %d: EGL_GREEN_SIZE: %d.\n", i, size);
#endif
#ifdef __DEBUG__
		eglGetConfigAttrib(
			client->egl_display,
			configs[i],
			EGL_BLUE_SIZE,
			&size
		);
		printf("Config %d: EGL_BLUE_SIZE: %d.\n", i, size);
#endif
		eglGetConfigAttrib(
			client->egl_display,
			configs[i],
			EGL_BUFFER_SIZE,
			&size
		);
		if (size == 4 * 8) {
			client->egl_config = configs[i];
			break;
		}
	}
	free(configs);

	client->egl_context = eglCreateContext(
		client->egl_display,
		client->egl_config,
		EGL_NO_CONTEXT,
		context_attribs
	);
	if (client->egl_context == EGL_NO_CONTEXT) {
		fprintf(stderr, "Cannot create egl context.\n");
		exit(EXIT_FAILURE);
	}

	client->egl_window = wl_egl_window_create(
		client->surface,
		client->width,
		client->height
	);
	if (client->egl_window == EGL_NO_SURFACE) {
		fprintf(stderr, "Cannot create egl window.\n");
		exit(EXIT_FAILURE);
	}

	client->egl_surface = eglCreateWindowSurface(
		client->egl_display,
		client->egl_config,
		client->egl_window,
		NULL
	);
	if (!eglMakeCurrent(
		client->egl_display,
		client->egl_surface,
		client->egl_surface,
		client->egl_context
	)) {
		fprintf(stderr, "Cannot make current.\n");
		exit(EXIT_FAILURE);
	}
	eglSwapInterval(client->egl_display, 0);
}

void init_gl(struct client *client)
{
	client->program = load_program(
		"../shaders/egl.v.glsl",
		"../shaders/egl.f.glsl"
	);

	glGenBuffers(3, client->buffers);

	// (1,1,1), (1,-1,-1), (-1,1,-1), (-1,-1,1)
	const float vertices[] = {
		1.0f, 1.0f, 1.0f,
		1.0f, -1.0f, -1.0f,
		-1.0f, 1.0f, -1.0f,
		-1.0f, -1.0f, 1.0f
	};
	glBindBuffer(GL_ARRAY_BUFFER, client->buffers[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	const float colors[] = {
		0.0f, 0.0f, 1.0f,
		0.0f, 1.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 1.0f, 1.0f
	};
	glBindBuffer(GL_ARRAY_BUFFER, client->buffers[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	const unsigned int indices[] = {
		0, 1, 3,
		0, 2, 1,
		0, 3, 2,
		1, 2, 3
	};
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, client->buffers[2]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glGenVertexArrays(1, client->vertex_arrays);
	
	glBindVertexArray(client->vertex_arrays[0]);

	glBindBuffer(GL_ARRAY_BUFFER, client->buffers[0]);
	glVertexAttribPointer(
		glGetAttribLocation(client->program, "in_position"),
		3,
		GL_FLOAT,
		GL_FALSE,
		3 * sizeof(*vertices),
		0
	);
	glEnableVertexAttribArray(
		glGetAttribLocation(client->program, "in_position")
	);

	glBindBuffer(GL_ARRAY_BUFFER, client->buffers[1]);
	glVertexAttribPointer(
		glGetAttribLocation(client->program, "in_color"),
		3,
		GL_FLOAT,
		GL_FALSE,
		3 * sizeof(*colors),
		0
	);
	glEnableVertexAttribArray(
		glGetAttribLocation(client->program, "in_color")
	);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, client->buffers[2]);

	glBindVertexArray(0);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	glViewport(0, 0, client->width, client->height);
}

void finish_gl(struct client *client)
{
	glDeleteBuffers(3, client->buffers);
	glDeleteVertexArrays(1, client->vertex_arrays);
	free_program(client->program);
}

void finish_egl(struct client *client)
{
	eglDestroyContext(client->egl_display, client->egl_context);
	eglDestroySurface(client->egl_display, client->egl_surface);
	wl_egl_window_destroy(client->egl_window);
	eglTerminate(client->egl_display);
	eglReleaseThread();
}

void finish_wl(struct client *client)
{
	xdg_toplevel_destroy(client->xdg_toplevel);
	xdg_surface_destroy(client->xdg_surface);
	wl_surface_destroy(client->surface);
}

int main(int argc, char *argv[])
{
	struct client client;

	init_wl(&client);
	init_egl(&client);
	init_gl(&client);
	
	// Render starts.
	render(&client, NULL, 0);

	// Main loop.
	while (client.running && wl_display_dispatch(client.display) != -1) {

	}

	finish_gl(&client);
	finish_egl(&client);
	finish_wl(&client);

	return 0;
}