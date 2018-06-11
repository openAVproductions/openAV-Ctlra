
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include "mappa.h"
#include "mappa_impl.h"

#include "ctlra.h"

/* per ctlra-device we need a look-up table, which points to the
 * correct target function pointer to call based on the current layer.
 *
 * These layers of "ctlra-to-target" pairs are the mappings that need to
 * be saved/restored. Somehow, the uint32 group/item id pairs need to be
 * consistent in the host to re-connect correctly? Or use group_id / item_id?
 */

/* LAYER notes;
 * - Need to be able to display clearly to user
 * - Individual layers need to make sense
 * - "mask" layers that only overwrite certain parts
 * --- mask layer implementation returns which events were handled?
 * --- if not handled, go "down the stack" of layers?
 * --- enables "masked-masked" layers etc... complexity worth it?
 * - Display each layer as tab along top of mapping UI?
 * --- A tab button [+] to create a new layer?
 * --- Adding a new layer adds Group "mappa" : Item "Layer 1" ?
 */

void mappa_feedback_func(struct ctlra_dev_t *dev, void *d)
{
}

void mappa_event_func(struct ctlra_dev_t* dev, uint32_t num_events,
		       struct ctlra_event_t** events, void *userdata)
{
	struct ctlra_dev_info_t info;
	ctlra_dev_get_info(dev, &info);

	for(uint32_t i = 0; i < num_events; i++) {
		struct ctlra_event_t *e = events[i];
		const char *pressed = 0;
		const char *name = 0;
		switch(e->type) {
		case CTLRA_EVENT_BUTTON: {
			name = ctlra_info_get_name(&info, CTLRA_EVENT_BUTTON,
						   e->button.id);
			char pressure[16] = {0};
			if(e->button.has_pressure) {
				snprintf(pressure, sizeof(pressure),
					 "%0.01f", e->button.pressure);
			}
			printf("[%s] button %s (%d)\n",
				e->button.has_pressure ?  pressure :
				(e->button.pressed ? " X " : "   "),
				name, e->button.id);
			}
			if(e->button.pressed) {
				/* any button down event */
			}
			break;


		case CTLRA_EVENT_ENCODER:
			name = ctlra_info_get_name(&info, CTLRA_EVENT_ENCODER,
						   e->encoder.id);
			printf("[%s] encoder %s (%d)\n",
			       e->encoder.delta > 0 ? " ->" : "<- ",
			       name, e->button.id);
			if(e->encoder.flags & CTLRA_EVENT_ENCODER_FLAG_FLOAT) {
				//printf("delta float = %f\n", e->encoder.delta_float);
			} break;

		case CTLRA_EVENT_SLIDER:
			name = ctlra_info_get_name(&info, CTLRA_EVENT_SLIDER,
						   e->slider.id);
			printf("[%03d] slider %s (%d)\n",
			       (int)(e->slider.value * 100.f),
			       name, e->slider.id);
			break;

		case CTLRA_EVENT_GRID:
			name = ctlra_info_get_name(&info, CTLRA_EVENT_GRID,
			                                  e->grid.id);
			if(e->grid.flags & CTLRA_EVENT_GRID_FLAG_BUTTON) {
				pressed = e->grid.pressed ? " X " : " - ";
			} else {
				pressed = "---";
			}
			printf("[%s] grid %d", pressed, e->grid.pos);
			if(e->grid.flags & CTLRA_EVENT_GRID_FLAG_PRESSURE)
				printf(", pressure %1.3f", e->grid.pressure);
			printf("\n");
			break;
		default:
			break;
		};
	}
}

/* perform a deep copy of the target so it doesn't rely on app memory */
static struct target_t *
target_create_copy_for_list(const struct mappa_sw_target_t *t)
{
	/* allocate a size for the list-pointer-enabled struct */
	struct target_t *n = malloc(sizeof(struct target_t));

	/* dereference the current target, shallow copy values */
	n->target = *t;

	/* deep copy strings (to not depend on app provided strings) */
	if(t->group_name)
		n->target.group_name = strdup(t->group_name);
	if(t->item_name)
		n->target.item_name = strdup(t->item_name);

	return n;
}

static void
target_destroy(struct target_t *t)
{
	assert(t != NULL);

	if(t->target.group_name)
		free(t->target.group_name);
	if(t->target.item_name)
		free(t->target.item_name);

	free(t);
}

int32_t
mappa_sw_target_add(struct mappa_t *m, struct mappa_sw_target_t *t)
{
	struct target_t * n= target_create_copy_for_list(t);
	TAILQ_INSERT_HEAD(&m->target_list, n, tailq);

	/* TODO: must each group_id and item_id be unique? Do we need to
	 * check this before add? How does remove work if we don't have
	 * a unique id?
	 */
	return 0;
}

int32_t
mappa_sw_target_remove(struct mappa_t *m, uint32_t group_id,
		       uint32_t item_id)
{
	return 0;
}

void
mappa_remove_func(struct ctlra_dev_t *dev, int unexpected_removal,
		  void *userdata)
{
	struct ctlra_dev_info_t info;
	ctlra_dev_get_info(dev, &info);
	printf("mappa_app: removing %s %s\n", info.vendor, info.device);
}

int
mappa_accept_func(struct ctlra_t *ctlra, const struct ctlra_dev_info_t *info,
		  struct ctlra_dev_t *dev, void *userdata)
{
	struct mappa_t *m = userdata;

	ctlra_dev_set_event_func(dev, mappa_event_func);
	ctlra_dev_set_feedback_func(dev, mappa_feedback_func);
	ctlra_dev_set_remove_func(dev, mappa_remove_func);
	ctlra_dev_set_callback_userdata(dev, m);

	return 1;
}

int32_t
mappa_iter(struct mappa_t *m)
{
	ctlra_idle_iter(m->ctlra);
	return 0;
}

struct mappa_t *
mappa_create(struct mappa_opts_t *opts)
{
	(void)opts;
	struct mappa_t *m = calloc(1, sizeof(struct mappa_t));
	if(!m)
		goto fail;

	struct ctlra_t *c = ctlra_create(NULL);
	if(!c)
		goto fail;

	m->ctlra = c;

	TAILQ_INIT(&m->target_list);

	int num_devs = ctlra_probe(c, mappa_accept_func, m);
	printf("mappa connected to %d devices\n", num_devs);

	return m;
fail:
	if(m)
		free(m);
	return 0;
}

void
mappa_destroy(struct mappa_t *m)
{
	struct target_t *i;
	while (!TAILQ_EMPTY(&m->target_list)) {
		i = TAILQ_FIRST(&m->target_list);
		TAILQ_REMOVE(&m->target_list, i, tailq);
		target_destroy(i);
	}

	/* iterate over all allocated resources and free them */
	ctlra_exit(m->ctlra);
	free(m);
}
