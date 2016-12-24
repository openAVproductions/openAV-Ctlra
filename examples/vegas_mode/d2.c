
#include "global.h"

#include <string.h>

#include "ctlra/ctlra.h"
#include "ctlra/devices/ni_kontrol_d2.h"

void kontrol_d2_update_state(struct ctlra_dev_t *dev, struct dummy_data *d)
{
	uint32_t i;

	for(i = 0; i < VEGAS_BTN_COUNT; i++)
		ctlra_dev_light_set(dev, i, UINT32_MAX * d->buttons[i]);

	uint8_t orange[25] = {0};
	uint8_t blue[25] = {0};

	for(int i = 0; i < d->progress * 25; i++) {
		blue[i] = 0xff;
	}

	ni_kontrol_d2_light_touchstrip(dev, orange, blue);

	ctlra_dev_light_flush(dev, 1);
	return;
}

void kontrol_d2_func_remap(struct ctlra_dev_t* dev,
			   uint32_t num_events,
			   struct ctlra_event_t** events,
			   void *userdata)
{
	printf("%s handling events\n", __func__);
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
	printf("%s handling events\n", __func__);
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
				       sizeof(struct dummy_data));
			}
			if(e->button.id == NI_KONTROL_D2_BTN_DECK && p) {
				printf("deck presend\n");
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

