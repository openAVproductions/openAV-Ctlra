
#include "global.h"

#include <string.h>

#include "ctlra/ctlra.h"
#include "ctlra/devices/ni_kontrol_d2.h"

/* for drawing graphics to screen */
#include <cairo/cairo.h>

static cairo_surface_t *surface = 0;
static cairo_t *cr = 0;

#define WIDTH  480
#define HEIGHT 272

void d2_screen_init()
{
	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					      WIDTH, HEIGHT);
	if(!surface)
		printf("!surface\n");
	cr = cairo_create (surface);
	if(!cr)
		printf("!cr\n");
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

void d2_screen_draw(struct ctlra_dev_t *dev, struct dummy_data *d)
{
	if(surface == 0)
		d2_screen_init();

	/* blank background */
	cairo_set_source_rgb(cr, 0, 0, 0);
	cairo_rectangle(cr, 0, 0, 480, 272);
	cairo_fill(cr);

	/* draw on surface */
	cairo_set_source_rgb(cr, 255, 0, 0);
	cairo_rectangle(cr, d->progress * (480-10), 20, 40, 40);
	cairo_fill(cr);

	cairo_set_source_rgb(cr, 0, 255, 0);
	cairo_rectangle(cr, d->progress * (480-10), 60, 40, 40);
	cairo_fill(cr);

	cairo_set_source_rgb(cr, 0, 0, 255);
	cairo_rectangle(cr, d->progress * (480-10),100, 40, 40);
	cairo_fill(cr);

	cairo_surface_flush(surface);

	/* Calculate stride / pixel copy */
	int stride = cairo_image_surface_get_stride(surface);
	unsigned char * data = cairo_image_surface_get_data(surface);
	if(!data)
		printf("error data == 0\n");
	uint32_t data_idx = 0;

	uint8_t *pixels = ni_kontrol_d2_screen_get_pixels(dev);
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

	ni_kontrol_d2_screen_blit(dev);
}

void kontrol_d2_update_state(struct ctlra_dev_t *dev, struct dummy_data *d)
{
	uint32_t i;

	for(i = 0; i < VEGAS_BTN_COUNT; i++)
		ctlra_dev_light_set(dev, i, UINT32_MAX * d->buttons[i]);

	uint8_t orange[25] = {0};
	uint8_t blue[25] = {0};

	for(i = 0; i < d->progress * 25; i++) {
		blue[i] = 0xff;
	}
	orange[i] = 0xff;
	ni_kontrol_d2_light_touchstrip(dev, orange, blue);

	/* flush LED feedback first (most important) */
	ctlra_dev_light_flush(dev, 1);

	/* Redraw + flush screen */
	d2_screen_draw(dev, d);
	return;
}

void kontrol_d2_func_remap(struct ctlra_dev_t* dev,
			   uint32_t num_events,
			   struct ctlra_event_t** events,
			   void *userdata)
{
	//printf("%s handling events\n", __func__);
	// slider move returns to previous map
	if(events[0]->type == CTLRA_EVENT_SLIDER) {
		printf("returning to previous map\n");
		ctlra_dev_set_event_func(dev, kontrol_d2_func);
	}
}

void kontrol_d2_func(struct ctlra_dev_t* dev,
		     uint32_t num_events,
		     struct ctlra_event_t** events,
		     void *userdata)
{
	struct dummy_data *d = (void *)userdata;
	(void)dev;
	(void)userdata;
	for(uint32_t i = 0; i < num_events; i++) {
		char *pressed = 0;
		struct ctlra_event_t *e = events[i];
		int p = 0;
		switch(e->type) {
		case CTLRA_EVENT_BUTTON:
			p = e->button.pressed;
			d->buttons[e->button.id] = e->button.pressed;
			//printf("btn %d : %d\n", e->button.id, p);
			if(e->button.id == 3) {
				memset(&d->buttons, e->button.pressed,
				       sizeof(d->buttons));
			}
			if(e->button.id == NI_KONTROL_D2_BTN_DECK && p) {
				printf("deck pressed, switching handle func\n");
				ctlra_dev_set_event_func(dev, kontrol_d2_func_remap);
			}
			break;

		case CTLRA_EVENT_ENCODER:
			break;

		case CTLRA_EVENT_SLIDER:
			d->volume = e->slider.value;
			if(e->slider.id == 5) {
				uint32_t iter = (int)((d->volume+0.05) * 7.f);
				for(i = 0; i < iter; i++) {
					ctlra_dev_light_set(dev, 1 + i, UINT32_MAX);
					ctlra_dev_light_set(dev, 8 + i, UINT32_MAX);
				}
				for(; i < 7.0; i++) {
					ctlra_dev_light_set(dev, 1 + i, 0);
					ctlra_dev_light_set(dev, 8 + i, 0);
				}
			}
			break;

		case CTLRA_EVENT_GRID:
			break;
		default:
			break;
		};
	}
	d->revision++;
}

