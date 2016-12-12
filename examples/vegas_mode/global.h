
#ifndef VEGAS_MODE_GLOBAL_H
#define VEGAS_MODE_GLOBAL_H

#include "ctlr.h"

#include <stdio.h>
#include <stdint.h>

struct dummy_data {
	uint64_t revision;
	float volume;
	int playing[2];
};

/* Functions to poll / push state to the devs */
void kontrol_x1_update_state(struct ctlr_dev_t *dev, struct dummy_data *d);
void kontrol_x1_func(struct ctlr_dev_t* dev, uint32_t num_events,
		     struct ctlr_event_t** events, void *userdata);

void kontrol_z1_update_state(struct ctlr_dev_t *dev, struct dummy_data *d);
void kontrol_z1_func(struct ctlr_dev_t* dev, uint32_t num_events,
		     struct ctlr_event_t** events, void *userdata);

#endif /* VEGAS_MODE_GLOBAL_H */
