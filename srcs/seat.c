#include <assert.h>
#include <stdlib.h>
#include "seat.h"

static void wkn_seat_request_set_cursor_notify(
	struct wl_listener *listener,
	void *data
)
{
	struct wkn_seat *seat = wl_container_of(listener, seat, request_set_cursor);
	struct wkn_server *server = seat->server;
	struct wlr_seat_pointer_request_set_cursor_event *event = data;
	struct wlr_seat_client *focused_client = seat->wlr_seat->pointer_state.focused_client;
	if (focused_client == event->seat_client) {
		wlr_cursor_set_surface(server->cursor->wlr_cursor, event->surface, event->hotspot_x, event->hotspot_y);
	}
}

struct wkn_seat *wkn_seat_create(struct wkn_server *server, const char name[])
{
	struct wkn_seat *seat = malloc(sizeof(*seat));
	assert(seat);
	seat->server = server;
	seat->wlr_seat = wlr_seat_create(server->wl_display, name);
	assert(seat->wlr_seat);
	seat->request_set_cursor.notify = wkn_seat_request_set_cursor_notify;
	wl_signal_add(&seat->wlr_seat->events.request_set_cursor, &seat->request_set_cursor);
	return seat;
}

void wkn_seat_destroy(struct wkn_seat *seat)
{
	if (!seat)
		return;
	if (seat->wlr_seat)
		wlr_seat_destroy(seat->wlr_seat);
	free(seat);
}
