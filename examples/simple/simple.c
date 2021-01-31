#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "ctlra.h"

/* TODO: move to NI Screens.h? */
static const uint8_t header_left[] = {
	0x84, 0x00, 0x00, 0x60,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x01, 0xe0, 0x01, 0x10,
};
static const uint8_t header_right[] = {
	0x84, 0x00, 0x01, 0x60,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x01, 0xe0, 0x01, 0x10,
};
static const uint8_t footer_left[] = {
	0x03, 0x00, 0x00, 0x00,
	0x40, 0x00, 0x00, 0x00
};
/* Notice the 2nd last byte is 0x01 for the right screen */
static const uint8_t footer_right[] = {
	0x03, 0x00, 0x00, 0x00,
	0x40, 0x00, 0x01, 0x00,
};


static const uint8_t var_px_command[] = {
	/* num_px/2: 0xff00 is the (total_px/2) */
	0x00, 0x0, 0xff, 0x00,
};

struct ni_screen_blit_t {
	uint8_t cmd;
	uint8_t zero;
	uint16_t pixels_div_two;
	uint16_t pixel_data[0];
};
/** Blit pixels to the screen.
 * Note that pixels must be a multiple of two!
 * Once done, the application must fill in blit
 */
static inline uint32_t
ni_screen_blit(struct ni_screen_blit_t *blit, uint16_t pixels)
{
	uint16_t px_div2 = (pixels >> 1);
	*blit = (struct ni_screen_blit_t) {
		.cmd = 0x0,
		.pixels_div_two = (px_div2 >> 8) | (px_div2 << 8),
	};
	/* Application now has pixel-data[0..(pixels-1)] to fill in. */
	return sizeof(struct ni_screen_blit_t) + (sizeof(uint16_t) * pixels);
}

struct ni_screen_line_t {
	uint8_t cmd;
	uint8_t zero;
	uint16_t length_div_two;
	uint16_t pixel_a;
	uint16_t pixel_b;
};
static inline uint32_t
ni_screen_line(void *data, uint16_t len, uint16_t px_a, uint16_t px_b)
{
	/* Zero lenght lines are not allowed, they freeze the screen. Returning
	 * zero here makes the next command overwrite, so its transparent to
	 * the caller.
	 */
	if (len == 0) {
		return 0;
	}
	uint16_t len_div_two = (len >> 1);
	struct ni_screen_line_t *line = data;
	*line = (struct ni_screen_line_t) {
		.cmd = 0x1,
		.length_div_two = (len_div_two >> 8) | (len_div_two << 8),
		.pixel_a = (px_a >> 8) | (px_a << 8),
		.pixel_b = (px_b >> 8) | (px_b << 8),
	};
	return sizeof(struct ni_screen_line_t);
}

struct ni_screen_skip_t {
	uint8_t cmd;
	uint8_t zero;
	uint16_t length_div_two;
};
static inline uint32_t
ni_screen_skip(void *data, uint16_t len)
{
	uint16_t len_div_two = (len >> 1);
	struct ni_screen_skip_t *skip = data;
	*skip = (struct ni_screen_skip_t) {
		.cmd = 0x2,
		.length_div_two = (len_div_two >> 8) | (len_div_two << 8),
	};
	return sizeof(struct ni_screen_skip_t);
}

uint32_t
ni_screen_audio_meter(void *data, float audio_level)
{
	uint32_t margin = 20;
	uint32_t width = 480 - (margin*2);

	uint32_t ridx = 0;
	ridx += ni_screen_skip(&data[ridx], margin);
	ridx += ni_screen_line(&data[ridx], width, 0xffff, 0xffff);
	ridx += ni_screen_skip(&data[ridx], margin * 2);

	uint16_t audio_col = 0b11111100000;
	if (audio_level > 1.0f) {
		audio_level = 1.0f;
	}
	float a_to_col = (audio_level - 0.7f) * 20;
	audio_col = audio_col << (int)(a_to_col);

	uint32_t audio_len = ((uint32_t)(width * audio_level)) & (~1);
	uint32_t blank_len = width - audio_len;
	uint32_t skip_len  = margin * 2;

	for (int i = 0; i < 10; i++) {
		ridx += ni_screen_line(&data[ridx], audio_len, audio_col, audio_col);
		ridx += ni_screen_line(&data[ridx], blank_len, 0, 0);
		ridx += ni_screen_skip(&data[ridx], skip_len);
	}

	ridx += ni_screen_line(&data[ridx], width, 0xffff, 0xffff);
	ridx += ni_screen_skip(&data[ridx], margin);
	return ridx;
}

