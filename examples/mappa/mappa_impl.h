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

/* internal data structure for linked list of sw feedback sources */
struct source_t {
	TAILQ_ENTRY(source_t) tailq;
	struct mappa_sw_source_t source;
};
TAILQ_HEAD(source_list_t, source_t);

/* forward decl only */
struct mappa_t;

struct lut_t {
	TAILQ_ENTRY(lut_t) tailq;

	/* The ID of this LUT. Used to switch between luts */
	uint32_t id;

	/* structure for lookup:
	 * - dynamic alloc array for each type of control
	 * - enables easy lookup for each event->id
	 * - no memory wastage
	 * - only have performance required items in cache
	 */
	struct mappa_sw_target_t *target_types[CTLRA_EVENT_T_COUNT];

	/* array of feedback items that the HW has, that can be mapped to.
	 * The size of the array is defined by the number of feedback items
	 * on the device */
	struct mappa_sw_source_t **sources;
};
TAILQ_HEAD(lut_list_t, lut_t);

/* device specific struct, passed into each device callback as userdata
 * This is used to easily look up the correct layer / lut, and then use
 * it to handle the events.
 */
struct dev_t {
	/* back pointer to the main instance. Allows the ctlra user-data
	 * pointer to be pointed at this struct for easy access to the
	 * main LUT for event dispatch, but allow self access.
	 */
	struct mappa_t *self;

	/* List of look up tables for events. Only to keep track of them,
	 * and switch between them as required. The hot-path uses a direct
	 * pointer to the lut
	 */
	struct lut_list_t lut_list;
	/* the active lut to use for incoming events */
	struct lut_t *active_lut;
	/* unique integer ID starting from zero for each LUT */
	uint32_t lut_idx;

	/* ctlra dev info, required to error check binding IDs */
	struct ctlra_dev_info_t ctlra_dev_info;
};

struct mappa_t {
	struct ctlra_t *ctlra;

	struct dev_t *dev;

	/* container for all targets in the system. This is not used to
	 * look-up events when they occur on the Ctlra device, this is
	 * book-keeping only for add/remove, and map/unmap.
	 */
	struct target_list_t target_list;

	/* container for all sources in the system. Not used for lookup */
	struct source_list_t source_list;
};
