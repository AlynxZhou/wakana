#include <assert.h>
#include <stdlib.h>
#define WLR_USE_UNSTABLE
#include <wlr/types/wlr_surface.h>
#include <wlr/types/wlr_matrix.h>
#include "output.h"

static void wkn_output_destroy_notify(struct wl_listener *listener, void *data)
{
	struct wkn_output *output = wl_container_of(listener, output, destroy);
	wl_list_remove(&output->link);
	wl_list_remove(&output->destroy.link);
	wl_list_remove(&output->frame.link);
	wkn_output_destroy(output);
}

struct render_data {
	struct wkn_server *server;
	struct wkn_output *output;
	struct wkn_client *client;
	struct timespec *when;
};
// A callback.
static void wkn_output_render_surface(
	struct wlr_surface *wlr_surface,
	int surface_x,
	int surface_y,
	void *data
)
{
	struct render_data *rdata = data;
	struct wkn_server *server = rdata->server;
	struct wkn_output *output = rdata->output;
	struct wkn_client *client = rdata->client;

	struct wlr_texture *wlr_texture = wlr_surface_get_texture(wlr_surface);
	if (!wlr_texture) {
		return;
	}

	double layout_x;
	double layout_y;
	wlr_output_layout_output_coords(server->wlr_output_layout, output->wlr_output, &layout_x, &layout_y);

	// Not a real hidpi support, just a dummy implemention.
	struct wlr_box render_box = {
		.x = (layout_x + client->x + surface_x) * output->wlr_output->scale,
		.y = (layout_y + client->y + surface_y) * output->wlr_output->scale,
		.width = wlr_surface->current.width * output->wlr_output->scale,
		.height = wlr_surface->current.height * output->wlr_output->scale
	};
	float matrix[9];
	wlr_matrix_project_box(
		matrix,
		&render_box,
		wlr_output_transform_invert(wlr_surface->current.transform),
		0,
		output->wlr_output->transform_matrix
	);
	wlr_render_texture_with_matrix(
		server->wlr_renderer,
		wlr_texture,
		matrix,
		1.0f
	);
	wlr_surface_send_frame_done(wlr_surface, rdata->when);
}

static void wkn_output_frame_notify(struct wl_listener *listener, void *data)
{
	struct wkn_output *output = wl_container_of(listener, output, frame);
	struct wkn_server *server = output->server;
	struct wlr_output *wlr_output = output->wlr_output;
	struct wlr_renderer *wlr_renderer = server->wlr_renderer;

	if (!wlr_output_attach_render(wlr_output, NULL))
		return;

	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	int width;
	int height;
	wlr_output_effective_resolution(wlr_output, &width, &height);
	wlr_renderer_begin(wlr_renderer, width, height);
	float color[4] = {0.9, 0.9f, 0.9f, 1.0f};
	wlr_renderer_clear(wlr_renderer, color);

	struct wkn_client *client;
	wl_list_for_each_reverse(client, &server->clients, link) {
		if (!client->mapped) {
			continue;
		}
		struct render_data rdata = {
			.server = server,
			.output = output,
			.client = client,
			.when = &now
		};
		wlr_xdg_surface_for_each_surface(
			client->wlr_xdg_surface,
			wkn_output_render_surface,
			&rdata
		);
	}

	wlr_renderer_end(wlr_renderer);
	wlr_output_commit(wlr_output);
}

struct wkn_output *wkn_output_create(
	struct wkn_server *server,
	struct wlr_output *wlr_output
)
{
	struct wkn_output *output = malloc(sizeof(*output));
	assert(output);
	output->server = server;
	output->wlr_output = wlr_output;
	output->destroy.notify = wkn_output_destroy_notify;
	wl_signal_add(&wlr_output->events.destroy, &output->destroy);
	output->frame.notify = wkn_output_frame_notify;
	wl_signal_add(&wlr_output->events.frame, &output->frame);
	return output;
}

void wkn_output_destroy(struct wkn_output *output)
{
	if (!output)
		return;
	free(output);
}
