#include <assert.h>
#include <stdlib.h>
#define WLR_USE_UNSTABLE
#include <wlr/types/wlr_keyboard.h>
#include "xdg-surface.h"
#include "cursor.h"
#include "logger.h"

void wkn_xdg_surface_focus(struct wkn_xdg_surface *xdg_surface)
{
	if (!xdg_surface)
		return;

	struct wkn_server *server = xdg_surface->server;
	struct wkn_seat *seat = server->seat;
	struct wlr_surface *prev_wlr_surface =
		seat->wlr_seat->keyboard_state.focused_surface;
	if (prev_wlr_surface == xdg_surface->wlr_xdg_surface->surface)
		return;

	if (prev_wlr_surface)
		wlr_xdg_toplevel_set_activated(
			wlr_xdg_surface_from_wlr_surface(prev_wlr_surface),
			false
		);
	struct wlr_keyboard *wlr_keyboard = wlr_seat_get_keyboard(
		seat->wlr_seat
	);
	// You are focused! Go to top!
	wl_list_remove(&xdg_surface->link);
	wl_list_insert(&server->xdg_surfaces, &xdg_surface->link);
	wlr_xdg_toplevel_set_activated(xdg_surface->wlr_xdg_surface, true);
	wlr_seat_keyboard_notify_enter(
		seat->wlr_seat,
		xdg_surface->wlr_xdg_surface->surface,
		wlr_keyboard->keycodes,
		wlr_keyboard->num_keycodes,
		&wlr_keyboard->modifiers
	);
}

// This only happens when move or resize starts.
static void wkn_xdg_surface_request(
	struct wkn_xdg_surface *xdg_surface,
	enum wkn_cursor_state cursor_state,
	uint32_t edges
)
{
	struct wkn_server *server = xdg_surface->server;
	struct wkn_cursor *cursor = server->cursor;
	struct wlr_surface *focused_surface =
		server->seat->wlr_seat->pointer_state.focused_surface;
	if (xdg_surface->wlr_xdg_surface->surface != focused_surface)
		return;
	server->focused_xdg_surface = xdg_surface;
	server->cursor->state = cursor_state;
	switch (cursor_state) {
	case WKN_CURSOR_MOVE:
		server->request_cursor_x = cursor->wlr_cursor->x;
		server->request_cursor_y = cursor->wlr_cursor->y;
		server->request_rect.x = xdg_surface->rect.x;
		server->request_rect.y = xdg_surface->rect.y;
		break;
	case WKN_CURSOR_RESIZE:
		server->request_cursor_x = cursor->wlr_cursor->x;
		server->request_cursor_y = cursor->wlr_cursor->y;
		server->request_rect.x = xdg_surface->rect.x;
		server->request_rect.y = xdg_surface->rect.y;
		server->request_rect.w = xdg_surface->rect.w;
		server->request_rect.h = xdg_surface->rect.h;
		server->request_resize_edges = edges;
		break;
	default:
		break;
	}
}

static void wkn_xdg_surface_destroy_notify(struct wl_listener *listener, void *data)
{
	struct wkn_xdg_surface *xdg_surface = wl_container_of(
		listener,
		xdg_surface,
		destroy
	);
	wl_list_remove(&xdg_surface->link);
	wkn_xdg_surface_destroy(xdg_surface);
}

static void wkn_xdg_surface_request_move_notify(
	struct wl_listener *listener,
	void *data
)
{
	struct wkn_xdg_surface *xdg_surface = wl_container_of(
		listener,
		xdg_surface,
		request_move
	);
	// struct wlr_xdg_toplevel_move_event *e = data;
	wkn_xdg_surface_request(xdg_surface, WKN_CURSOR_MOVE, 0);
}

static void wkn_xdg_surface_request_resize_notify(
	struct wl_listener *listener,
	void *data
)
{
	struct wkn_xdg_surface *xdg_surface = wl_container_of(
		listener,
		xdg_surface,
		request_resize
	);
	struct wlr_xdg_toplevel_resize_event *event = data;
	wkn_xdg_surface_request(xdg_surface, WKN_CURSOR_RESIZE, event->edges);
}

