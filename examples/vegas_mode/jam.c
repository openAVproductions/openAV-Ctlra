
#include "global.h"

#include <string.h>

#include "ctlra.h"
#include "devices/ni_maschine_jam.h"

static int chan;
const char *patch_name;

void jam_update_state(struct ctlra_dev_t *dev, void *ud)
{
	struct dummy_data *d = ud;
	uint32_t i;

	for(i = 0; i < VEGAS_BTN_COUNT; i++)
		ctlra_dev_light_set(dev, i, UINT32_MAX * d->buttons[i]);

	ctlra_dev_light_flush(dev, 0);
	return;
}

void jam_func(struct ctlra_dev_t* dev,
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
			if(e->button.id == 10) {
				memset(&d->buttons, e->button.pressed,
				       sizeof(d->buttons));
			}
			break;

		case CTLRA_EVENT_ENCODER:
			if(e->encoder.delta > 0)
				chan++;
			else
				chan--;
			if(chan < 0)   chan =   0;
			if(chan > 127) chan = 127;
			soffa_set_patch(d->soffa, 0, 0, chan, &patch_name);
			printf("maschine: patch switched to %s (%d)\n",
			       patch_name, chan);
			break;

		case CTLRA_EVENT_SLIDER:
			break;

		case CTLRA_EVENT_GRID:
			if (e->grid.flags & CTLRA_EVENT_GRID_FLAG_BUTTON) {
				d->buttons[e->grid.pos] = e->grid.pressed;
				if(e->grid.pressed) {
					soffa_note_on(d->soffa, 0, 36 + e->grid.pos, 0.7);
					printf("jam note on %d\n", e->grid.pos);
				}
				else
					soffa_note_off(d->soffa, 0, 36 + e->grid.pos);
			}
		default:
			break;
		};
	}
	d->revision++;
}

