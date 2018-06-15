#pragma once

#include "mappa.h"
#include "ctlra.h"

#include <sys/queue.h>

/* internal data structure for a linked list of targets */
struct target_t {
	TAILQ_ENTRY(target_t) tailq;
	struct mappa_sw_target_t target;
};
TAILQ_HEAD(target_list_t, target_t);

/* forward decl only */
struct mappa_t;

struct lut_t {
	/* back pointer to the main instance. Allows the ctlra user-data
	 * pointer to be pointed at this struct for easy access to the
	 * main LUT for event dispatch, but allow self access.
	 */
	struct mappa_t *self;

	/* structure for lookup:
	 * - dynamic alloc array for each type of control
	 * - enables easy lookup for each event->id
	 * - no memory wastage
	 * - only have performance required items in cache
	 */
	struct mappa_sw_target_t *target_types[CTLRA_EVENT_T_COUNT];

	TAILQ_ENTRY(lut_t) tailq;
};
TAILQ_HEAD(lut_list_t, lut_t);

struct mappa_t {
	struct ctlra_t *ctlra;

	/* container for all targets in the system. This is not used to
	 * look-up events when they occur on the Ctlra device, this is
	 * book-keeping only for add/remove, and map/unmap.
	 */
	struct target_list_t target_list;

	struct lut_t *lut;
};