static void wkn_xdg_surface_request_fullscreen_notify(
	struct wl_listener *listener,
	void *data
)
{
	struct wkn_xdg_surface *xdg_surface = wl_container_of(
		listener,
		xdg_surface,
		request_fullscreen
	);
	// struct wlr_xdg_toplevel_set_fullscreen_event *event = data;

	if (!xdg_surface->mapped)
		return;

	// TODO: Set fullscreen.
}

static void wkn_xdg_surface_request_maximize_notify(
	struct wl_listener *listener,
	void *data
)
{
	struct wkn_xdg_surface *xdg_surface = wl_container_of(
		listener,
		xdg_surface,
		request_maximize
	);
	struct wkn_server *server = xdg_surface->server;

	if (!xdg_surface->mapped)
		return;

	if (!xdg_surface->maximized) {
		// Going maximized, save state.
		wkn_logger_debug(server->logger, "Maximize\n");

		xdg_surface->prev_rect = xdg_surface->rect;

		struct wlr_output *wlr_output = wlr_output_layout_output_at(
			server->wlr_output_layout,
			xdg_surface->rect.x,
			xdg_surface->rect.y
		);
		struct wlr_box *output_rect = wlr_output_layout_get_box(
			server->wlr_output_layout,
			wlr_output
		);
		xdg_surface->rect.x = output_rect->x;
		xdg_surface->rect.y = output_rect->y;
		xdg_surface->rect.w = output_rect->width;
		xdg_surface->rect.h = output_rect->height;
		wlr_xdg_toplevel_set_size(
			xdg_surface->wlr_xdg_surface,
			xdg_surface->rect.w,
			xdg_surface->rect.h
		);
	} else {
		// Going unmaximized, load state.
		wkn_logger_debug(server->logger, "Unmaximize!\n");

		xdg_surface->rect = xdg_surface->prev_rect;
		wlr_xdg_toplevel_set_size(
			xdg_surface->wlr_xdg_surface,
			xdg_surface->rect.w,
			xdg_surface->rect.h
		);
	}

	xdg_surface->maximized = !xdg_surface->maximized;
	wlr_xdg_toplevel_set_maximized(
		xdg_surface->wlr_xdg_surface,
		xdg_surface->maximized
	);
}

static void wkn_xdg_surface_request_minimize_notify(
	struct wl_listener *listener,
	void *data
)
{
	struct wkn_xdg_surface *xdg_surface = wl_container_of(
		listener,
		xdg_surface,
		request_minimize
	);

	if (!xdg_surface->mapped)
		return;

	xdg_surface->minimized = !xdg_surface->minimized;
}

// static void wkn_xdg_surface_ack_configure_notify(
// 	struct wl_listener *listener,
// 	void *data
// )
// {
// 	struct wkn_xdg_surface *xdg_surface = wl_container_of(
// 		listener,
// 		xdg_surface,
// 		ack_configure
// 	);
// 	xdg_surface->rect.w = xdg_surface->wlr_xdg_surface->geometry.width;
// 	xdg_surface->rect.h = xdg_surface->wlr_xdg_surface->geometry.height;
// }