static volatile uint32_t done;
static uint32_t led;
static uint32_t led_set;
static int screens_enabled = 1;
static int redraw_screens = 1;

void simple_feedback_func(struct ctlra_dev_t *dev, void *d)
{
	/* feedback like LEDs and Screen drawing based on application
	 * state goes here. See the vegas_mode/ example for an example of
	 * how to turn on LEDs on the controllers.
	 *
	 * Alternatively re-run the simple example with an integer
	 * parameter and that light number will be lit: ./simple 5
	 */
	if(led_set) {
		ctlra_dev_light_set(dev, led, 0xffffffff);
		ctlra_dev_light_flush(dev, 1);
	}
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
			}
			if(e->button.pressed) {
				/* any button event to repain screen */
				redraw_screens = 1;
			}
			break;


		case CTLRA_EVENT_ENCODER:
			name = ctlra_info_get_name(&info, CTLRA_EVENT_ENCODER,
						   e->encoder.id);
			printf("[%s] encoder %s (%d) type = %s\n",
			       e->encoder.delta > 0 ? " ->" : "<- ",
			       name, e->encoder.id,
			       e->encoder.flags & CTLRA_EVENT_ENCODER_FLAG_FLOAT ?
					"smooth" : "notched");
			break;

		case CTLRA_EVENT_SLIDER:
			name = ctlra_info_get_name(&info, CTLRA_EVENT_SLIDER,
						   e->slider.id);
			printf("[%03d] slider %s (%d)\n",
			       (int)(e->slider.value * 100.f),
			       name, e->slider.id);
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
			break;
		default:
			break;
		};
	}
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

