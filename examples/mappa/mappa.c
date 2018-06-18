
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "mappa.h"
#include "mappa_impl.h"

#include "ctlra.h"
#include "ini.h"

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

/* mushes a single float to multiple feedback items. Eg: volume LEDs.
 * The application supplies a single float value, shown as VU LED strip
 */
static uint32_t
source_mush_value_to_array(float v, uint32_t idx, uint32_t total)
{
	return v > (idx / ((float)total)) ? -1 : 0xf;
}

void mappa_feedback_func(struct ctlra_dev_t *ctlra_dev, void *userdata)
{
	struct dev_t *dev = userdata;
	struct mappa_t *m = dev->self;
	struct lut_t *lut = dev->active_lut;

	/* iterate over the # of destinations, pulling sources for each */
	int count = dev->ctlra_dev_info.control_count[CTLRA_FEEDBACK_ITEM];
	for(int i = 0; i < count; i++) {
		struct mappa_source_t *s = lut->sources[i];
		if(s && s->func) {
			float v;
			uint32_t token_size = 0;
			s->func(&v, 0, token_size, s->userdata);
			for(int j = 0; j < 7; j++) {
				ctlra_dev_light_set(ctlra_dev, i * 7 + j,
						    source_mush_value_to_array(v, j, 7));
			}
		}
	}
	ctlra_dev_light_flush(ctlra_dev, 1);

	/*
	int sidx = 0;
	struct source_t *s;
	TAILQ_FOREACH(s, &m->source_list, tailq) {
		float v;
		void *token = 0x0;
		if(s && s->source.func)
			s->source.func(token, &v, s->source.userdata);

	}
	ctlra_dev_light_flush(ctlra_dev, 1);
	*/
}

void mappa_event_func(struct ctlra_dev_t* ctlra_dev, uint32_t num_events,
		       struct ctlra_event_t** events, void *userdata)
{
	struct dev_t *dev = userdata;
	if(!dev) {
		printf("programming error - dev is NULL\n");
		return;
	}
	struct lut_t *lut = dev->active_lut;
	if(!lut) {
		printf("programming error - lut is NULL\n");
		return;
	}

	struct target_t *t = 0;

	for(uint32_t i = 0; i < num_events; i++) {
		struct ctlra_event_t *e = events[i];
		float v = 0;
		uint32_t id = 0;

		switch(e->type) {
		case CTLRA_EVENT_BUTTON:
			id = e->button.id;
			t = lut->target_types[CTLRA_EVENT_BUTTON][id];
			v = e->button.pressed;
			break;
		case CTLRA_EVENT_ENCODER:
			// TODO: handle delta floats
			id = e->encoder.id;
			t = lut->target_types[CTLRA_EVENT_ENCODER][id];
			v = e->encoder.delta;
			break;
		case CTLRA_EVENT_SLIDER:
			id = e->slider.id;
			t = lut->target_types[CTLRA_EVENT_SLIDER][id];
			v = e->slider.value;
			break;
		case CTLRA_EVENT_GRID:
			break;
		default:
			break;
		};

		/* ensure event id is within known range. Badly written
		 * device code might send events that they don't advertise.
		 * Hence, we check if the event is in the advertised range,
		 * and bomb out if its not, notifying the dev.
		 */
		const int count = dev->ctlra_dev_info.control_count[e->type];
		if(id >= count) {
			printf("%s() device error: type %d id %d >= control count %d\n",
			       __func__, e->type, id, count);
			continue;
		}

		/* perform callback as currently mapped by lut */
		float value = v * 1.0f; // TODO scaling here?
		if(t && t->target.func)
			t->target.func(t->id, value,
				       t->token_buf, t->token_size,
				       t->target.userdata);
	}
}

