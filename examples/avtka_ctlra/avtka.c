#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include <cairo/cairo.h>

#include "ctlra.h"
#include "avtka.h"

static volatile uint32_t done;
static uint32_t led;
static uint32_t led_set;

struct avtka_t *a;
static uint32_t revision;

static uint32_t slider_id;

struct pad_t {
	uint32_t item_id;
	uint8_t pressed;
};
static struct pad_t pads[16];

void simple_feedback_func(struct ctlra_dev_t *dev, void *d)
{
	for(int i = 0; i < 45; i++)
		ctlra_dev_light_set(dev, i, 0xffffffff);
	ctlra_dev_light_flush(dev, 1);
}

static inline
void pixel_convert_from_argb(int r, int g, int b, uint8_t *data)
{
	r = ((int)((r / 255.0) * 31)) & ((1<<5)-1);
	g = ((int)((g / 255.0) * 63)) & ((1<<6)-1);
	b = ((int)((b / 255.0) * 31)) & ((1<<5)-1);

	uint16_t combined = (b | g << 5 | r << 11);
	data[0] = combined >> 8;
	data[1] = combined & 0xff;
}

void convert_scalar(unsigned char *data, uint8_t *pixels, uint32_t stride)
{
	const uint16_t HEIGHT = 272;
	const uint16_t WIDTH = 480;
	uint16_t *write_head = (uint16_t*)pixels;
	/* Copy the Cairo pixels to the usb buffer, taking the
	 * stride of the cairo memory into account, converting from
	 * RGB into the BGR that the screen expects */
	for(int j = 0; j < HEIGHT; j++) {
		for(int i = 0; i < WIDTH; i++) {
			uint8_t *p = &data[(j * stride) + (i*4)];
			int idx = (j * WIDTH) + (i);
			pixel_convert_from_argb(p[2], p[1], p[0],
						(uint8_t*)&write_head[idx]);
		}
	}
}

int32_t screen_redraw_cb(struct ctlra_dev_t *dev, uint32_t screen_idx,
			 uint8_t *pixel_data, uint32_t bytes,
			 struct ctlra_screen_zone_t *zone,
			 void *userdata)
{
	static int redraw;
	if(redraw == revision) {
		return 0;
	}

	redraw = revision;

	cairo_surface_t *surf = avtka_get_cairo_surface(a);
	if(!surf) {
		printf("error getting surface!!\n");
		exit(-2);
	}

	unsigned char * data = cairo_image_surface_get_data(surf);
	if(!data) {
		printf("error data == 0\n");
		exit(-1);
	}
	int stride = cairo_image_surface_get_stride(surf);

	/* convert from ARGB32 to 565 */
	convert_scalar(data, pixel_data, stride);

	/* fill in redraw co-ords */
	avtka_redraw_get_damaged_area(a, &zone->x, &zone->y,
				         &zone->w, &zone->h);

	return 1;
}

void simple_event_func(struct ctlra_dev_t* dev, uint32_t num_events,
		       struct ctlra_event_t** events, void *userdata)
{
	/* Events from the Ctlra device are handled here. They should be
	 * decoded, and events sent to the application. Note that this
	 * function must *NOT* send feedback to the device. Instead, the
	 * feedback_func() above should be used to send feedback. For a
	 * proper demo on how to do feedback, see examples/vegas/
	 */

	static const char* grid_pressed[] = { "   ", " X " };

	/* Retrieve info, so we can look up names. Note this is not
	 * expected to be used in production code - the names should be
	 * cached in the UI, and not retrieved on every event */
	struct ctlra_dev_info_t info;
	ctlra_dev_get_info(dev, &info);

	int redraw = 0;

	for(uint32_t i = 0; i < num_events; i++) {
		struct ctlra_event_t *e = events[i];
		const char *pressed = 0;
		const char *name = 0;
		switch(e->type) {
		case CTLRA_EVENT_BUTTON: {
			name = ctlra_info_get_name(&info, CTLRA_EVENT_BUTTON,
						   e->button.id);
			char pressure[16] = {0};
			if(e->button.has_pressure) {
				snprintf(pressure, sizeof(pressure),
					 "%0.01f", e->button.pressure);
			}
			printf("[%s] button %s (%d)\n",
				e->button.has_pressure ?  pressure :
				(e->button.pressed ? " X " : "   "),
				name, e->button.id);
			} break;

		case CTLRA_EVENT_ENCODER:
			name = ctlra_info_get_name(&info, CTLRA_EVENT_ENCODER,
						   e->encoder.id);
			printf("[%s] encoder %s (%d)\n",
			       e->encoder.delta > 0 ? " ->" : "<- ",
			       name, e->button.id);
			avtka_item_value_inc(a, 1, e->encoder.delta_float * 1.5);
			//avtka_item_value_inc(a, 2, e->encoder.delta_float);
			redraw = 1;
			revision++;
			break;

		case CTLRA_EVENT_SLIDER:
			name = ctlra_info_get_name(&info, CTLRA_EVENT_SLIDER,
						   e->slider.id);
			printf("[%03d] slider %s (%d)\n",
			       (int)(e->slider.value * 100.f),
			       name, e->slider.id);
			avtka_item_value(a, slider_id, e->slider.value);
			redraw = 1;
			revision++;
			break;

		case CTLRA_EVENT_GRID:
			name = ctlra_info_get_name(&info, CTLRA_EVENT_GRID,
			                                  e->grid.id);
			if(e->grid.flags & CTLRA_EVENT_GRID_FLAG_BUTTON) {
				pressed = grid_pressed[e->grid.pressed];
			} else {
				pressed = "---";
			}
			printf("[%s] grid %d", pressed, e->grid.pos);
			if(e->grid.flags & CTLRA_EVENT_GRID_FLAG_PRESSURE)
				printf(", pressure %1.3f", e->grid.pressure);
			printf("\n");
			avtka_item_value(a, pads[e->grid.pos].item_id,
					 e->grid.pressed);
			redraw = 1;
			revision++;

			break;
		default:
			break;
		};
	}

