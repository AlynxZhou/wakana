#ifndef __SERVER_H__
#define __SERVER_H__

#include <linux/input-event-codes.h>
// Support first 128 keys may be enough.
#define KEYCODE_NUMS 128
#include <wayland-server.h>
#include <wayland-util.h>
#define WLR_USE_UNSTABLE
#include <wlr/backend.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include "wakana.h"
#include "client.h"
#include "layer-surface.h"
#include "cursor.h"
#include "seat.h"

struct wkn_server {
	struct wl_display *wl_display;
	struct wl_event_loop *wl_event_loop;
	struct wlr_backend *wlr_backend;
	struct wlr_renderer *wlr_renderer;
	struct wlr_compositor *wlr_compositor;
	struct wlr_xdg_shell *wlr_xdg_shell;
	struct wlr_layer_shell_v1 *wlr_layer_shell;
	struct wlr_output_layout *wlr_output_layout;
	struct wkn_cursor *cursor;
	struct wkn_seat *seat;
	struct wkn_client *focused_client;
	double request_cursor_x;
	double request_cursor_y;
	struct wkn_rect request_rect;
	uint32_t request_resize_edges;
	struct wl_listener new_output;
	struct wl_list outputs;
	struct wl_listener new_xdg_surface;
	struct wl_listener new_layer_surface;
	struct wl_list clients;
	struct wl_list layers;
	struct wl_listener new_input;
	struct wl_list keyboards;
	void *png_data;
	uint32_t png_width;
	uint32_t png_height;
	enum wlr_key_state key_states[KEYCODE_NUMS];
};

struct wkn_server *wkn_server_create(void);
void wkn_server_move_focused_client(struct wkn_server *server);
void wkn_server_resize_focused_client(struct wkn_server *server);
void wkn_server_update_keys(
	struct wkn_server *server,
	uint32_t keycode,
	enum wlr_key_state state
);
bool wkn_server_handle_keybindings(struct wkn_server *server);
struct wkn_client *wkn_server_find_client_at(
	struct wkn_server *server,
	double layout_x,
	double layout_y
);
void wkn_server_destroy(struct wkn_server *server);

#endif
