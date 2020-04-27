#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#define WLR_USE_UNSTABLE
#include <wlr/backend/session.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_xdg_output_v1.h>
#include "logger.h"
#include "server.h"
#include "output.h"
#include "keyboard.h"

static void wkn_server_new_input_notify(
	struct wl_listener *listener,
	void *data
)
{
	struct wkn_server *server = wl_container_of(
		listener,
		server,
		new_input
	);
	struct wlr_input_device *device = data;
	switch (device->type) {
	case WLR_INPUT_DEVICE_POINTER: {
		wlr_cursor_attach_input_device(
			server->cursor->wlr_cursor,
			device
		);
		break;
	}
	case WLR_INPUT_DEVICE_KEYBOARD: {
		struct wkn_keyboard *keyboard = wkn_keyboard_create(
			server,
			device
		);
		wl_list_insert(&server->keyboards, &keyboard->link);
		break;
	}
	default:
		break;
	}
	uint32_t caps = WL_SEAT_CAPABILITY_POINTER;
	if (!wl_list_empty(&server->keyboards))
		caps |= WL_SEAT_CAPABILITY_KEYBOARD;
	wlr_seat_set_capabilities(server->seat->wlr_seat, caps);
}

static void wkn_server_new_xdg_surface_notify(
	struct wl_listener *listener,
	void *data
)
{
	struct wkn_server *server = wl_container_of(
		listener,
		server,
		new_xdg_surface
	);
	struct wlr_xdg_surface *wlr_xdg_surface = data;

	if (wlr_xdg_surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
		return;
	}

	wlr_xdg_surface_ping(wlr_xdg_surface);

	struct wkn_xdg_surface *xdg_surface = wkn_xdg_surface_create(
		server,
		wlr_xdg_surface
	);
	if (wl_list_empty(&server->outputs))
		return;
	// Attach to the first output.
	struct wkn_output *output = wl_container_of(
		server->outputs.next,
		output,
		link
	);
	xdg_surface->output = output;
	wl_list_insert(&output->xdg_surfaces, &xdg_surface->link);
}

static void wkn_server_new_layer_surface_notify(
	struct wl_listener *listener,
	void *data
)
{
	struct wkn_server *server = wl_container_of(
		listener,
		server,
		new_layer_surface
	);
	struct wlr_layer_surface_v1 *wlr_layer_surface = data;

	struct wkn_output *output = NULL;
	if (!wlr_layer_surface->output) {
		// Just attach it to the first output.
		if (wl_list_empty(&server->outputs))
			return;
		output = wl_container_of(server->outputs.next, output, link);
		wlr_layer_surface->output = output->wlr_output;
	} else {
		struct wkn_output *iter;
		wl_list_for_each(iter, &server->outputs, link) {
			if (iter->wlr_output == wlr_layer_surface->output) {
				output = iter;
				break;
			}
		}
		if (output == NULL)
			return;
	}

	struct wkn_layer_surface *layer_surface = wkn_layer_surface_create(
		server,
		wlr_layer_surface
	);
	layer_surface->output = output;
	wl_list_insert(
		&(output->layer_surfaces[wlr_layer_surface->current.layer]),
		&(layer_surface->link)
	);
	wkn_output_arrange_layer_surfaces(
		output,
		wlr_layer_surface->current.layer
	);
}

static void wkn_server_new_output_notify(
	struct wl_listener *listener,
	void *data
)
{
	// See https://gitlab.freedesktop.org/wayland/wayland/blob/master/src/wayland-util.h#L373 for wl_container_of.
	struct wkn_server *server = wl_container_of(
		listener, server, new_output
	);
	struct wlr_output *wlr_output = data;

	wlr_output_create_global(wlr_output);
	// See https://gitlab.freedesktop.org/wayland/wayland/blob/master/src/wayland-util.h#L231 for wl_list.
	if (!wl_list_empty(&wlr_output->modes)) {
		// Get preferred output mode here.
		struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
		wlr_output_set_mode(wlr_output, mode);
	}

	struct wkn_output *output = wkn_output_create(server, wlr_output);
	wl_list_insert(&server->outputs, &output->link);
	wlr_output_layout_add_auto(server->wlr_output_layout, wlr_output);
	wlr_output_create_global(wlr_output);
}

