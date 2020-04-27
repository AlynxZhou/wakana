#ifndef __XDG_SURFACE_H__
#define __XDG_SURFACE_H__

#include <wayland-server.h>
#include <wayland-util.h>
#include <stdbool.h>
#define WLR_USE_UNSTABLE
#include <wlr/types/wlr_xdg_shell.h>
#include "wakana.h"
#include "server.h"

struct wkn_xdg_surface {
	struct wkn_server *server;
	struct wkn_output *output;
	struct wlr_xdg_surface *wlr_xdg_surface;
	struct wlr_xdg_toplevel *wlr_xdg_toplevel;
	struct wl_listener map;
	struct wl_listener unmap;
	struct wl_listener destroy;

	struct wl_listener request_move;
	struct wl_listener request_resize;
	struct wl_listener request_fullscreen;
	struct wl_listener request_maximize;
	struct wl_listener request_minimize;
	struct wl_listener commit;

	bool mapped;
	bool minimized;
	bool maximized;
	struct wkn_rect rect;
	struct wkn_rect prev_rect;
	struct wl_list link;
};

struct wkn_xdg_surface *wkn_xdg_surface_create(
	struct wkn_server *server,
	struct wlr_xdg_surface *wlr_xdg_surface
);
void wkn_xdg_surface_focus(struct wkn_xdg_surface *xdg_surface);
void wkn_xdg_surface_destroy(struct wkn_xdg_surface *xdg_surface);

#endif