/* perform a deep copy of the target so it doesn't rely on app memory */
static struct target_t *
target_clone(const struct mappa_target_t *t, uint32_t token_size,
	     void *token)
{
	/* allocate a size for the list-pointer-enabled struct */
	struct target_t *n = malloc(sizeof(struct target_t) + token_size);
	/* dereference the current target, shallow copy values */
	n->target = *t;
	/* deep copy strings (to not depend on app provided strings) */
	if(t->name)
		n->target.name = strdup(t->name);
	/* store token */
	n->token_size = token_size;
	memcpy(n->token_buf, token, token_size);

	return n;
}

static void
target_destroy(struct target_t *t)
{
	assert(t != NULL);
	free(t->target.name);
	free(t);
}

static void
source_destroy(struct source_t *s)
{
	assert(s != NULL);
	free(s->source.name);
	free(s);
}

struct source_t *
source_deep_copy(const struct mappa_source_t *s)
{
	struct source_t *n = calloc(1, sizeof(struct source_t));
	n->source = *s;
	if(s->name)
		n->source.name = strdup(s->name);
	return n;
}

int32_t
mappa_source_add(struct mappa_t *m, struct mappa_source_t *t,
		 uint32_t *source_id, void *token, uint32_t token_size)
{
	struct source_t *n = source_deep_copy(t);
	TAILQ_INSERT_HEAD(&m->source_list, n, tailq);
	n->id = m->source_ids++;
	if(source_id)
		*source_id = n->id;
	/*
	printf("added source %s, id %u, func %p\n", n->source.name, n->id,
	       n->source.func);
	*/

	return 0;
}

uint32_t
mappa_get_source_id(struct mappa_t *m, const char *sname)
{
	struct source_t *s;
	TAILQ_FOREACH(s, &m->source_list, tailq) {
		if(strcmp(s->source.name, sname) == 0) {
			return s->id;
		}
	}
	return 0;
}

int32_t
mappa_target_add(struct mappa_t *m,
		 struct mappa_target_t *t,
		 uint32_t *target_id,
		 void *token,
		 uint32_t token_size)
{
	struct target_t *n = target_clone(t, token_size, token);
	TAILQ_INSERT_HEAD(&m->target_list, n, tailq);
	n->id = m->target_ids++;
	if(target_id)
		*target_id = n->id;
	return 0;
}

uint32_t
mappa_get_target_id(struct mappa_t *m, const char *tname)
{
	struct target_t *t;
	TAILQ_FOREACH(t, &m->target_list, tailq) {
		if(strcmp(t->target.name, tname) == 0) {
			return t->id;
		}
	}
	return 0;
}

int32_t
mappa_target_remove(struct mappa_t *m, uint32_t target_id)
{
	struct target_t *t;
	TAILQ_FOREACH(t, &m->target_list, tailq) {
		if(t->id == target_id) {
			/* TODO: nuke any controller mappings here */
			TAILQ_REMOVE(&m->target_list, t, tailq);
			target_destroy(t);
			return 0;
		}
	}

	return -EINVAL;
}

void
mappa_remove_func(struct ctlra_dev_t *dev, int unexpected_removal,
		  void *userdata)
{
	struct ctlra_dev_info_t info;
	ctlra_dev_get_info(dev, &info);
	printf("mappa_app: removing %s %s\n", info.vendor, info.device);

	struct mappa_t *m = userdata;
	/* TODO: cleanup dev_t * and resources here */
	//m->dev = 0x0;
}

#define DEV_FROM_MAPPA(m, dev, find_id) do {				\
	dev = 0;							\
	struct dev_t *__d = 0;						\
	TAILQ_FOREACH(__d, &m->dev_list, tailq) {			\
		if(__d->id == find_id) {				\
			dev = __d; break;				\
		}							\
	}								\
	if(!dev) {							\
		printf("%s invalid dev id %d\n", __func__, ctlra_dev_id);\
		return -EINVAL;						\
	}								\
} while(0)

