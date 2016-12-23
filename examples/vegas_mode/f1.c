
#include "global.h"

#include <string.h>

#include "ctlra/ctlra.h"
#include "ctlra/devices/ni_kontrol_f1.h"

void kontrol_f1_update_state(struct ctlra_dev_t *dev, struct dummy_data *d)
{
	uint32_t i;

	for(i = 0; i < VEGAS_BTN_COUNT; i++)
		ctlra_dev_light_set(dev, i, UINT32_MAX * d->buttons[i]);

	ctlra_dev_light_flush(dev, 0);
	return;
}

void kontrol_f1_func(struct ctlra_dev_t* dev,
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
		const char *name = 0;
		switch(e->type) {
		case CTLRA_EVENT_BUTTON:
			if(d->print_events) {
				name = ctlra_dev_control_get_name(dev, e->button.id);
				printf("[%s] button %s (%d)\n",
				       e->button.pressed ? " X " : "   ",
				       name, e->button.id);
			}
			d->buttons[e->button.id] = e->button.pressed;
			if(e->button.id == NI_KONTROL_F1_BTN_SHIFT) {
				memset(&d->buttons, e->button.pressed,
				       sizeof(struct dummy_data));
			}
			break;

		case CTLRA_EVENT_ENCODER:
			if(d->print_events) {
				name = ctlra_dev_control_get_name(dev, e->button.id);
				printf("[%s] encoder %s (%d)\n",
				       e->encoder.delta > 0 ? " ->" : "<- ",
				       name, e->button.id);
			}
			break;

		case CTLRA_EVENT_SLIDER:
			if(d->print_events) {
				name = ctlra_dev_control_get_name(dev, e->button.id);
				printf("[%03d] slider %s (%d)\n",
				       (int)(e->slider.value * 100.f),
				       name, e->slider.id);
			}
			if(e->slider.id == NI_KONTROL_F1_SLIDER_FADER_1) {
				d->volume = e->slider.value;
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
			if(d->print_events) {
				name = ctlra_dev_control_get_name(dev, e->button.id);
				if(e->grid.flags & CTLRA_EVENT_GRID_BUTTON) {
					pressed = e->grid.pressed ? " X " : "   ";
				} else {
					pressed = "---";
				}
				printf("[%s] grid %d", pressed, e->grid.pos);
				if(e->grid.flags & CTLRA_EVENT_GRID_PRESSURE)
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

