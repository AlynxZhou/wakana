#include <assert.h>
#include <stdlib.h>
#include "keyboard.h"

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
	// TODO
}

static void wkn_keyboard_key_notify(struct wkn_keyboard *keyboard, void *data)
{
	struct wkn_keyboard *keyboard = wl_container_of(
		listener,
		keyboard,
		key
	);
	struct wkn_server *server = keyboard->server;
	// TODO
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
	keyboard->wlr_keyboard = wlr_input_device->wlr_keyboard;

	struct xkb_rule_names rules = {0};
	struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	struct xkb_keymap *keymap = xkb_map_new_from_names(
		context,
		&rules,
		XKB_KEYMAP_COMPILE_NO_FLAGS
	);
	wlr_keyboard_set_keymap(keyboard->wlr_keyboard, keymap);
	xkb_keymap_unref(keymap);
	xkb_content_unref(context);

	keyboard->modifiers.notify = wkn_keyboard_modifiers_notify;
	wl_signal_add(
		&keyboard->wlr_keyboard->events.modifiers,
		&keyboard->modifiers
	);
	keyboard->key.notify = wkn_keyboard_key_notify;
	wl_signal_add(&keyboard->wlr_keyboard->events.key, &keyboard->key);

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
