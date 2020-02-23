#ifndef __LAYER_SURFACE_H__
#define __LAYER_SURFACE_H__

#include <wayland-server.h>
#define WLR_USE_UNSTABLE
#include <wlr/types/wlr_layer_shell_v1.h>
#include "server.h"
#include "output.h"

struct wkn_layer_surface {
	struct wkn_server *server;
	struct wkn_output *output;
	struct wlr_layer_surface_v1 *wlr_layer_surface;
	struct wl_listener map;
	struct wl_listener unmap;
	struct wl_listener commit;
	struct wl_listener output_destroy;
	struct wl_listener new_popup;
	struct wl_listener destroy;
	bool mapped;
	double x;
	double y;
	struct wl_list link;
};

struct wkn_layer_surface *wkn_layer_surface_create(
	struct wkn_server *server,
	struct wlr_layer_surface_v1 *wlr_layer_surface
);
void wkn_layer_surface_destroy(struct wkn_layer_surface *layer_surface);

#endif