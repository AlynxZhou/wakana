#ifndef __CURSOR_H__
#define __CURSOR_H__

#include <wayland-server.h>
#include <wayland-util.h>
#define WLR_USE_UNSTABLE
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include "server.h"

enum wkn_cursor_state {
	// Passthrough: just moving the cursor.
	WKN_CURSOR_PASSTHROUGH,
	// Move: click and drag a window with the cursor.
	WKN_CURSOR_MOVE,
	// Resize: click and resize a window with the cursor.
	WKN_CURSOR_RESIZE
};

struct wkn_cursor {
	struct wkn_server *server;
	struct wlr_cursor *wlr_cursor;
	struct wlr_xcursor_manager *wlr_xcursor_manager;
	struct wl_listener motion;
	struct wl_listener motion_absolute;
	struct wl_listener button;
	struct wl_listener axis;
	struct wl_listener frame;
	enum wkn_cursor_state state;
};

struct wkn_cursor *wkn_cursor_create(struct wkn_server *server);
void wkn_cursor_destroy(struct wkn_cursor *cursor);

#endif
