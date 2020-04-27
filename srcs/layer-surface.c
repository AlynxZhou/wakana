#include <assert.h>
#include <stdlib.h>
#include "layer-surface.h"

void wkn_layer_surface_unmap_notify(struct wl_listener *listener, void *data)
{
	struct wkn_layer_surface *layer_surface = wl_container_of(
		listener,
		layer_surface,
		unmap
	);
	layer_surface->mapped = false;
}

void wkn_layer_surface_map_notify(struct wl_listener *listener, void *data)
{
	struct wkn_layer_surface *layer_surface = wl_container_of(
		listener,
		layer_surface,
		map
	);
	layer_surface->mapped = true;
}

void wkn_layer_surface_destroy_notify(struct wl_listener *listener, void *data)
{
	struct wkn_layer_surface *layer_surface = wl_container_of(
		listener,
		layer_surface,
		destroy
	);
	wl_list_remove(&layer_surface->link);
	wl_list_remove(&layer_surface->destroy.link);
	wl_list_remove(&layer_surface->map.link);
	wl_list_remove(&layer_surface->unmap.link);
	if (layer_surface->output) {
		wkn_output_arrange_layer_surfaces(
			layer_surface->output,
			layer_surface->wlr_layer_surface->current.layer
		);
	}
	wkn_layer_surface_destroy(layer_surface);
}

struct wkn_layer_surface *wkn_layer_surface_create(
	struct wkn_server *server,
	struct wlr_layer_surface_v1 *wlr_layer_surface
)
{
	struct wkn_layer_surface *layer_surface = malloc(
		sizeof(*layer_surface)
	);
	assert(layer_surface);
	layer_surface->server = server;
	layer_surface->wlr_layer_surface = wlr_layer_surface;

	layer_surface->destroy.notify = wkn_layer_surface_destroy_notify;
	wl_signal_add(
		&wlr_layer_surface->events.destroy,
		&layer_surface->destroy
	);
	layer_surface->map.notify = wkn_layer_surface_map_notify;
	wl_signal_add(&wlr_layer_surface->events.map, &layer_surface->map);
	layer_surface->unmap.notify = wkn_layer_surface_unmap_notify;
	wl_signal_add(&wlr_layer_surface->events.unmap, &layer_surface->unmap);
	return layer_surface;
}

void wkn_layer_surface_destroy(struct wkn_layer_surface *layer_surface)
{
	if (!layer_surface)
		return;
	if (layer_surface->wlr_layer_surface)
		wlr_layer_surface_v1_close(layer_surface->wlr_layer_surface);
	free(layer_surface);
}
