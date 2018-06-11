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

struct mappa_t {
	struct ctlra_t *ctlra;

	/* container for all targets in the system. This is not used to
	 * look-up events when they occur on the Ctlra device, this is
	 * book-keeping only for add/remove, and map/unmap.
	 */
	struct target_list_t target_list;
};

