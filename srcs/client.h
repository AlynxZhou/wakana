#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <wayland-server.h>
#include <wayland-util.h>
#include <stdbool.h>
#define WLR_USE_UNSTABLE
#include <wlr/types/wlr_xdg_shell.h>
#include "wakana.h"
#include "server.h"

struct wkn_client {
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
	bool mapped;
	bool minimized;
	bool maximized;
	struct wkn_rect rect;
	struct wkn_rect prev_rect;
	struct wl_list link;
};

struct wkn_client *wkn_client_create(
	struct wkn_server *server,
	struct wlr_xdg_surface *wlr_xdg_surface
);
void wkn_client_focus(struct wkn_client *client);
void wkn_client_destroy(struct wkn_client *client);

#endif
