#ifndef __LAYER_POPUP_H__
#define __LAYER_POPUP_H__

#include <wayland-server.h>
#define WLR_USE_UNSTABLE
#include <wlr/types/wlr_layer_shell_v1.h>
#include "server.h"
#include "layer-surface.h"

enum wkn_layer_type {
	WKN_LAYER_TYPE_SURFACE;
	WKN_LAYER_TYPE_POPUP;
};

// I am still confusing why I need to handle popup
// manually in layer_shell, I can just ignore popup
// in xdg_shell, though.
// Better to email wlroots' author.
struct wkn_layer_popup {
	struct wkn_server *server;
	struct wlr_xdg_output *wlr_xdg_output;
	enum wkn_layer_type parent_type;
	struct wkn_layer_surface *parent_surface;
	struct wkn_layer_popup *parent_popup;
	struct wl_listener map;
	struct wl_listener unmap;
	struct wl_listener destroy;
	struct wl_listener commit;
	struct wl_listener new_popup;
};

#endif