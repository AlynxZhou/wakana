#include <assert.h>
#include <stdlib.h>
#define WLR_USE_UNSTABLE
#include <wlr/types/wlr_keyboard.h>
#include "client.h"
#include "cursor.h"
#include "logger.h"

void wkn_client_focus(struct wkn_client *client)
{
	if (!client)
		return;
	struct wkn_server *server = client->server;
	struct wkn_output *output = client->output;
	struct wkn_seat *seat = server->seat;
	struct wlr_surface *prev_wlr_surface = seat->wlr_seat->keyboard_state.focused_surface;
	if (prev_wlr_surface == client->wlr_xdg_surface->surface)
		return;
	if (prev_wlr_surface)
		wlr_xdg_toplevel_set_activated(
			wlr_xdg_surface_from_wlr_surface(prev_wlr_surface),
			false
		);
	struct wlr_keyboard *wlr_keyboard = wlr_seat_get_keyboard(seat->wlr_seat);
	// You are focused! Go to top!
	wl_list_remove(&client->link);
	wl_list_insert(&output->clients, &client->link);
	wlr_xdg_toplevel_set_activated(client->wlr_xdg_surface, true);
	wlr_seat_keyboard_notify_enter(
		seat->wlr_seat,
		client->wlr_xdg_surface->surface,
		wlr_keyboard->keycodes,
		wlr_keyboard->num_keycodes,
		&wlr_keyboard->modifiers
	);
}

// This only happens when move or resize starts.
static void wkn_client_request(
	struct wkn_client *client,
	enum wkn_cursor_state cursor_state,
	uint32_t edges
)
{
	struct wkn_server *server = client->server;
	struct wkn_cursor *cursor = server->cursor;
	struct wlr_surface *focused_surface = server->seat->wlr_seat->pointer_state.focused_surface;
	if (client->wlr_xdg_surface->surface != focused_surface)
		return;
	server->focused_client = client;
	server->cursor->state = cursor_state;
	struct wlr_box geo_box;
	wlr_xdg_surface_get_geometry(client->wlr_xdg_surface, &geo_box);
	switch (cursor_state) {
	case WKN_CURSOR_MOVE:
		server->request_cursor_x = cursor->wlr_cursor->x;
		server->request_cursor_y = cursor->wlr_cursor->y;
		server->request_rect.x = client->rect.x;
		server->request_rect.y = client->rect.y;
		break;
	case WKN_CURSOR_RESIZE:
		server->request_cursor_x = cursor->wlr_cursor->x;
		server->request_cursor_y = cursor->wlr_cursor->y;
		server->request_rect.x = client->rect.x;
		server->request_rect.y = client->rect.y;
		server->request_rect.w = geo_box.width;
		server->request_rect.h = geo_box.height;
		server->request_resize_edges = edges;
		break;
	default:
		break;
	}
}

static void wkn_client_map_notify(struct wl_listener *listener, void *data)
{
	struct wkn_client *client = wl_container_of(listener, client, map);
	client->mapped = true;
	wkn_client_focus(client);
}

static void wkn_client_unmap_notify(struct wl_listener *listener, void *data)
{
	struct wkn_client *client = wl_container_of(listener, client, unmap);
	client->mapped = false;
}

static void wkn_client_destroy_notify(struct wl_listener *listener, void *data)
{
	struct wkn_client *client = wl_container_of(listener, client, destroy);
	wl_list_remove(&client->link);
	wkn_client_destroy(client);
}

static void wkn_client_request_move_notify(
	struct wl_listener *listener,
	void *data
)
{
	struct wkn_client *client = wl_container_of(
		listener,
		client,
		request_move
	);
	wkn_client_request(client, WKN_CURSOR_MOVE, 0);
}

static void wkn_client_request_resize_notify(
	struct wl_listener *listener,
	void *data
)
{
	struct wkn_client *client = wl_container_of(
		listener,
		client,
		request_resize
	);
	struct wlr_xdg_toplevel_resize_event *event = data;
	wkn_client_request(client, WKN_CURSOR_RESIZE, event->edges);
}

