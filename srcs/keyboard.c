#include <assert.h>
#include <stdlib.h>
#include "keyboard.h"

struct wkn_keyboard *wkn_keyboard_create(
	struct wkn_server *server,
	struct wlr_input_device *wlr_input_device
)
{
	struct wkn_keyboard *keyboard = malloc(sizeof(*keyboard));
	assert(keyboard);
	keyboard->server = server;
	keyboard->wlr_input_device = wlr_input_device;
	// TODO
	return keyboard;
}
