
#include "global.h"

#include <stdlib.h>

#include "ctlra.h"
#include "devices/ni_maschine_mikro_mk2.h"

static int chan;
const char *patch_name;

const char modes[] = {
	"Pads",
};

struct mikro_t
{
	/* changes between note-ons and other functions */
	int mode;
};

void mm_update_state(struct ctlra_dev_t *dev, void *ud)
{
	struct dummy_data *d = ud;
	struct mikro_t *m = d->mikro;

	int i;
	for(i = 0; i < VEGAS_BTN_COUNT; i++)
		ctlra_dev_light_set(dev, i, 0);

	if(!m) {
		ctlra_dev_light_set(dev, 22, UINT32_MAX);
		return;
	}

	switch(m->mode) {
	case 0:
		ctlra_dev_light_set(dev, 22, UINT32_MAX);
		ctlra_dev_light_flush(dev, 0);
		break;
	default:
		ctlra_dev_light_set(dev, 22, 0);
		for(i = 0; i < VEGAS_BTN_COUNT; i++)
			ctlra_dev_light_set(dev, i, UINT32_MAX * d->buttons[i]);
		break;
	}

	uint8_t *pixel_data;
	uint32_t bytes;
	int has_screen = ctlra_dev_screen_get_data(dev,
						   &pixel_data,
						   &bytes, 0);

	/* TODO: fix the bytes parameter, once a decision on the
	 * general approach is made; see here for details:
	 * https://github.com/openAVproductions/openAV-Avtka/pull/5
	 */
	/* 128x64 pixels, RGB = 3 channels, Cairo Stride +1 */
#define SIZE (128 * 64 * (3 + 1))
	for(int i = 0; i < SIZE; i++)
		pixel_data[i] = rand();

	return;
}

void mm_pads(struct ctlra_dev_t* dev,
	     uint32_t num_events,
	     struct ctlra_event_t** events,
	     void *userdata)
{
	struct dummy_data *dummy = (void *)userdata;

	for(uint32_t i = 0; i < num_events; i++) {
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
			if(chan < 0)   chan =   0;
			if(chan > 127) chan = 127;
			soffa_set_patch(dummy->soffa, 0, 0, chan, &patch_name);
			printf("maschine: patch switched to %s (%d)\n",
			       patch_name, chan);
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
			}
			break;
		default:
			break;
		};
	}
	dummy->revision++;
}

void mm_func(struct ctlra_dev_t* dev,
	     uint32_t num_events,
	     struct ctlra_event_t** events,
	     void *userdata)
{
	struct dummy_data *dummy = (void *)userdata;

	if(!dummy->mikro) {
		dummy->mikro = calloc(1, sizeof(struct mikro_t));
		if(!dummy->mikro) {
			printf("failed to allocate mikro struct\n");
			exit(-1);
		}
	}

	struct mikro_t *m = dummy->mikro;
	if(m->mode == 0) {
		mm_pads(dev, num_events, events, userdata);
	}
}

