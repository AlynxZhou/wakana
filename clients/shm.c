// Compile it with `-std=gnu11`.
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <wayland-client.h>
#include <wayland-client-protocol.h>
#include <errno.h>
#include <fcntl.h>
// For `mmap()`.
#include <sys/mman.h>
#include <unistd.h>
#include "xdg-shell-client-protocol.h"

#define CLIENT_TITLE "shm"
#define CLIENT_WIDTH 800
#define CLIENT_HEIGHT 600
#define BYTES_PER_PIXEL 4

struct client {
	struct wl_display *display;
	struct wl_registry *registry;
	struct wl_compositor *compositor;
	struct wl_shm *shm;
	struct xdg_wm_base *xdg_wm_base;
	struct wl_buffer *buffer;
	void *shm_data;
	bool buffer_busy;
	struct wl_surface *surface;
	struct xdg_surface *xdg_surface;
	struct xdg_toplevel *xdg_toplevel;
	int width;
	int height;
	bool running;
};

static int set_cloexec_or_close(int fd)
{
        if (fd == -1)
                return -1;

        long flags = fcntl(fd, F_GETFD);
        if (flags == -1) {
		close(fd);
		return -1;
	}

        if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1) {
		close(fd);
		return -1;
	}

        return fd;
}

static int create_tmpfile_cloexec(char *tmpname)
{
#ifdef HAVE_MKOSTEMP
        int fd = mkostemp(tmpname, O_CLOEXEC);
        if (fd >= 0)
                unlink(tmpname);
#else
        int fd = mkstemp(tmpname);
        if (fd >= 0) {
                fd = set_cloexec_or_close(fd);
                unlink(tmpname);
        }
#endif
        return fd;
}

static int os_create_anonymous_file(off_t size)
{
        static const char template[] = "/wakana-shared-XXXXXX";

        char *path = getenv("XDG_RUNTIME_DIR");
        if (!path) {
		errno = ENOENT;
                return -1;
        }

        char *name = malloc(strlen(path) + sizeof(template));
        if (!name)
                return -1;
        strncpy(name, path, strlen(path) + 1);
        strncat(name, template, sizeof(template));

        int fd = create_tmpfile_cloexec(name);

        free(name);

        if (fd < 0)
                return -1;

        if (ftruncate(fd, size) < 0) {
                close(fd);
                return -1;
        }

        return fd;
}

void paint_pixels(void *shm_data, int width, int height)
{
	uint32_t *pixel = shm_data;
	for (int i = 0; i < width * height; ++i)
		*pixel++ = 0x55667788;
}

static void redraw(void *data, struct wl_callback *callback, uint32_t time);
static struct wl_callback_listener frame_listener = {
	redraw
};

// TODO: What's this?
static void redraw(void *data, struct wl_callback *callback, uint32_t time)
{
	struct client *client = data;
	paint_pixels(client->shm_data, client->width, client->height);

	wl_surface_attach(client->surface, client->buffer, 0, 0);
	wl_surface_damage(client->surface, 0, 0, client->width, client->height);

	if (callback != NULL)
		wl_callback_destroy(callback);

	wl_callback_add_listener(
		wl_surface_frame(client->surface),
		&frame_listener,
		client
	);
	wl_surface_commit(client->surface);
	client->buffer_busy = true;
}

static void buffer_release(
	void *data,
	struct wl_buffer *buffer
)
{
	struct client *client = data;
	client->buffer_busy = false;
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

static void shm_format(
	void *data,
	struct wl_shm *shm,
	uint32_t format
)
{
	char *s;
	switch (format) {
	case WL_SHM_FORMAT_ARGB8888:
		s = "ARGB8888";
		break;  
	case WL_SHM_FORMAT_XRGB8888:
		s = "XRGB8888";
		break;  
	case WL_SHM_FORMAT_RGB565:
		s = "RGB565";
		break;  
	default:
		s = "other format";
		break;  
	}
	printf("Possible shmem format %s.\n", s);
}

static struct xdg_wm_base_listener xdg_wm_base_listener = {
	xdg_wm_base_ping,
};
static struct wl_shm_listener shm_listener = {
	shm_format
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
	} else if (strcmp(interface, "wl_shm") == 0) {
		client->shm = wl_registry_bind(
			registry,
			id,
			&wl_shm_interface,
			1
		);
		wl_shm_add_listener(client->shm, &shm_listener, client);
	}
}

