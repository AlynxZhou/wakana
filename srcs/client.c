#include <assert.h>
#include <stdlib.h>
#define WLR_USE_UNSTABLE
#include <wlr/types/wlr_keyboard.h>
#include "client.h"
#include "cursor.h"

void wkn_client_focus(struct wkn_client *client)
{
	if (!client)
		return;
	struct wkn_server *server = client->server;
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
	wl_list_insert(&server->clients, &client->link);
	wlr_xdg_toplevel_set_activated(client->wlr_xdg_surface, true);
	wlr_seat_keyboard_notify_enter(seat->wlr_seat, client->wlr_xdg_surface->surface, wlr_keyboard->keycodes, wlr_keyboard->num_keycodes, &wlr_keyboard->modifiers);
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
		server->request_client_x = client->x;
		server->request_client_y = client->y;
		break;
	case WKN_CURSOR_RESIZE:
		server->request_cursor_x = cursor->wlr_cursor->x;
		server->request_cursor_y = cursor->wlr_cursor->y;
		server->request_client_x = client->x;
		server->request_client_y = client->y;
		server->request_client_width = geo_box.width;
		server->request_client_height = geo_box.height;
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

struct wkn_client *wkn_client_create(
	struct wkn_server *server,
	struct wlr_xdg_surface *wlr_xdg_surface
)
{
	struct wkn_client *client = malloc(sizeof(*client));
	assert(client);
	client->server = server;
	client->wlr_xdg_surface = wlr_xdg_surface;
	client->x = 0;
	client->y = 0;
	client->mapped = true;

	client->map.notify = wkn_client_map_notify;
	wl_signal_add(&wlr_xdg_surface->events.map, &client->map);
	client->unmap.notify = wkn_client_unmap_notify;
	wl_signal_add(&wlr_xdg_surface->events.unmap, &client->unmap);
	client->destroy.notify = wkn_client_destroy_notify;
	wl_signal_add(&wlr_xdg_surface->events.destroy, &client->destroy);

	client->wlr_xdg_toplevel = wlr_xdg_surface->toplevel;
	client->request_move.notify = wkn_client_request_move_notify;
	wl_signal_add(&client->wlr_xdg_toplevel->events.request_move, &client->request_move);
	client->request_resize.notify = wkn_client_request_resize_notify;
	wl_signal_add(&client->wlr_xdg_toplevel->events.request_resize, &client->request_resize);
	return client;
}

void wkn_client_destroy(struct wkn_client *client)
{
	if (!client)
		return;
	free(client);
}