void wkn_server_move_focused_xdg_surface(struct wkn_server *server)
{
	struct wkn_xdg_surface *xdg_surface = server->focused_xdg_surface;
	struct wkn_cursor *cursor = server->cursor;
	int dx = server->request_cursor_x - cursor->wlr_cursor->x;
	int dy = server->request_cursor_y - cursor->wlr_cursor->y;
	xdg_surface->rect.x = server->request_rect.x - dx;
	xdg_surface->rect.y = server->request_rect.y - dy;
}

void wkn_server_resize_focused_xdg_surface(struct wkn_server *server)
{
	struct wkn_xdg_surface *xdg_surface = server->focused_xdg_surface;
	struct wkn_cursor *cursor = server->cursor;
	int dx = server->request_cursor_x - cursor->wlr_cursor->x;
	int dy = server->request_cursor_y - cursor->wlr_cursor->y;
	struct wkn_rect rect;
	if (server->request_resize_edges & WLR_EDGE_LEFT) {
		rect.x = server->request_rect.x - dx;
		rect.w = server->request_rect.w + dx;
	} else if (server->request_resize_edges & WLR_EDGE_RIGHT) {
		rect.x = server->request_rect.x;
		rect.w = server->request_rect.w - dx;
	} else {
		rect.x = server->request_rect.x;
		rect.w = server->request_rect.w;
	}
	if (server->request_resize_edges & WLR_EDGE_TOP) {
		rect.y = server->request_rect.y - dy;
		rect.h = server->request_rect.h + dy;
	} else if (server->request_resize_edges & WLR_EDGE_BOTTOM) {
		rect.y = server->request_rect.y;
		rect.h = server->request_rect.h - dy;
	} else {
		rect.y = server->request_rect.y;
		rect.h = server->request_rect.h;
	}

	wlr_xdg_toplevel_set_size(xdg_surface->wlr_xdg_surface, rect.w, rect.h);
	// Better to wait for the xdg_surface's redrawing then do recompositing.
	xdg_surface->rect = rect;
}

void wkn_server_update_keys(
	struct wkn_server *server,
	uint32_t keycode,
	enum wlr_key_state state
)
{
	if (keycode >= KEYCODE_NUMS)
		return;
	server->key_states[keycode] = state;
}

bool wkn_server_handle_keybindings(struct wkn_server *server)
{
	if (server->key_states[KEY_LEFTCTRL] == WLR_KEY_PRESSED &&
	    server->key_states[KEY_LEFTALT] == WLR_KEY_PRESSED &&
	    server->key_states[KEY_BACKSPACE] == WLR_KEY_PRESSED) {
		wkn_logger_debug(server->logger, "Restart!\n");
		// TODO: Restart session.
		return true;
	}
	if ((server->key_states[KEY_LEFTMETA] == WLR_KEY_PRESSED &&
	     server->key_states[KEY_ESC] == WLR_KEY_PRESSED) ||
	    (server->key_states[KEY_LEFTCTRL] == WLR_KEY_PRESSED &&
	     server->key_states[KEY_LEFTALT] == WLR_KEY_PRESSED &&
	     server->key_states[KEY_ESC] == WLR_KEY_PRESSED)) {
		wkn_logger_debug(server->logger, "Exit!\n");
		// TODO: better exit.
		// wl_display_terminate(server->wl_display);
		// wkn_server_destroy(server);
		exit(EXIT_SUCCESS);
		return true;
	}
	return false;
}

struct wkn_xdg_surface *wkn_server_find_xdg_surface_at(
	struct wkn_server *server,
	int layout_x,
	int layout_y
)
{
	struct wkn_xdg_surface *xdg_surface = NULL;
	struct wkn_output *output;
	wl_list_for_each(output, &server->outputs, link) {
		xdg_surface = wkn_output_find_xdg_surface_at(
			output,
			layout_x,
			layout_y
		);
		if (xdg_surface != NULL)
			break;
	}
	return xdg_surface;
}