static void wkn_client_request_fullscreen_notify(
	struct wl_listener *listener,
	void *data
)
{
	struct wkn_client *client = wl_container_of(
		listener,
		client,
		request_fullscreen
	);
	// struct wlr_xdg_toplevel_set_fullscreen_event *event = data;

	if (!client->mapped)
		return;

	// TODO: Set fullscreen.
}

static void wkn_client_request_maximize_notify(
	struct wl_listener *listener,
	void *data
)
{
	struct wkn_client *client = wl_container_of(
		listener,
		client,
		request_maximize
	);
	struct wkn_output *output = client->output;

	if (!client->mapped)
		return;


	if (!client->maximized) {
		// Going maximized, save state.
		wkn_logger_debug("Maximize\n");

		client->prev_rect = client->rect;

		int w;
		int h;
		wlr_output_effective_resolution(output->wlr_output, &w, &h);

		client->rect.x = 0;
		client->rect.y = 0;
		client->rect.w = w;
		client->rect.h = h;
		wlr_xdg_toplevel_set_size(client->wlr_xdg_surface, w, h);
	} else {
		// Going unmaximized, load state.
		wkn_logger_debug("Unmaximize!\n");

		client->rect = client->prev_rect;
		wlr_xdg_toplevel_set_size(client->wlr_xdg_surface, client->rect.w, client->rect.h);
	}

	client->maximized = !client->maximized;
	wlr_xdg_toplevel_set_maximized(client->wlr_xdg_surface, client->maximized);
}

static void wkn_client_request_minimize_notify(
	struct wl_listener *listener,
	void *data
)
{
	struct wkn_client *client = wl_container_of(
		listener,
		client,
		request_minimize
	);

	if (!client->mapped)
		return;

	client->minimized = !client->minimized;
}

struct wkn_client *wkn_client_create(
	struct wkn_server *server,
	struct wlr_xdg_surface *wlr_xdg_surface
)
{
	struct wkn_client *client = malloc(sizeof(*client));
	assert(client);
	client->server = server;
	client->wlr_xdg_surface = wlr_xdg_surface;
	client->wlr_xdg_toplevel = wlr_xdg_surface->toplevel;
	client->rect.x = 0;
	client->rect.y = 0;
	client->rect.w = client->wlr_xdg_toplevel->current.width;
	client->rect.h = client->wlr_xdg_toplevel->current.height;
	client->mapped = true;
	client->minimized = false;
	client->maximized = false;

	client->map.notify = wkn_client_map_notify;
	wl_signal_add(&wlr_xdg_surface->events.map, &client->map);
	client->unmap.notify = wkn_client_unmap_notify;
	wl_signal_add(&wlr_xdg_surface->events.unmap, &client->unmap);
	client->destroy.notify = wkn_client_destroy_notify;
	wl_signal_add(&wlr_xdg_surface->events.destroy, &client->destroy);

	client->request_move.notify = wkn_client_request_move_notify;
	wl_signal_add(&client->wlr_xdg_toplevel->events.request_move, &client->request_move);
	client->request_resize.notify = wkn_client_request_resize_notify;
	wl_signal_add(&client->wlr_xdg_toplevel->events.request_resize, &client->request_resize);
	client->request_fullscreen.notify = wkn_client_request_fullscreen_notify;
	wl_signal_add(&client->wlr_xdg_toplevel->events.request_fullscreen, &client->request_fullscreen);
	client->request_maximize.notify = wkn_client_request_maximize_notify;
	wl_signal_add(&client->wlr_xdg_toplevel->events.request_maximize, &client->request_maximize);
	client->request_minimize.notify = wkn_client_request_minimize_notify;
	wl_signal_add(&client->wlr_xdg_toplevel->events.request_minimize, &client->request_minimize);
	return client;
}

void wkn_client_destroy(struct wkn_client *client)
{
	if (!client)
		return;
	free(client);
}