int32_t simple_screen_redraw_func(struct ctlra_dev_t *dev,
				  uint32_t screen_idx,
				  uint8_t *pixel_data,
				  uint32_t bytes,
				  struct ctlra_screen_zone_t *redraw_zone,
				  void *userdata)
{
	if (!screens_enabled)
		return 0;

	/* Draw images to pixel_data here. Note that the exact data-format
	 * for the specific device is not abstracted from the application today
	 * resulting in this callback giving raw access to the device's I/O
	 * pixel format. For many devices this is RGB565 or BGR565, for 2 bytes
	 * per pixel.
	 *
	 * It is likely that your UI toolkit of choice has routines to render
	 * or draw to a buffer, and later retrieve those pixels. The pixel
	 * format can be converted to the hardware pixel format here, and then
	 * pushed to the device.
	 *
	 * It may seem a sub-optimal solution from a performance perspective
	 * to have to convert between buffers, but in practice drawing in any
	 * complex format such as RGB565 is way more CPU intensive than drawing
	 * in RGB888 or RGBA8888.
	 *
	 * A number of lightweight and embeddable libraries exist for drawing
	 * pixels, but if unsure of what to use, the Cairo library is
	 * recommended for starting as it is widely used and packaged.
	 */

	printf("in %s, screen idx %d\n", __func__, screen_idx);

	/* Fill with some sort of data. Please note that this is *expected* to
	 * show random colors on the screen. Its just to demo it works - the
	 * application must figure out how to draw useful stuff.
	 */
	uint16_t col1 = rand();

	uint8_t *header = header_left;
	uint8_t *footer = footer_left;
	if (screen_idx == 1) {
		header = header_right;
		footer = footer_right;
	}

	uint32_t px_done = 0;
	uint32_t raw_idx = 0;
	uint8_t dev_spec_raw_data[1024 * 16] = {0};
	memcpy(&dev_spec_raw_data[raw_idx], header, 16);
	raw_idx += 16;

#if 0
	uint16_t red = ((1 << 5) - 1) << 11;
	uint16_t grn = ((1 << 6) - 1) << 5;
	uint16_t blu = ((1 << 5) - 1);

	for (int i = 0; i < 20; i++) {
		raw_idx += ni_screen_skip(&dev_spec_raw_data[raw_idx], 30);
		raw_idx += ni_screen_line(&dev_spec_raw_data[raw_idx], 210, red, red);
		raw_idx += ni_screen_line(&dev_spec_raw_data[raw_idx], 240, blu, blu);
		px_done += (30 + 210 + 240);
	}
#endif

	/* Skip to half way */
	uint32_t px_screen_half = (480 * 272)/2;
	uint32_t px_to_skip = px_screen_half - px_done;
	raw_idx += ni_screen_skip(&dev_spec_raw_data[raw_idx], px_to_skip);
	//raw_idx += ni_screen_line(&dev_spec_raw_data[raw_idx], px_screen_half, 0,0);

#if 0
	uint32_t blit_px = 480 * 8;
	struct ni_screen_blit_t *blit = &dev_spec_raw_data[raw_idx];
	raw_idx += ni_screen_blit(blit, blit_px);
	for (uint32_t i = 0; i < blit_px; i++) {
		blit->pixel_data[i] = rand();
	}
#endif

	float r = rand() / (float)RAND_MAX * 0.25;
#if 0
	raw_idx += ni_screen_skip(&dev_spec_raw_data[raw_idx], 480*10);
	raw_idx += ni_screen_audio_meter(&dev_spec_raw_data[raw_idx], 0.1f);

	raw_idx += ni_screen_skip(&dev_spec_raw_data[raw_idx], 480*10);
	raw_idx += ni_screen_audio_meter(&dev_spec_raw_data[raw_idx], 0.4f + r);

	raw_idx += ni_screen_skip(&dev_spec_raw_data[raw_idx], 480*10);
	raw_idx += ni_screen_audio_meter(&dev_spec_raw_data[raw_idx], 1.f);
#endif
	raw_idx += ni_screen_skip(&dev_spec_raw_data[raw_idx], 480*10);
	raw_idx += ni_screen_audio_meter(&dev_spec_raw_data[raw_idx], 0.75f + r);


	memcpy(&dev_spec_raw_data[raw_idx], footer, 8);
	raw_idx += 8;
	printf("raw buffer xfer size %d\n", raw_idx);

	int32_t raw_written = ctlra_dev_screen_redraw_raw_data(dev,
					screen_idx,
					dev_spec_raw_data,
					raw_idx);

	if (raw_written == raw_idx) {
		/* Return *without* flush, as raw() already blitted. */
		return 0;
	}


	for (uint32_t i = 0; i < bytes; i++) {
		pixel_data[i] = col1;
	}

	/* return 1 to flush updates to the screens. */
	return 1;
}

int accept_dev_func(struct ctlra_t *ctlra,
		    const struct ctlra_dev_info_t *info,
		    struct ctlra_dev_t *dev,
                    void *userdata)
{
	printf("simple: accepting %s %s\n", info->vendor, info->device);

	/* here we use the Ctlra APIs to set callback functions to get
	 * events and send feedback updates to/from the device */
	ctlra_dev_set_event_func(dev, simple_event_func);
	ctlra_dev_set_feedback_func(dev, simple_feedback_func);
	ctlra_dev_set_screen_feedback_func(dev, simple_screen_redraw_func);
	ctlra_dev_set_remove_func(dev, simple_remove_func);
	ctlra_dev_set_callback_userdata(dev, 0x0);

	return 1;
}

int main(int argc, char **argv)
{
	if(argc > 1) {
		led = atoi(argv[1]);
		led_set = 1;
	}
	if(argc > 2) {
		screens_enabled = 0;
		printf("screens redrawing disabled.\n");
	}

	signal(SIGINT, sighndlr);

	struct ctlra_create_opts_t opts = {
		.screen_redraw_target_fps = 3,
	};

	struct ctlra_t *ctlra = ctlra_create(&opts);
	int num_devs = ctlra_probe(ctlra, accept_dev_func, 0x0);
	printf("connected devices %d\n", num_devs);

	while(!done) {
		ctlra_idle_iter(ctlra);
		usleep(10 * 1000);
	}

	ctlra_exit(ctlra);

	return 0;
}