	if(redraw)
		avtka_redraw(a);

}

void sighndlr(int signal)
{
	done = 1;
	printf("\n");
}

void simple_remove_func(struct ctlra_dev_t *dev, int unexpected_removal,
			void *userdata)
{
	/* Notifies application of device removal, also allows cleanup
	 * of any device specific structures. See daemon/ example, where
	 * the MIDI I/O is cleaned up in the remove() function */
	struct ctlra_dev_info_t info;
	ctlra_dev_get_info(dev, &info);
	printf("simple: removing %s %s\n", info.vendor, info.device);
}


int accept_dev_func(struct ctlra_t *ctlra,
		    const struct ctlra_dev_info_t *info,
		    struct ctlra_dev_t *dev,
                    void *userdata)
{
	printf("avtka: accepting %s %s\n", info->vendor, info->device);

	/* here we use the Ctlra APIs to set callback functions to get
	 * events and send feedback updates to/from the device */
	ctlra_dev_set_event_func(dev, simple_event_func);
	ctlra_dev_set_feedback_func(dev, simple_feedback_func);
	ctlra_dev_set_screen_feedback_func(dev, screen_redraw_cb);
	ctlra_dev_set_remove_func(dev, simple_remove_func);
	ctlra_dev_set_callback_userdata(dev, userdata);

	return 1;
}

void event_cb(struct avtka_t *avtka, uint32_t item, float v, void *userdata)
{
	//printf("event on item %d, value %f\n", item, v);
	//avtka_item_value(a, 2, v);
	revision++;
}

int main(int argc, char **argv)
{
	if(argc > 1) {
		led = atoi(argv[1]);
		led_set = 1;
	}

	signal(SIGINT, sighndlr);

	/* initialize avtka UI for ctlra screen */
	struct avtka_opts_t opts = {
		.w = 480,
		.h = 272,
		.resizeable = 1,
		.event_callback = event_cb,
		.event_callback_userdata = 0,
		.offscreen_only = 1,
#if 0
		.debug_redraws = 1,
#endif
	};
	a = avtka_create("Avtka Ctlra Screen", &opts);
	if(!a) {
		printf("failed to create avtka instance, quitting\n");
		return -1;
	}

	struct avtka_item_opts_t item = {
		.name = "Dial 1",
		.x = 120, .y = 10, .w = 70, .h = 70,
		.draw = AVTKA_DRAW_DIAL,
		.interact = AVTKA_INTERACT_DRAG_V,
	};
	int id = avtka_item_create(a, &item);
	(void)id; // keep gcc7 happy

	item.x = 10;
	item.y = 10;
	item.w = 100;
	item.h =  80;
	item.draw = AVTKA_DRAW_FILTER;
	item.interact = AVTKA_INTERACT_DRAG_V;
	snprintf(item.name, sizeof(item.name), "Filter");
	id = avtka_item_create(a, &item);

	item.x = 210;
	item.y =  10;
	item.w =  20;
	item.h =  80;
	item.draw = AVTKA_DRAW_SLIDER;
	item.interact = AVTKA_INTERACT_DRAG_V;
	snprintf(item.name, sizeof(item.name), "TS");
	slider_id = avtka_item_create(a, &item);

	for(int i = 0; i < 16; i++) {
		struct avtka_item_opts_t item = {
			.name = "Pad",
			.x = 120 + ((i%4)) * 30,
			.y = 180 + -(3-(i/4)) * 30,
			.w = 39, .h = 39,
			.draw = AVTKA_DRAW_DIAL,
			.interact = AVTKA_INTERACT_TOGGLE,
			.params[0] = 1,
		};
		pads[i].item_id = avtka_item_create(a, &item);
		avtka_item_value(a, pads[i].item_id, 0);
	}

	struct ctlra_t *ctlra = ctlra_create(NULL);
	int num_devs = ctlra_probe(ctlra, accept_dev_func, 0x0);
	printf("connected devices %d\n", num_devs);

	avtka_visible(a, 0);

	while(!done) {
		ctlra_idle_iter(ctlra);
		avtka_iterate(a);
		usleep(10);
	}

	ctlra_exit(ctlra);

	return 0;
}
