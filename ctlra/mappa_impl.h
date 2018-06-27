#pragma once

#include "mappa.h"
#include "ctlra.h"

#include <sys/queue.h>

#define mappa_print_check(c, level)					\
	(!c || (c->opts.debug_level >= level))

#define MAPPA_ERROR(mappa, fmt, ...)					\
	do { if (mappa_print_check(mappa, MAPPA_DEBUG_ERROR))		\
	fprintf(stderr, "[\033[1;31m%s +%d\033[0m] " fmt,		\
		__func__, __LINE__, __VA_ARGS__);			\
	} while (0)
#define MAPPA_WARN(mappa, fmt, ...)					\
	do { if (mappa_print_check(mappa, MAPPA_DEBUG_WARN))		\
	fprintf(stderr, "[\033[1;33m%s +%d\033[0m] " fmt,		\
		__func__, __LINE__, __VA_ARGS__);			\
	} while (0)
#define MAPPA_INFO(mappa, fmt, ...)					\
	do { if (mappa_print_check(mappa, MAPPA_DEBUG_INFO))		\
	fprintf(stderr, "[\033[1;32m%s +%d\033[0m] " fmt,		\
		__func__, __LINE__, __VA_ARGS__);			\
	} while (0)



/* internal data structure for a linked list of targets */
struct target_t {
	TAILQ_ENTRY(target_t) tailq;
	struct mappa_target_t target;
	uint32_t id;
	/* variably sized token size and data */
	uint32_t token_size;
	uint8_t token_buf[];
};
TAILQ_HEAD(target_list_t, target_t);

/* internal data structure for linked list of sw feedback sources */
struct source_t {
	TAILQ_ENTRY(source_t) tailq;
	struct mappa_source_t source;
	uint32_t id;
};
TAILQ_HEAD(source_list_t, source_t);

/* forward decl only */
struct mappa_t;

struct lut_t {
	TAILQ_ENTRY(lut_t) tailq;

	/* name uniquely identifies lut. This requires some string compares
	 * on layer-switching, however cost is only on change, not on event
	 * lookup - so performance is not a major concern here */
	char *name;

	/* structure for lookup:
	 * - dynamic alloc array for each type of control
	 * - enables easy lookup for each event->id
	 * - no memory wastage
	 * - only have performance required items in cache
	 */
	struct target_t **target_types[CTLRA_EVENT_T_COUNT];

	/* array of feedback items that the HW has, that can be mapped to.
	 * The size of the array is defined by the number of feedback items
	 * on the device */
	struct mappa_source_t **sources;
};
TAILQ_HEAD(lut_list_t, lut_t);

/* device specific struct, passed into each device callback as userdata
 * This is used to easily look up the correct layer / lut, and then use
 * it to handle the events.
 */
struct dev_t {
	TAILQ_ENTRY(dev_t) tailq;
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

	/* dev id for mappa bindings */
	uint32_t id;

	/* ctlra dev info, required to error check binding IDs */
	struct ctlra_dev_info_t ctlra_dev_info;
};
TAILQ_HEAD(dev_list_t, dev_t);

struct mappa_t {
	struct mappa_opts_t opts;
	struct ctlra_t *ctlra;

	uint32_t dev_ids;
	uint32_t target_ids;
	uint32_t source_ids;

	/* list of devices currently in use by mappa */
	struct dev_list_t dev_list;

	/* container for all targets in the system. This is not used to
	 * look-up events when they occur on the Ctlra device, this is
	 * book-keeping only for add/remove, and map/unmap.
	 */
	struct target_list_t target_list;

	/* container for all sources in the system. Not used for lookup */
	struct source_list_t source_list;
};

/* create a binding from the ctlra dev id at control id, to gid,iid, for
 * layer name
 */
int32_t mappa_bind_ctlra_to_target(struct mappa_t *m,
				   uint32_t ctlra_dev_id,
				   const char *layer_name,
				   uint32_t control_type,
				   uint32_t control_id,
				   uint32_t target_id);

/* retrieve a target id from a mappa instance.
 * @retval 0 Target name not registered, invalid to map
 * @retval >0 Valid target ID, pass to bind_ctlra_to_target()
 */
uint32_t mappa_get_target_id(struct mappa_t *m, const char *target_name);

uint32_t mappa_get_source_id(struct mappa_t *m, const char *sname);


int32_t mappa_bind_source_to_ctlra(struct mappa_t *m,
				   uint32_t ctlra_dev_id,
				   const char *layer,
				   uint32_t ctlra_feedback_id,
				   uint32_t source_id);
