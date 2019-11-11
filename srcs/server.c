#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#define WLR_USE_UNSTABLE
#include <wlr/types/wlr_input_device.h>
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
		wlr_cursor_attach_input_device(server->cursor->wlr_cursor, device);
		break;
	}
	case WLR_INPUT_DEVICE_KEYBOARD: {
		struct wkn_keyboard *keyboard = wkn_keyboard_create(server, device);
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

static void wkn_server_new_surface_notify(
	struct wl_listener *listener,
	void *data
)
{
	struct wkn_server *server = wl_container_of(listener, server, new_surface);
	struct wlr_xdg_surface *wlr_xdg_surface = data;

	if (wlr_xdg_surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
		return;
	}

	struct wkn_client *client = wkn_client_create(server, wlr_xdg_surface);
	wl_list_insert(&server->clients, &client->link);
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
		// Get first output mode here.
		struct wlr_output_mode *mode = wl_container_of(
			wlr_output->modes.prev, mode, link
		);
		// I personally think using wl_list_for_each and break is also enough.
		// But wl_list_for_each internally call wl_container_of.
		wlr_output_set_mode(wlr_output, mode);
	}

	struct wkn_output *output = wkn_output_create(server, wlr_output);
	wl_list_insert(&server->outputs, &output->link);
	wlr_output_layout_add_auto(server->wlr_output_layout, wlr_output);
	wlr_output_create_global(wlr_output);
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

	server->wlr_renderer = wlr_backend_get_renderer(server->wlr_backend);
	assert(server->wlr_renderer);
	wlr_renderer_init_wl_display(server->wlr_renderer, server->wl_display);

	server->wlr_compositor = wlr_compositor_create(
		server->wl_display, server->wlr_renderer
	);
	assert(server->wlr_compositor);

	wlr_data_device_manager_create(server->wl_display);

	server->wlr_xdg_shell = wlr_xdg_shell_create(server->wl_display);
	assert(server->wlr_xdg_shell);

	server->wlr_output_layout = wlr_output_layout_create();
	assert(server->wlr_output_layout);

	server->cursor = wkn_cursor_create(server);
	assert(server->cursor);
	wlr_cursor_attach_output_layout(server->cursor->wlr_cursor, server->wlr_output_layout);

	server->seat = wkn_seat_create(server, "seat0");
	assert(server->seat);

	wl_list_init(&server->outputs);
	server->new_output.notify = wkn_server_new_output_notify;
	wl_signal_add(&server->wlr_backend->events.new_output, &server->new_output);

	wl_list_init(&server->clients);
	server->new_surface.notify = wkn_server_new_surface_notify;
	wl_signal_add(&server->wlr_xdg_shell->events.new_surface, &server->new_surface);

	wl_list_init(&server->keyboards);
	server->new_input.notify = wkn_server_new_input_notify;
	wl_signal_add(&server->wlr_backend->events.new_input, &server->new_input);

	memset(server->key_states, WLR_KEY_RELEASED, sizeof(server->key_states));

	return server;
}

void wkn_server_move_focused_client(struct wkn_server *server)
{
	struct wkn_client *client = server->focused_client;
	struct wkn_cursor *cursor = server->cursor;
	double dx = server->request_cursor_x - cursor->wlr_cursor->x;
	double dy = server->request_cursor_y - cursor->wlr_cursor->y;
	client->x = server->request_client_x - dx;
	client->y = server->request_client_y - dy;
}