static void wkn_xdg_surface_map_notify(struct wl_listener *listener, void *data)
{
	struct wkn_xdg_surface *xdg_surface = wl_container_of(listener, xdg_surface, map);
	xdg_surface->mapped = true;

	xdg_surface->rect.x = xdg_surface->wlr_xdg_surface->geometry.x;
	xdg_surface->rect.y = xdg_surface->wlr_xdg_surface->geometry.y;
	xdg_surface->rect.w = xdg_surface->wlr_xdg_surface->geometry.width;
	xdg_surface->rect.h = xdg_surface->wlr_xdg_surface->geometry.height;

	xdg_surface->request_move.notify = wkn_xdg_surface_request_move_notify;
	wl_signal_add(
		&xdg_surface->wlr_xdg_toplevel->events.request_move,
		&xdg_surface->request_move
	);
	xdg_surface->request_resize.notify = wkn_xdg_surface_request_resize_notify;
	wl_signal_add(
		&xdg_surface->wlr_xdg_toplevel->events.request_resize,
		&xdg_surface->request_resize
	);
	xdg_surface->request_fullscreen.notify = wkn_xdg_surface_request_fullscreen_notify;
	wl_signal_add(
		&xdg_surface->wlr_xdg_toplevel->events.request_fullscreen,
		&xdg_surface->request_fullscreen
	);
	xdg_surface->request_maximize.notify = wkn_xdg_surface_request_maximize_notify;
	wl_signal_add(
		&xdg_surface->wlr_xdg_toplevel->events.request_maximize,
		&xdg_surface->request_maximize
	);
	xdg_surface->request_minimize.notify = wkn_xdg_surface_request_minimize_notify;
	wl_signal_add(
		&xdg_surface->wlr_xdg_toplevel->events.request_minimize,
		&xdg_surface->request_minimize
	);
	// xdg_surface->ack_configure.notify = wkn_xdg_surface_ack_configure_notify;
	// wl_signal_add(
	// 	&xdg_surface->wlr_xdg_surface->events.ack_configure,
	// 	&xdg_surface->ack_configure
	// );

	wkn_xdg_surface_focus(xdg_surface);
}

static void wkn_xdg_surface_unmap_notify(struct wl_listener *listener, void *data)
{
	struct wkn_xdg_surface *xdg_surface = wl_container_of(
		listener,
		xdg_surface,
		unmap
	);
	xdg_surface->mapped = false;

	wl_list_remove(&xdg_surface->request_move.link);
	wl_list_remove(&xdg_surface->request_resize.link);
	wl_list_remove(&xdg_surface->request_minimize.link);
	wl_list_remove(&xdg_surface->request_maximize.link);
	wl_list_remove(&xdg_surface->request_fullscreen.link);
	// wl_list_remove(&xdg_surface->ack_configure.link);
}

struct wkn_xdg_surface *wkn_xdg_surface_create(
	struct wkn_server *server,
	struct wlr_xdg_surface *wlr_xdg_surface
)
{
	struct wkn_xdg_surface *xdg_surface = malloc(sizeof(*xdg_surface));
	assert(xdg_surface);
	xdg_surface->server = server;
	xdg_surface->wlr_xdg_surface = wlr_xdg_surface;
	xdg_surface->wlr_xdg_toplevel = wlr_xdg_surface->toplevel;
	xdg_surface->rect.x = xdg_surface->wlr_xdg_surface->geometry.x;
	xdg_surface->rect.y = xdg_surface->wlr_xdg_surface->geometry.y;
	xdg_surface->rect.w = xdg_surface->wlr_xdg_surface->geometry.width;
	xdg_surface->rect.h = xdg_surface->wlr_xdg_surface->geometry.height;
	xdg_surface->mapped = false;
	xdg_surface->minimized = false;
	xdg_surface->maximized = false;

	xdg_surface->map.notify = wkn_xdg_surface_map_notify;
	wl_signal_add(&wlr_xdg_surface->events.map, &xdg_surface->map);
	xdg_surface->unmap.notify = wkn_xdg_surface_unmap_notify;
	wl_signal_add(&wlr_xdg_surface->events.unmap, &xdg_surface->unmap);
	xdg_surface->destroy.notify = wkn_xdg_surface_destroy_notify;
	wl_signal_add(&wlr_xdg_surface->events.destroy, &xdg_surface->destroy);

	return xdg_surface;
}

void wkn_xdg_surface_destroy(struct wkn_xdg_surface *xdg_surface)
{
	if (!xdg_surface)
		return;
	if (xdg_surface->wlr_xdg_surface)
		wlr_xdg_toplevel_send_close(xdg_surface->wlr_xdg_surface);
	free(xdg_surface);
}
