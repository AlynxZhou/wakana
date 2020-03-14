#include <assert.h>
#include <stdlib.h>
#include "cursor.h"

static void wkn_cursor_process_motion(struct wkn_cursor *cursor, uint32_t time)
{
	struct wkn_server *server = cursor->server;
	struct wkn_seat *seat = server->seat;
	if (cursor->state == WKN_CURSOR_MOVE) {
		wkn_server_move_focused_client(server);
		return;
	} else if (cursor->state == WKN_CURSOR_RESIZE) {
		wkn_server_resize_focused_client(server);
		return;
	}
	struct wkn_client *client = wkn_server_find_client_at(
		server, cursor->wlr_cursor->x, cursor->wlr_cursor->y
	);
	if (!client) {
		wlr_xcursor_manager_set_cursor_image(
			cursor->wlr_xcursor_manager,
			"left_ptr",
			cursor->wlr_cursor
		);
		wlr_seat_pointer_clear_focus(seat->wlr_seat);
		return;
	}
	double client_relative_x = cursor->wlr_cursor->x - client->rect.x;
	double client_relative_y = cursor->wlr_cursor->y - client->rect.y;
	double sub_x;
	double sub_y;
	struct wlr_surface *wlr_surface = wlr_xdg_surface_surface_at(
		client->wlr_xdg_surface,
		client_relative_x, client_relative_y,
		&sub_x, &sub_y
	);
	if (wlr_surface) {
		wlr_seat_pointer_notify_enter(
			seat->wlr_seat,
			wlr_surface,
			sub_x, sub_y
		);
		if (seat->wlr_seat->pointer_state.focused_surface == wlr_surface)
			wlr_seat_pointer_notify_motion(
				seat->wlr_seat,
				time, sub_x, sub_y
			);
	} else {
		wlr_seat_pointer_clear_focus(seat->wlr_seat);
	}
}

static void wkn_cursor_motion_notify(struct wl_listener *listener, void *data)
{
	struct wkn_cursor *cursor = wl_container_of(listener, cursor, motion);
	// struct wkn_server *server = cursor->server;
	struct wlr_event_pointer_motion *event = data;

	wlr_cursor_move(cursor->wlr_cursor, event->device, event->delta_x, event->delta_y);
	wkn_cursor_process_motion(cursor, event->time_msec);
}

static void wkn_cursor_motion_absolute_notify(struct wl_listener *listener, void *data)
{
	struct wkn_cursor *cursor = wl_container_of(listener, cursor, motion_absolute);
	// struct wkn_server *server = cursor->server;
	struct wlr_event_pointer_motion_absolute *event = data;

	wlr_cursor_warp_absolute(cursor->wlr_cursor, event->device, event->x, event->y);
	wkn_cursor_process_motion(cursor, event->time_msec);
}

static void wkn_cursor_button_notify(struct wl_listener *listener, void *data)
{
	struct wkn_cursor *cursor = wl_container_of(listener, cursor, button);
	struct wkn_server *server = cursor->server;
	struct wlr_event_pointer_button *event = data;

	wlr_seat_pointer_notify_button(server->seat->wlr_seat, event->time_msec, event->button, event->state);
	// double sx;
	// double sy;
	// struct wlr_seat *wlr_seat = server->seat->wlr_seat;
	struct wkn_client *client = wkn_server_find_client_at(
		server,
		cursor->wlr_cursor->x,
		cursor->wlr_cursor->y
	);

	if (event->state == WLR_BUTTON_RELEASED)
		cursor->state = WKN_CURSOR_PASSTHROUGH;
	else
		wkn_client_focus(client);
}

static void wkn_cursor_axis_notify(struct wl_listener *listener, void *data)
{
	struct wkn_cursor *cursor = wl_container_of(listener, cursor, axis);
	struct wkn_server *server = cursor->server;
	struct wlr_event_pointer_axis *event = data;

	wlr_seat_pointer_notify_axis(
		server->seat->wlr_seat,
		event->time_msec,
		event->orientation,
		event->delta,
		event->delta_discrete,
		event->source
	);
}

static void wkn_cursor_frame_notify(struct wl_listener *listener, void *data)
{
	struct wkn_cursor *cursor = wl_container_of(listener, cursor, frame);
	struct wkn_server *server = cursor->server;
	// struct wlr_event_pointer_frame *event = data;
	wlr_seat_pointer_notify_frame(server->seat->wlr_seat);
}

struct wkn_cursor *wkn_cursor_create(struct wkn_server *server)
{
	struct wkn_cursor *cursor = malloc(sizeof(*cursor));
	assert(cursor);
	cursor->state = WKN_CURSOR_PASSTHROUGH;
	cursor->server = server;
	cursor->wlr_cursor = wlr_cursor_create();
	assert(cursor->wlr_cursor);
	cursor->wlr_xcursor_manager = wlr_xcursor_manager_create(NULL, 24);
	assert(cursor->wlr_xcursor_manager);
	wlr_xcursor_manager_load(cursor->wlr_xcursor_manager, 1);

	cursor->motion.notify = wkn_cursor_motion_notify;
	wl_signal_add(&cursor->wlr_cursor->events.motion, &cursor->motion);
	cursor->motion_absolute.notify = wkn_cursor_motion_absolute_notify;
	wl_signal_add(&cursor->wlr_cursor->events.motion_absolute, &cursor->motion_absolute);
	cursor->button.notify = wkn_cursor_button_notify;
	wl_signal_add(&cursor->wlr_cursor->events.button, &cursor->button);
	cursor->axis.notify = wkn_cursor_axis_notify;
	wl_signal_add(&cursor->wlr_cursor->events.axis, &cursor->axis);
	cursor->frame.notify = wkn_cursor_frame_notify;
	wl_signal_add(&cursor->wlr_cursor->events.frame, &cursor->frame);

	return cursor;
}

void wkn_cursor_destroy(struct wkn_cursor *cursor)
{
	if (!cursor)
		return;
	if (cursor->wlr_cursor)
		wlr_cursor_destroy(cursor->wlr_cursor);
	if (cursor->wlr_xcursor_manager)
		wlr_xcursor_manager_destroy(cursor->wlr_xcursor_manager);
	free(cursor);
}
