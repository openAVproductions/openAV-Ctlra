
#include "global.h"

#include "ctlra.h"
#include "devices/ni_maschine_mikro_mk2.h"

static int chan;

void mm_update_state(struct ctlra_dev_t *dev, void *ud)
{
	struct dummy_data *d = ud;
	uint32_t i;
	for(i = 0; i < VEGAS_BTN_COUNT; i++)
		ctlra_dev_light_set(dev, i, UINT32_MAX * d->buttons[i]);
	ctlra_dev_light_flush(dev, 0);
	return;
}

void mm_func(struct ctlra_dev_t* dev,
	     uint32_t num_events,
	     struct ctlra_event_t** events,
	     void *userdata)
{
	struct dummy_data *dummy = (void *)userdata;
	(void)dev;
	(void)userdata;
	for(uint32_t i = 0; i < num_events; i++) {
		//char *pressed = 0;
		struct ctlra_event_t *e = events[i];
		switch(e->type) {
		case CTLRA_EVENT_BUTTON:
			ctlra_dev_light_set(dev, e->button.id, UINT32_MAX);
			break;
		case CTLRA_EVENT_ENCODER:
			if(e->encoder.delta > 0)
				chan++;
			else
				chan--;
			if(chan < 0)
				chan = 0;
			const char *name = 0;
			soffa_set_patch(dummy->soffa, 0, 0, chan, &name);
			printf("enc %d, %d, patch = %d\t%s\n",
			       e->encoder.id, e->encoder.delta, chan, name);

			break;
		case CTLRA_EVENT_SLIDER:
			if(e->slider.id == 11) {
				uint32_t iter = (int)((e->slider.value+0.05) * 7.f);
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
			if(e->grid.flags & CTLRA_EVENT_GRID_FLAG_BUTTON) {
				dummy->buttons[e->grid.pos] = e->grid.pressed;
				if(e->grid.pressed)
					soffa_note_on(dummy->soffa, 0, 36 + e->grid.pos,
						      0.3 + 0.7 * e->grid.pressure);
				else
					soffa_note_off(dummy->soffa, 0, 36 + e->grid.pos);
			} else {
				//pressed = "---";
			}
#if 0
			printf("[%s] grid %d", pressed, e->grid.pos);
			if(e->grid.flags & CTLRA_EVENT_GRID_FLAG_PRESSURE)
				printf(", pressure %1.3f", e->grid.pressure);
			printf("\n");
#endif
			break;
		default:
			break;
		};
	}
	dummy->revision++;
}

