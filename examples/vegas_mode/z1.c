
#include "global.h"

#include <string.h>

#include "ctlra.h"
#include "devices/ni_kontrol_z1.h"

void kontrol_z1_update_state(struct ctlra_dev_t *dev, void *ud)
{
	struct dummy_data *d = ud;
	uint32_t i;

	for(i = 0; i < VEGAS_BTN_COUNT; i++)
		ctlra_dev_light_set(dev, i, UINT32_MAX * d->buttons[i]);

	ctlra_dev_light_flush(dev, 0);
	return;
}

void kontrol_z1_func(struct ctlra_dev_t* dev,
		     uint32_t num_events,
		     struct ctlra_event_t** events,
		     void *userdata)
{
	struct dummy_data *d = (void *)userdata;
	(void)dev;
	(void)userdata;
	for(uint32_t i = 0; i < num_events; i++) {
		struct ctlra_event_t *e = events[i];
		switch(e->type) {
		case CTLRA_EVENT_BUTTON:
			d->buttons[e->button.id] = e->button.pressed;
			if(e->button.id == NI_KONTROL_Z1_BTN_MODE) {
				memset(&d->buttons, e->button.pressed,
				       sizeof(d->buttons));
			}
			break;

		case CTLRA_EVENT_ENCODER:
			break;

		case CTLRA_EVENT_SLIDER:
			if(e->slider.id == NI_KONTROL_Z1_SLIDER_LEFT_FADER) {
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
			break;
		default:
			break;
		};
	}
	d->revision++;
}