int32_t mappa_bind_ctlra_to_target(struct mappa_t *m,
				   uint32_t ctlra_dev_id,
				   uint32_t layer,
				   uint32_t control_type,
				   uint32_t control_id,
				   uint32_t target_id)
{
	if(!m)
		return -EINVAL;

	/* search for dev */
	struct dev_t *dev = 0;
	DEV_FROM_MAPPA(m, dev, ctlra_dev_id);

	/* error check control type/id combo */
	if(control_id >= dev->ctlra_dev_info.control_count[control_type] ||
			control_type >= CTLRA_EVENT_T_COUNT) {
		printf("%s: control id %d out of range for type %d, dev %s\n",
		       __func__, control_id, control_type,
		       dev->ctlra_dev_info.vendor);
		return -EINVAL;
	}

	/* search for correct lut layer */
	struct lut_t *l;
	struct lut_t *lut = 0;
	TAILQ_FOREACH(l, &dev->lut_list, tailq) {
		if(l->id == layer) {
			lut = l;
			break;
		}
	}
	if(lut == 0) {
		printf("invalid layer %d requested for binding\n", layer);
		return -EINVAL;
	}

	struct target_t *t;
	TAILQ_FOREACH(t, &m->target_list, tailq) {
		if(t->id == target_id) {
			/* point the lut event type:id combo at the target */
			lut->target_types[control_type][control_id] = t;
			/*
			printf("bound layer %d, ctype %d cid %d to %s\n",
			       layer, control_type, control_id, t->target.name);
			*/
			return 0;
		}
	}

	return -EINVAL;
}

int32_t
mappa_bind_source_to_ctlra(struct mappa_t *m, uint32_t ctlra_dev_id,
			   uint32_t layer, uint32_t fb_id,
			   uint32_t source_id)
{
	struct dev_t *dev = 0;
	DEV_FROM_MAPPA(m, dev, ctlra_dev_id);

	if(fb_id >= dev->ctlra_dev_info.control_count[CTLRA_FEEDBACK_ITEM]) {
		printf("%s() invalid fb_id\n", __func__);
		return -EINVAL;
	}

	/* iterate over sources by name, compare */
	int found = 0;
	struct source_t *s;
	TAILQ_FOREACH(s, &m->source_list, tailq) {
		if(s->id == source_id) {
			found = 1;
			break;
		}
	}
	if(!found)
		return -EINVAL;

	/* search for correct lut layer */
	struct lut_t *l;
	found = 0;
	TAILQ_FOREACH(l, &dev->lut_list, tailq) {
		if(l->id == layer) {
			found = 1;
			break;
		}
	}
	if(!found)
		return -EINVAL;

	/* set source to feed into the feedback id */
	l->sources[fb_id] = &s->source;

	/*
	int count = dev->ctlra_dev_info.control_count[CTLRA_FEEDBACK_ITEM];
	for(int i = 0; i < count; i++) {
		struct mappa_source_t *s = l->sources[i];
		if(s)
			printf("postbind i %d, s %p, func %p\n", i, s, s->func);
	}
	*/

	return 0;
}

void
lut_destroy(struct lut_t *lut)
{
	for(int i = 0; i < CTLRA_EVENT_T_COUNT; i++) {
		if(lut->target_types[i])
			free(lut->target_types[i]);
	}
	if(lut->sources)
		free(lut->sources);
	free(lut);
}

int32_t
lut_create_add_to_dev(struct dev_t *dev, const struct ctlra_dev_info_t *info)
{
	struct lut_t * lut = calloc(1, sizeof(*lut));
	if(!lut)
		return -ENOMEM;

	/* allocate event input to target lookup arrays */
	for(int i = 0; i < CTLRA_EVENT_T_COUNT; i++) {
		uint32_t c = info->control_count[i];
		lut->target_types[i] = calloc(c, sizeof(void *));
		/*
		printf("lut %p type %d, size %d, ptr %p\n", lut, i, c,
		       lut->target_types[i]);
		*/
		if(!lut->target_types[i])
			goto fail;
	}

	/* allocate feedback item lookup array */
	int items = info->control_count[CTLRA_FEEDBACK_ITEM];
	lut->sources = calloc(items, sizeof(struct mappa_source_t));
	if(!lut->sources)
		goto fail;

	/* set index, append to list of LUTs */
	lut->id = dev->lut_idx++;
	TAILQ_INSERT_TAIL(&dev->lut_list, lut, tailq);
	return 0;

fail:
	lut_destroy(lut);
	return -ENOMEM;
}

