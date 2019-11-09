#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#include <wayland-util.h>
#include <wayland-server.h>
#define WLR_USE_UNSTABLE
#include <wlr/types/wlr_input_device.h>
#include "server.h"

struct wkn_keyboard {
	struct wkn_server *server;
	struct wlr_input_device *wlr_input_device;
	struct wl_listener modifiers;
	struct wl_listener key;
	struct wl_list link;
};

struct wkn_keyboard *wkn_keyboard_create(
	struct wkn_server *server,
	struct wlr_input_device *wlr_input_device
);

#endif
