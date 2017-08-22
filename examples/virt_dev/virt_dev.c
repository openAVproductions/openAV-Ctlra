#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include <avtka/avtka.h>

#include "ctlra.h"

static volatile uint32_t done;
static uint32_t led;
static uint32_t led_set;

void virt_dev_feedback_func(struct ctlra_dev_t *dev, void *d)
{
	/* feedback like LEDs and Screen drawing based on application
	 * state goes here. See the vegas_mode/ example for an example of
	 * how to turn on LEDs on the controllers.
	 *
	 * Alternatively re-run the virt_dev example with an integer
	 * parameter and that light number will be lit: ./virt_dev 5
	 */
	if(led_set) {
		ctlra_dev_light_set(dev, led, 0xffffffff);
		ctlra_dev_light_flush(dev, 1);
	}
}

void virt_dev_event_func(struct ctlra_dev_t* dev, uint32_t num_events,
		       struct ctlra_event_t** events, void *userdata)
{
	/* Events from the Ctlra device are handled here. They should be
	 * decoded, and events sent to the application. Note that this
	 * function must *NOT* send feedback to the device. Instead, the
	 * feedback_func() above should be used to send feedback. For a
	 * proper demo on how to do feedback, see examples/vegas/
	 */

	static const char* grid_pressed[] = { " X ", "   " };

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
		case CTLRA_EVENT_BUTTON:
			name = ctlra_info_get_name(&info, CTLRA_EVENT_BUTTON,
						   e->button.id);
			printf("[%s] button %s (%d)\n",
			       e->button.pressed ? " X " : "   ",
			       name, e->button.id);
			break;

		case CTLRA_EVENT_ENCODER:
			name = ctlra_info_get_name(&info, CTLRA_EVENT_ENCODER,
						   e->encoder.id);
			printf("[%s] encoder %s (%d)\n",
			       e->encoder.delta > 0 ? " ->" : "<- ",
			       name, e->button.id);
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
			printf("[%s] grid %d", pressed ? " X " : "   ", e->grid.pos);
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

void virt_dev_remove_func(struct ctlra_dev_t *dev, int unexpected_removal,
			void *userdata)
{
	/* Notifies application of device removal, also allows cleanup
	 * of any device specific structures. See daemon/ example, where
	 * the MIDI I/O is cleaned up in the remove() function */
	struct ctlra_dev_info_t info;
	ctlra_dev_get_info(dev, &info);
	printf("virt_dev: removing %s %s\n", info.vendor, info.device);

}

int accept_dev_func(const struct ctlra_dev_info_t *info,
                    ctlra_event_func *event_func,
                    ctlra_feedback_func *feedback_func,
                    ctlra_remove_dev_func *remove_func,
                    void **userdata_for_event_func,
                    void *userdata)
{
	printf("virt_dev: accepting %s %s\n", info->vendor, info->device);
	*event_func    = virt_dev_event_func;
	*feedback_func = virt_dev_feedback_func;
	*remove_func   = virt_dev_remove_func;
	*userdata_for_event_func = userdata;

	return 1;
}

int main(int argc, char **argv)
{
	if(argc > 1) {
		led = atoi(argv[1]);
		led_set = 1;
	}

	signal(SIGINT, sighndlr);

#if 0
	struct avtka_opts_t opts = {
		.w = info->size_x,
		.h = info->size_y,
	};
	char dev_name[64];
	snprintf(dev_name, sizeof(dev_name), "Ctlra VDev: %s %s",
		 info->vendor, info->device);
	struct avtka_t *a = avtka_create(dev_name, &opts);

	if(info != NULL) {
		printf("static info %s %s\n  buttons = %d\n  sliders %d\n",
		       info->vendor, info->device,
		       info->control_count[CTLRA_EVENT_BUTTON],
		       info->control_count[CTLRA_EVENT_SLIDER]);

		for(int i = 0; i < info->control_count[CTLRA_EVENT_BUTTON]; i++) {
			struct ctlra_item_info_t *item =
				&info->control_info[CTLRA_EVENT_BUTTON][i];
			struct avtka_item_opts_t ai = {
				 //.name = name,
				.x = item->x,
				.y = item->y,
				.w = item->w,
				.h = item->h,
				.draw = AVTKA_DRAW_BUTTON,
			};
			avtka_item_create(a, &ai);
		}

		printf("sliders\n");
		for(int i = 0; i < info->control_count[CTLRA_EVENT_SLIDER]; i++) {
			struct ctlra_item_info_t *item =
				&info->control_info[CTLRA_EVENT_SLIDER][i];

			const char *name = ctlra_info_get_name(info,
						CTLRA_EVENT_SLIDER, i);
			printf("%d, %s: %d %d\t%d %d\n", i, name,
			       item->x, item->y,
			       item->w, item->h);

			struct avtka_item_opts_t ai = {
				.x = item->x,
				.y = item->y,
				.w = item->w,
				.h = item->h,
			};
			ai.draw = (item->flags & CTLRA_ITEM_FADER) ?
				  AVTKA_DRAW_SLIDER :  AVTKA_DRAW_DIAL;

			snprintf(ai.name, sizeof(ai.name), "%s", name);
			avtka_item_create(a, &ai);
		}
	}
#endif

	struct ctlra_t *ctlra = ctlra_create(NULL);
	int num_devs = ctlra_probe(ctlra, accept_dev_func, 0x0);
	printf("connected devices %d\n", num_devs);

	struct ctlra_dev_id_t id;
	id.type = CTLRA_DEV_TYPE_USB_HID;
	struct ctlra_dev_info_t *info = ctlra_dev_get_info_by_id(&id);

	/* add a virtualized device */
	ctlra_dev_virtualize(ctlra, info);

	int i = 0;
	while(i < 4 && !done) {
		ctlra_idle_iter(ctlra);
		usleep(10 * 1000);
	}

	ctlra_exit(ctlra);

	return 0;
}
