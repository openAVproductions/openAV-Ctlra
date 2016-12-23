
#ifndef VEGAS_MODE_GLOBAL_H
#define VEGAS_MODE_GLOBAL_H

#include "ctlra.h"

#include <stdio.h>
#include <stdint.h>

#define VEGAS_BTN_COUNT 128

struct dummy_data {
	uint8_t print_events;
	uint64_t revision;
	float volume;
	int playing[2];
	int buttons[VEGAS_BTN_COUNT];
};

/* Typedefs so we can store the per-device functions in a struct, see
 * vegas_mode.c ctlra_supported_t for details and how these are  used */
typedef void (*update_state_cb)(struct ctlra_dev_t *dev, struct dummy_data *d);
typedef void (*ctlra_poll_func)(struct ctlra_dev_t* dev, uint32_t num_events,
			       struct ctlra_event_t** events, void *userdata);

/* Functions to poll / push state to the devs */
void kontrol_x1_update_state(struct ctlra_dev_t *dev, struct dummy_data *d);
void kontrol_x1_func(struct ctlra_dev_t* dev, uint32_t num_events,
		     struct ctlra_event_t** events, void *userdata);

void kontrol_z1_update_state(struct ctlra_dev_t *dev, struct dummy_data *d);
void kontrol_z1_func(struct ctlra_dev_t* dev, uint32_t num_events,
		     struct ctlra_event_t** events, void *userdata);

void kontrol_d2_update_state(struct ctlra_dev_t *dev, struct dummy_data *d);
void kontrol_d2_func(struct ctlra_dev_t* dev, uint32_t num_events,
		     struct ctlra_event_t** events, void *userdata);

void kontrol_f1_update_state(struct ctlra_dev_t *dev, struct dummy_data *d);
void kontrol_f1_func(struct ctlra_dev_t* dev, uint32_t num_events,
		     struct ctlra_event_t** events, void *userdata);

void mm_update_state(struct ctlra_dev_t *dev, struct dummy_data *d);
void mm_func(struct ctlra_dev_t* dev, uint32_t num_events,
	     struct ctlra_event_t** events, void *userdata);

#endif /* VEGAS_MODE_GLOBAL_H */
