#ifndef __OUTPUT_H__
#define __OUTPUT_H__

#include <wayland-server.h>
#include <wayland-util.h>
#define WLR_USE_UNSTABLE
#include <wlr/types/wlr_output.h>
#include "server.h"

struct wkn_output {
	struct wkn_server *server;
	struct wlr_output *wlr_output;
	struct wl_listener destroy;
	struct wl_listener frame;
	// wl_list requires this for item.
	struct wl_list link;
};

struct wkn_output *wkn_output_create(struct wkn_server *server, struct wlr_output *wlr_output);
void wkn_output_destroy(struct wkn_output *output);

#endif