struct dev_t *
dev_create(struct mappa_t *m, struct ctlra_dev_t *ctlra_dev,
	   const struct ctlra_dev_info_t *info)
{
	struct dev_t *dev = calloc(1, sizeof(*dev));
	if(!dev)
		return 0;

	TAILQ_INIT(&dev->lut_list);

	/* store the dev info into the struct, for error checking later */
	dev->ctlra_dev_info = *info;

	/* add LUT to this dev */
	int32_t ret = lut_create_add_to_dev(dev, info);
	if(ret)
		printf("error ret from lut_create_add_to_dev: %d\n", ret);

	ret = lut_create_add_to_dev(dev, info);
	if(ret)
		printf("error ret from 2ND lut_create_add_to_dev: %d\n", ret);

	dev->self = m;
	dev->active_lut = TAILQ_FIRST(&dev->lut_list);

	dev->id = m->dev_ids++;
	TAILQ_INSERT_TAIL(&m->dev_list, dev, tailq);
	return dev;
}

void
dev_destroy(struct dev_t *dev)
{
	/* cleanup all LUTs */
	struct lut_t *l;
	while (!TAILQ_EMPTY(&dev->lut_list)) {
		l = TAILQ_FIRST(&dev->lut_list);
		TAILQ_REMOVE(&dev->lut_list, l, tailq);
		lut_destroy(l);
	}

	/* cleanup the sources */
	free(dev);
}

int
mappa_accept_func(struct ctlra_t *ctlra, const struct ctlra_dev_info_t *info,
		  struct ctlra_dev_t *ctlra_dev, void *userdata)
{
	struct mappa_t *m = userdata;

	struct dev_t *dev = dev_create(m, ctlra_dev, info);
	if(!dev) {
		printf("failed to create dev\n");
		return 0;
	}

	ctlra_dev_set_event_func(ctlra_dev, mappa_event_func);
	ctlra_dev_set_feedback_func(ctlra_dev, mappa_feedback_func);
	ctlra_dev_set_remove_func(ctlra_dev, mappa_remove_func);

	/* the callback here is set per *DEVICE* - NOT the mappa pointer!
	 * The struct being passed is used directly to avoid lookup of the
	 * correct device from a list. The structure has a mappa_t back-
	 * pointer in order to communicate with "self" if required.
	 */
	ctlra_dev_set_callback_userdata(ctlra_dev, dev);

	/* TODO: check for default map to load for this device */
	printf("accepting dev %s\n", info->vendor);

	return 1;
}

int32_t
mappa_iter(struct mappa_t *m)
{
	ctlra_idle_iter(m->ctlra);
	return 0;
}

