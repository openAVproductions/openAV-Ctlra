
#include "global.h"

#include <string.h>

#include "ctlr/ctlr.h"
#include "ctlr/devices/ni_kontrol_z1.h"

void kontrol_z1_update_state(struct ctlr_dev_t *dev, struct dummy_data *d)
{
	uint32_t i;

	for(i = 0; i < VEGAS_BTN_COUNT; i++)
		ctlr_dev_light_set(dev, i, UINT32_MAX * d->buttons[i]);

	ctlr_dev_light_flush(dev, 0);
	return;
}

void kontrol_z1_func(struct ctlr_dev_t* dev,
		     uint32_t num_events,
		     struct ctlr_event_t** events,
		     void *userdata)
{
	struct dummy_data *d = (void *)userdata;
	(void)dev;
	(void)userdata;
	for(uint32_t i = 0; i < num_events; i++) {
		char *pressed = 0;
		struct ctlr_event_t *e = events[i];
		const char *name = 0;
		switch(e->id) {
		case CTLR_EVENT_BUTTON:
			if(d->print_events) {
				name = ctlr_dev_control_get_name(dev, e->button.id);
				printf("[%s] button %s (%d)\n",
				       e->button.pressed ? " X " : "   ",
				       name, e->button.id);
			}
			d->buttons[e->button.id] = e->button.pressed;
			if(e->button.id == NI_KONTROL_Z1_BTN_MODE) {
				memset(&d->buttons, e->button.pressed, VEGAS_BTN_COUNT);
			}
			break;

		case CTLR_EVENT_ENCODER:
			if(d->print_events) {
				name = ctlr_dev_control_get_name(dev, e->button.id);
				printf("[%s] encoder %s (%d)\n",
				       e->encoder.delta > 0 ? " ->" : "<- ",
				       name, e->button.id);
			}
			break;

		case CTLR_EVENT_SLIDER:
			if(d->print_events) {
				name = ctlr_dev_control_get_name(dev, e->slider.id);
				printf("[%03d] slider %s (%d)\n",
				       (int)(e->slider.value * 100.f),
				       name, e->slider.id);
			}
			if(e->slider.id == NI_KONTROL_Z1_SLIDER_LEFT_FADER) {
				d->volume = e->slider.value;
				uint32_t iter = (int)((d->volume+0.05) * 7.f);
				for(i = 0; i < iter; i++) {
					ctlr_dev_light_set(dev, 1 + i, UINT32_MAX);
					ctlr_dev_light_set(dev, 8 + i, UINT32_MAX);
				}
				for(; i < 7.0; i++) {
					ctlr_dev_light_set(dev, 1 + i, 0);
					ctlr_dev_light_set(dev, 8 + i, 0);
				}
			}
			break;

		case CTLR_EVENT_GRID:
			if(d->print_events) {
				name = ctlr_dev_control_get_name(dev, e->button.id);
				if(e->grid.flags & CTLR_EVENT_GRID_BUTTON) {
					pressed = e->grid.pressed ? " X " : "   ";
				} else {
					pressed = "---";
				}
				printf("[%s] grid %d", pressed, e->grid.pos);
				if(e->grid.flags & CTLR_EVENT_GRID_PRESSURE)
					printf(", pressure %1.3f", e->grid.pressure);
				printf("\n");
			}
			break;
		default:
			break;
		};
	}
	d->revision++;
}

