#ifndef __OUTPUT_H__
#define __OUTPUT_H__

#include <wayland-server.h>
#include <wayland-util.h>
#define WLR_USE_UNSTABLE
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_damage.h>
#include "wakana.h"
#include "server.h"
#include "xdg-surface.h"
#include "layer-surface.h"

#define LAYER_NUMBER 4

struct wkn_output {
	struct wkn_server *server;
	struct wlr_output *wlr_output;
	struct wlr_output_damage *damage;
	struct wl_listener destroy;
	struct wl_listener frame;
	struct wl_listener damage_frame;
	struct wl_list layer_surfaces[LAYER_NUMBER];
	// wl_list requires this for item.
	struct wl_list link;
};

void wkn_output_arrange_layer_surfaces(
	struct wkn_output *output,
	enum zwlr_layer_shell_v1_layer layer
);
struct wkn_output *wkn_output_create(
	struct wkn_server *server,
	struct wlr_output *wlr_output
);
void wkn_output_destroy(struct wkn_output *output);

#endif