/* int cast of the float value will indicate the layer to switch to */
void mappa_layer_switch_target(uint32_t target_id, float value,
			       void *token, uint32_t token_size,
			       void *userdata)
{
	struct mappa_t *m = userdata;
	struct dev_t *dev = TAILQ_FIRST(&m->dev_list);
	int layer = (int)value;
	//printf("%s, layer = %d\n", __func__, layer);
	/* search for correct lut layer */
	struct lut_t *l;
	TAILQ_FOREACH(l, &dev->lut_list, tailq) {
		if(l->id == layer) {
			dev->active_lut = l;
			printf("  layer %d active!\n", layer);
			break;
		}
	}

	/* TODO: how to notify of failure to switch? */
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

	/* 0 is the error return for uint32_t functions, so offset
	 * all targets by 1. They don't have an absolute meaning, so no
	 * issue in doing this */
	m->target_ids = 1;
	m->source_ids = 1;

	m->ctlra = c;

	TAILQ_INIT(&m->target_list);
	TAILQ_INIT(&m->dev_list);

	/* push back "internal" targets like layer switching */
	struct mappa_target_t t = {
		.name = "mappa:layer switch",
		.func = mappa_layer_switch_target,
		.userdata = m,
	};

	uint32_t sid;
	int ret = mappa_target_add(m, &t, &sid, 0, 0);
	assert(ret == 0);

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
	struct target_t *t;
	while (!TAILQ_EMPTY(&m->target_list)) {
		t = TAILQ_FIRST(&m->target_list);
		TAILQ_REMOVE(&m->target_list, t, tailq);
		target_destroy(t);
	}

	struct source_t *s;
	while (!TAILQ_EMPTY(&m->source_list)) {
		s = TAILQ_FIRST(&m->source_list);
		TAILQ_REMOVE(&m->source_list, s, tailq);
		source_destroy(s);
	}

	struct dev_t *d;
	while (!TAILQ_EMPTY(&m->dev_list)) {
		d = TAILQ_FIRST(&m->dev_list);
		TAILQ_REMOVE(&m->dev_list, d, tailq);
		dev_destroy(d);
	}

	/* iterate over all allocated resources and free them */
	ctlra_exit(m->ctlra);
	free(m);
}

static int32_t
bind_config_to_target(struct mappa_t *m, uint32_t dev_id, uint32_t layer,
		      uint32_t type, uint32_t cid, const char *target)
{
	/* lookup target, map id */
	uint32_t tid = mappa_get_target_id(m, target);
	if(tid == 0)
		return -EINVAL;

	int ret = mappa_bind_ctlra_to_target(m, dev_id, layer, type, cid, tid);
	assert(ret == 0);
	return 0;
}

int32_t
mappa_load_bindings(struct mappa_t *m, const char *file)
{
	ini_t *config = ini_load("mappa_z1.ini");
	if(!config)
		return -EINVAL;

	const char *name = 0;
	const char *email = 0;
	const char *org = 0;
	ini_sget(config, "author", "name", NULL, &name);
	ini_sget(config, "author", "email", NULL, &email);
	ini_sget(config, "author", "organization", NULL, &org);
	printf("Loading config file %s\n", file);
	printf("  Name: %s\n", name);
	printf("  Email: %s\n", email);
	printf("  Org: %s\n", org);

	/* TODO: lookup dev id by vendor/device/serial */
	int dev = 0;

	for(int l = 0; l < 5; l++) {
		char layer[32];
		snprintf(layer, sizeof(layer), "layer.%u", l);
		for(int i = 0; i < 100; i++) {
			char key[32];
			snprintf(key, sizeof(key), "slider.%u", i);
			const char *value = 0;
			ini_sget(config, layer, key, NULL, &value);
			/* TODO: can we error check this better? */
			if(!value)
				continue;
			int ret = bind_config_to_target(m, dev, l,
							CTLRA_EVENT_SLIDER,
							i, value);

			/* buttons */
			snprintf(key, sizeof(key), "button.%u", i);
			value = 0;
			ini_sget(config, layer, key, NULL, &value);
			if(!value)
				continue;
			ret = bind_config_to_target(m, dev, l,
						    CTLRA_EVENT_BUTTON,
						    i, value);

			snprintf(key, sizeof(key), "light.%u", i);
			value = 0;
			ini_sget(config, layer, key, NULL, &value);
			if(!value)
				continue;
			uint32_t sid = mappa_get_source_id(m, value);
			if(sid == 0) {
				printf("invalid source %s\n", value);
				continue;
			}
			ret = mappa_bind_source_to_ctlra(m, 0, l, i, sid);
		}
	}

	ini_free(config);

	return 0;
}
