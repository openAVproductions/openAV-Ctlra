
#include "global.h"

#include "ctlr/ctlr.h"
#include "ctlr/devices/ni_kontrol_x1_mk2.h"

void kontrol_x1_update_state(struct ctlr_dev_t *dev, struct dummy_data *d)
{
	uint32_t i;
	int v = d->volume > 0.5f;
	ctlr_dev_light_set(dev, 15, v * UINT32_MAX);

	for(i = 0; i < VEGAS_BTN_COUNT; i++)
		ctlr_dev_light_set(dev, i, UINT32_MAX * d->buttons[i]);

	ctlr_dev_light_flush(dev, 0);
	return;
}

void kontrol_x1_func(struct ctlr_dev_t* dev,
		     uint32_t num_events,
		     struct ctlr_event_t** events,
		     void *userdata)
{
	struct dummy_data *d = (void *)userdata;
	for(uint32_t i = 0; i < num_events; i++) {
		char *pressed = 0;
		struct ctlr_event_t *e = events[i];
		const char *name = 0;
		switch(e->type) {

		case CTLR_EVENT_BUTTON:
			if(d->print_events) {
				name = ctlr_dev_control_get_name(dev, e->button.id);
				printf("[%s] button %s (%d)\n",
				       e->button.pressed ? " X " : "   ",
				       name, e->button.id);
			}
			d->buttons[e->button.id] = e->button.pressed;
			ctlr_dev_light_set(dev, e->button.id, UINT32_MAX);
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
				name = ctlr_dev_control_get_name(dev, e->button.id);
				printf("[%03d] slider %s (%d)\n",
				       (int)(e->slider.value * 100.f),
				       name, e->slider.id);
			}

			if(e->slider.id == 11) {
				uint32_t iter = (int)((e->slider.value+0.05) * 7.f);
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
			if(e->grid.flags & CTLR_EVENT_GRID_BUTTON) {
				pressed = e->grid.pressed ? " X " : "   ";
			} else {
				pressed = "---";
			}
			if(d->print_events) {
				name = ctlr_dev_control_get_name(dev, e->button.id);
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

