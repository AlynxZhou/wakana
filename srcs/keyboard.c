#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#define WLR_USE_UNSTABLE
#include <wlr/types/wlr_seat.h>
#include "keyboard.h"

// #define toXkbcommonKeycode(x) ((x) + 8)

static void wkn_keyboard_destroy_notify(
	struct wl_listener *listener,
	void *data
)
{
	struct wkn_keyboard *keyboard = wl_container_of(
		listener,
		keyboard,
		destroy
	);
	wl_list_remove(&keyboard->link);
	wkn_keyboard_destroy(keyboard);
}

static void wkn_keyboard_modifiers_notify(
	struct wl_listener *listener,
	void *data
)
{
	struct wkn_keyboard *keyboard = wl_container_of(
		listener,
		keyboard,
		modifiers
	);
	struct wkn_server *server = keyboard->server;
	struct wkn_seat *seat = server->seat;
	wlr_seat_set_keyboard(seat->wlr_seat, keyboard->wlr_input_device);
	wlr_seat_keyboard_notify_modifiers(
		seat->wlr_seat,
		&keyboard->wlr_keyboard->modifiers
	);
}

static void wkn_keyboard_key_notify(struct wl_listener *listener, void *data)
{
	struct wkn_keyboard *keyboard = wl_container_of(
		listener,
		keyboard,
		key
	);
	struct wkn_server *server = keyboard->server;
	struct wlr_event_keyboard_key *event = data;
	struct wkn_seat *seat = server->seat;

	wkn_server_update_keys(server, event->keycode, event->state);
	bool server_handled = wkn_server_handle_keybindings(server);
	if (!server_handled) {
		wlr_seat_set_keyboard(
			seat->wlr_seat,
			keyboard->wlr_input_device
		);
		wlr_seat_keyboard_notify_key(
			seat->wlr_seat,
			event->time_msec,
			event->keycode,
			event->state
		);
	}
}

struct wkn_keyboard *wkn_keyboard_create(
	struct wkn_server *server,
	struct wlr_input_device *wlr_input_device
)
{
	struct wkn_keyboard *keyboard = malloc(sizeof(*keyboard));
	assert(keyboard);
	keyboard->server = server;
	keyboard->wlr_input_device = wlr_input_device;
	keyboard->wlr_keyboard = wlr_input_device->keyboard;

	struct xkb_rule_names rules = {0};
	struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	struct xkb_keymap *keymap = xkb_map_new_from_names(
		context,
		&rules,
		XKB_KEYMAP_COMPILE_NO_FLAGS
	);
	wlr_keyboard_set_keymap(keyboard->wlr_keyboard, keymap);
	xkb_keymap_unref(keymap);
	xkb_context_unref(context);

	keyboard->modifiers.notify = wkn_keyboard_modifiers_notify;
	wl_signal_add(
		&keyboard->wlr_keyboard->events.modifiers,
		&keyboard->modifiers
	);
	keyboard->key.notify = wkn_keyboard_key_notify;
	wl_signal_add(&keyboard->wlr_keyboard->events.key, &keyboard->key);
	// TODO: enable when wlroots has new releases.
	// keyboard->destroy.notify = wkn_keyboard_destroy_notify;
	// wl_signal_add(
	// 	&keyboard->wlr_keyboard->events.destroy,
	// 	&keyboard->destroy
	// );

	wlr_keyboard_set_repeat_info(
		keyboard->wlr_keyboard,
		KEYBOARD_REPEAT_RATE,
		KEYBOARD_REPEAT_DELAY
	);
	return keyboard;
}

void wkn_keyboard_destroy(struct wkn_keyboard *keyboard)
{
	if (!keyboard)
		return;
	free(keyboard);
}