void wkn_server_resize_focused_client(struct wkn_server *server)
{
	struct wkn_client *client = server->focused_client;
	struct wkn_cursor *cursor = server->cursor;
	double dx = server->request_cursor_x - cursor->wlr_cursor->x;
	double dy = server->request_cursor_y - cursor->wlr_cursor->y;
	double x;
	double y;
	double width;
	double height;
	if (server->request_resize_edges & WLR_EDGE_LEFT) {
		x = server->request_client_x - dx;
		width = server->request_client_width + dx;
	} else if (server->request_resize_edges & WLR_EDGE_RIGHT) {
		x = server->request_client_x;
		width = server->request_client_width - dx;
	} else {
		x = server->request_client_x;
		width = server->request_client_width;
	}
	if (server->request_resize_edges & WLR_EDGE_TOP) {
		y = server->request_client_y - dy;
		height = server->request_client_height + dy;
	} else if (server->request_resize_edges & WLR_EDGE_BOTTOM) {
		y = server->request_client_y;
		height = server->request_client_height - dy;
	} else {
		y = server->request_client_y;
		height = server->request_client_height;
	}
	wlr_xdg_toplevel_set_size(client->wlr_xdg_surface, width, height);
	client->x = x;
	client->y = y;
}

void wkn_server_update_keys(
	struct wkn_server *server,
	uint32_t keycode,
	enum wlr_key_state state
)
{
	printf("KEY: %d, STATE: %d\n", keycode, state);
	if (keycode >= KEYCODE_NUMS)
		return;
	server->key_states[keycode] = state;
}

bool wkn_server_handle_keybindings(struct wkn_server *server)
{
	if (server->key_states[KEY_LEFTCTRL] == WLR_KEY_PRESSED &&
	    server->key_states[KEY_LEFTALT] == WLR_KEY_PRESSED &&
	    server->key_states[KEY_BACKSPACE] == WLR_KEY_PRESSED) {
		printf("Restart!\n");
		// TODO: Restart session.
		return true;
	}
	if ((server->key_states[KEY_LEFTMETA] == WLR_KEY_PRESSED &&
	     server->key_states[KEY_ESC] == WLR_KEY_PRESSED) ||
	    (server->key_states[KEY_LEFTCTRL] == WLR_KEY_PRESSED &&
	     server->key_states[KEY_LEFTALT] == WLR_KEY_PRESSED &&
	     server->key_states[KEY_ESC] == WLR_KEY_PRESSED)) {
		printf("Exit!\n");
		// TODO: better exit.
		// wl_display_terminate(server->wl_display);
		// wkn_server_destroy(server);
		exit(EXIT_SUCCESS);
		return true;
	}
	return false;
}

struct wkn_client *wkn_server_find_client_at(
	struct wkn_server *server,
	double layout_x,
	double layout_y
)
{
	struct wkn_client *client;
	wl_list_for_each(client, &server->clients, link) {
		double client_relative_x = layout_x - client->x;
		double client_relative_y = layout_y - client->y;
		// Ugly api needs this...I don't need them.
		// I have submitted a PR to prevent wlr_xdg_surface_surface_at to assign it when passing NULL.
		// It has been merged.
		// TODO: Use NULL when wlroots updated.
		double _sx;
		double _sy;
		struct wlr_surface *wlr_surface = wlr_xdg_surface_surface_at(client->wlr_xdg_surface, client_relative_x, client_relative_y, &_sx, &_sy);
		if (wlr_surface)
			return client;
	}
	return NULL;
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
	if (server->wlr_compositor)
		wlr_compositor_destroy(server->wlr_compositor);
	if (server->wlr_xdg_shell)
		wlr_xdg_shell_destroy(server->wlr_xdg_shell);
	if (server->wlr_output_layout)
		wlr_output_layout_destroy(server->wlr_output_layout);
	if (server->cursor)
		wkn_cursor_destroy(server->cursor);
	if (server->seat)
		wkn_seat_destroy(server->seat);
	if (!wl_list_empty(&server->outputs)) {
		struct wkn_output *output;
		wl_list_for_each(output, &server->outputs, link)
			wkn_output_destroy(output);
	}
	if (!wl_list_empty(&server->clients)) {
		struct wkn_client *client;
		wl_list_for_each(client, &server->clients, link)
			wkn_client_destroy(client);
	}
	if (!wl_list_empty(&server->keyboards)) {
		struct wkn_keyboard *keyboard;
		wl_list_for_each(keyboard, &server->keyboards, link)
			wkn_keyboard_destroy(keyboard);
	}
	free(server);
}