struct wkn_server *wkn_server_create(void)
{
	struct wkn_server *server = malloc(sizeof(*server));
	assert(server);

	server->wl_display = wl_display_create();
	assert(server->wl_display);

	server->wl_event_loop = wl_display_get_event_loop(server->wl_display);
	assert(server->wl_event_loop);

	server->wlr_backend = wlr_backend_autocreate(server->wl_display, NULL);
	assert(server->wlr_backend);

	return server;
}

void wkn_server_setup(
	struct wkn_server *server,
	bool debug,
	char *logpath
)
{
	server->logger = wkn_logger_create(server, debug, logpath);
	assert(server->logger);

	server->wlr_renderer = wlr_backend_get_renderer(server->wlr_backend);
	assert(server->wlr_renderer);
	wlr_renderer_init_wl_display(server->wlr_renderer, server->wl_display);

	server->wlr_compositor = wlr_compositor_create(
		server->wl_display, server->wlr_renderer
	);
	assert(server->wlr_compositor);

	server->wlr_xdg_shell = wlr_xdg_shell_create(server->wl_display);
	assert(server->wlr_xdg_shell);

	server->wlr_layer_shell = wlr_layer_shell_v1_create(server->wl_display);
	assert(server->wlr_layer_shell);

	server->wlr_output_layout = wlr_output_layout_create();
	assert(server->wlr_output_layout);

	wlr_data_device_manager_create(server->wl_display);
	wlr_xdg_output_manager_v1_create(
		server->wl_display,
		server->wlr_output_layout
	);

	server->cursor = wkn_cursor_create(server);
	assert(server->cursor);
	wlr_cursor_attach_output_layout(
		server->cursor->wlr_cursor,
		server->wlr_output_layout
	);

	server->seat = wkn_seat_create(server, "seat0");
	assert(server->seat);

	wl_list_init(&server->outputs);
	server->new_output.notify = wkn_server_new_output_notify;
	wl_signal_add(
		&server->wlr_backend->events.new_output,
		&server->new_output
	);

	server->new_xdg_surface.notify = wkn_server_new_xdg_surface_notify;
	wl_signal_add(
		&server->wlr_xdg_shell->events.new_surface,
		&server->new_xdg_surface
	);
	server->new_layer_surface.notify = wkn_server_new_layer_surface_notify;
	wl_signal_add(
		&server->wlr_layer_shell->events.new_surface,
		&server->new_layer_surface
	);

	wl_list_init(&server->keyboards);
	server->new_input.notify = wkn_server_new_input_notify;
	wl_signal_add(
		&server->wlr_backend->events.new_input,
		&server->new_input
	);

	memset(
		server->key_states,
		WLR_KEY_RELEASED,
		sizeof(server->key_states)
	);
}

void wkn_server_destroy(struct wkn_server *server)
{
	if (!server)
		return;
	if (server->wl_display) {
		wl_display_destroy_clients(server->wl_display);
		wl_display_destroy(server->wl_display);
	}
	if (server->wlr_backend)
		wlr_backend_destroy(server->wlr_backend);
	if (server->logger)
		wkn_logger_destroy(server->logger);
	// if (server->wlr_compositor)
	// 	wlr_compositor_destroy(server->wlr_compositor);
	// if (server->wlr_xdg_shell)
	// 	wlr_xdg_shell_destroy(server->wlr_xdg_shell);
	if (server->wlr_output_layout)
		wlr_output_layout_destroy(server->wlr_output_layout);
	if (server->cursor)
		wkn_cursor_destroy(server->cursor);
	if (server->seat)
		wkn_seat_destroy(server->seat);
	if (!wl_list_empty(&server->outputs)) {
		struct wkn_output *output;
		struct wkn_output *tmp;
		wl_list_for_each_safe(output, tmp, &server->outputs, link) {
			wl_list_remove(&output->link);
			wkn_output_destroy(output);
		}
	}
	if (!wl_list_empty(&server->keyboards)) {
		struct wkn_keyboard *keyboard;
		wl_list_for_each(keyboard, &server->keyboards, link)
			wkn_keyboard_destroy(keyboard);
	}
	free(server);
}
