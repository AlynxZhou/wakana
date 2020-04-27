#ifndef __SEAT_H__
#define __SEAT_H__

#include <wayland-server.h>
#include <wayland-util.h>
#define WLR_USE_UNSTABLE
#include <wlr/types/wlr_seat.h>
#include "server.h"

struct wkn_seat {
	struct wkn_server *server;
	struct wlr_seat *wlr_seat;
	struct wl_listener request_set_cursor;
	struct wl_list keyboards;
};

struct wkn_seat *wkn_seat_create(struct wkn_server *server, char *name);
void wkn_seat_destroy(struct wkn_seat *seat);

#endif