static void registry_global_remove(
	void *data,
	struct wl_registry *registry,
	uint32_t id
)
{

}

int main(int argc, char *argv[])
{
	struct client client;
	client.display = NULL;
	client.compositor = NULL;
	client.shm_data = NULL;
	client.running = true;
	client.buffer_busy = false;
	client.width = CLIENT_WIDTH;
	client.height = CLIENT_HEIGHT;

	client.display = wl_display_connect(NULL);
	if (client.display == NULL) {
		fprintf(stderr, "Cannot connect to display.\n");
		exit(EXIT_FAILURE);
	}

	client.registry = wl_display_get_registry(client.display);
	struct wl_registry_listener registry_listener = {
		registry_global,
		registry_global_remove
	};
	wl_registry_add_listener(client.registry, &registry_listener, &client);
	// Send request and wait.
	wl_display_roundtrip(client.display);

	if (client.compositor == NULL) {
		fprintf(stderr, "Cannot get compositor.\n");
		exit(EXIT_FAILURE);
	}

	client.surface = wl_compositor_create_surface(client.compositor);
	if (client.surface == NULL) {
		fprintf(stderr, "Cannot create surface.\n");
		exit(EXIT_FAILURE);
	}

	client.xdg_surface = xdg_wm_base_get_xdg_surface(
		client.xdg_wm_base,
		client.surface
	);
	if (client.xdg_surface == NULL) {
		fprintf(stderr, "Cannot get xdg surface.\n");
		exit(EXIT_FAILURE);
	}
	// TODO: What is this?
	struct xdg_surface_listener xdg_surface_listener = {
		xdg_surface_configure
	};
	xdg_surface_add_listener(
		client.xdg_surface,
		&xdg_surface_listener,
		&client
	);

	client.xdg_toplevel = xdg_surface_get_toplevel(client.xdg_surface);
	if (client.xdg_toplevel == NULL) {
		fprintf(stderr, "Cannot get xdg toplevel.\n");
		exit(EXIT_FAILURE);
	}
	xdg_toplevel_set_title(client.xdg_toplevel, CLIENT_TITLE);
	struct xdg_toplevel_listener xdg_toplevel_listener = {
		xdg_toplevel_configure,
		xdg_toplevel_close
	};
	xdg_toplevel_add_listener(
		client.xdg_toplevel,
		&xdg_toplevel_listener,
		&client
	);

	wl_surface_commit(client.surface);

	int shm_data_size = client.width * client.height * BYTES_PER_PIXEL;
	int fd = os_create_anonymous_file(shm_data_size);
	if (fd < 0) {
		fprintf(stderr, "Cannot create temp file.\n");
		exit(EXIT_FAILURE);
	}

	client.shm_data = mmap(
		NULL,
		shm_data_size,
		PROT_READ | PROT_WRITE,
		MAP_SHARED,
		fd,
		0
	);
	if (client.shm_data == MAP_FAILED) {
		fprintf(stderr, "Cannot map shm data.\n");
		close(fd);
		exit(EXIT_FAILURE);
	}

	struct wl_shm_pool *shm_pool = wl_shm_create_pool(
		client.shm,
		fd,
		shm_data_size
	);
	client.buffer = wl_shm_pool_create_buffer(
		shm_pool,
		0,
		client.width,
		client.height,
		client.width * BYTES_PER_PIXEL,
		// Read from shm listener?
		WL_SHM_FORMAT_ARGB8888
	);
	struct wl_buffer_listener buffer_listener = {
		buffer_release
	};
	wl_buffer_add_listener(client.buffer, &buffer_listener, &client);
	wl_shm_pool_destroy(shm_pool);
	close(fd);

	// Wait for configure.
	wl_display_roundtrip(client.display);

	// Trigger drawing.
	wl_surface_damage(client.surface, 0, 0, client.width, client.height);
	redraw(&client, NULL, 0);

	while (client.running && wl_display_dispatch(client.display) != -1) {

	}

	return 0;
}