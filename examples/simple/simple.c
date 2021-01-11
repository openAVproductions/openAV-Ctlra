#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "ctlra.h"

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
		.screen_redraw_target_fps = 15,
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
