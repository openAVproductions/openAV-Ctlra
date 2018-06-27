
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
		printf("mappa programming error - dev is NULL\n");
		return;
	}
	struct mappa_t *m = dev->self;

	struct lut_t *lut = dev->active_lut;
	if(!lut) {
		MAPPA_ERROR(m, "programming error - lut is %p\n", lut);
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
			MAPPA_ERROR(m,
				"ctlra device error: type %d id %d >= control count %d\n",
				e->type, id, count);
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
	return 0;
}

int32_t
mappa_source_remove(struct mappa_t *m, uint32_t source_id)
{
	struct source_t *s;
	TAILQ_FOREACH(s, &m->source_list, tailq) {
		if(s->id == source_id) {
			/* TODO: release any source bindings here */
			TAILQ_REMOVE(&m->source_list, s, tailq);
			source_destroy(s);
			return 0;
		}
	}
	return -EINVAL;
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

void dev_destroy(struct dev_t *dev);

void
mappa_remove_func(struct ctlra_dev_t *ctlra_dev, int unexpected_removal,
		  void *userdata)
{
	struct dev_t *mappa_dev = userdata;
	struct mappa_t *m = mappa_dev->self;

	struct ctlra_dev_info_t info;
	ctlra_dev_get_info(ctlra_dev, &info);
	MAPPA_INFO(m, "mappa_app: removing %s %s\n", info.vendor, info.device);

	/* remove dev from tailq and free it */
	TAILQ_REMOVE(&m->dev_list, mappa_dev, tailq);
	dev_destroy(mappa_dev);
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
		MAPPA_ERROR(m, "%s invalid dev id %d\n", __func__, find_id);\
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
		MAPPA_WARN(m, "%s: control id %d out of range for type %d, dev %s\n",
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
		MAPPA_WARN(m, "invalid layer %d requested for binding\n", layer);
		return -EINVAL;
	}

	struct target_t *t;
	TAILQ_FOREACH(t, &m->target_list, tailq) {
		if(t->id == target_id) {
			/* point the lut event type:id combo at the target */
			lut->target_types[control_type][control_id] = t;
			return 0;
		}
	}

	MAPPA_WARN(m,"target id %d not found\n", target_id);

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
		MAPPA_ERROR(m, "feedback id %d is out of range\n", fb_id);
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

	return 0;
}

void
lut_destroy(struct lut_t *lut)
{
	for(int i = 0; i < CTLRA_EVENT_T_COUNT; i++) {
		if(lut->target_types[i])
			free(lut->target_types[i]);
	}
	if(lut->name)
		free(lut->name);
	if(lut->sources)
		free(lut->sources);
	free(lut);
}

struct lut_t *
lut_create(struct dev_t *dev, const struct ctlra_dev_info_t *info)
{
	struct lut_t * lut = calloc(1, sizeof(*lut));
	if(!lut)
		return 0;

	/* allocate event input to target lookup arrays */
	for(int i = 0; i < CTLRA_EVENT_T_COUNT; i++) {
		uint32_t c = info->control_count[i];
		lut->target_types[i] = calloc(c, sizeof(void *));
		if(!lut->target_types[i])
			goto fail;
	}

	/* allocate feedback item lookup array */
	int items = info->control_count[CTLRA_FEEDBACK_ITEM];
	lut->sources = calloc(items, sizeof(struct mappa_source_t));
	if(!lut->sources)
		goto fail;
	return lut;

fail:
	lut_destroy(lut);
	return 0;
}

int32_t lut_create_add_to_dev(struct mappa_t *m,
			      struct dev_t *dev,
			      const struct ctlra_dev_info_t *info)
{
	/* add LUT to this dev */
	struct lut_t *lut = lut_create(dev, info);
	if(!lut) {
		MAPPA_ERROR(m, "lut create failed, returned %p\n", lut);
		return -ENOMEM;
	}
	lut->id = dev->lut_idx++;
	TAILQ_INSERT_TAIL(&dev->lut_list, lut, tailq);
	return 0;
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

	int ret = lut_create_add_to_dev(m, dev, info);
	if(ret) {
		MAPPA_ERROR(m, "lut create_add_to_dev() ret %d\n", ret);
	}
	ret = lut_create_add_to_dev(m, dev, info);
	if(ret) {
		MAPPA_ERROR(m, "lut create_add_to_dev() ret %d\n", ret);
	}

	/* allocate memory for the the "flattened" lut of all the active
	 * layers */
	dev->active_lut = lut_create(dev, info);

	dev->self = m;
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
		MAPPA_ERROR(m, "failed to create mappa dev_t for %s %s\n",
			    info->vendor, info->device);
		return 0;
	}

	/* TODO: check all available mappa config files to see if we want
	 * to accept this device? Use mappa_opts to configure this?
	 */

	ctlra_dev_set_event_func(ctlra_dev, mappa_event_func);
	ctlra_dev_set_feedback_func(ctlra_dev, mappa_feedback_func);
	ctlra_dev_set_remove_func(ctlra_dev, mappa_remove_func);

	/* the callback here is set per *DEVICE* - NOT the mappa pointer!
	 * The struct being passed is used directly to avoid lookup of the
	 * correct device from a list. The structure has a mappa_t back-
	 * pointer in order to communicate with "self" if required.
	 */
	ctlra_dev_set_callback_userdata(ctlra_dev, dev);

	MAPPA_INFO(m, "accepting device %s %s\n", info->vendor, info->device);
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

	struct lut_t *l;
	TAILQ_FOREACH(l, &dev->lut_list, tailq) {
		if(l->id == layer) {
			dev->active_lut = l;
			return;
		}
	}

	MAPPA_ERROR(m, "layer %d not found, switch failed\n", layer);
}

struct mappa_t *
mappa_create(struct mappa_opts_t *opts)
{
	(void)opts;
	struct mappa_t *m = calloc(1, sizeof(struct mappa_t));
	if(!m)
		goto fail;

	if(opts)
		m->opts = *opts;

	char *mappa_debug = getenv("MAPPA_DEBUG");
	if(mappa_debug) {
		int debug_level = atoi(mappa_debug);
		m->opts.debug_level = debug_level;
		MAPPA_INFO(m, "debug level: %d\n", debug_level);
	}

	struct ctlra_create_opts_t c_opts = {
		.debug_level = m->opts.debug_level,
	};
	struct ctlra_t *c = ctlra_create(&c_opts);
	if(!c)
		goto fail;
	m->ctlra = c;

	/* 0 is the error return for uint32_t functions, so offset
	 * all targets by 1. They don't have an absolute meaning, so no
	 * issue in doing this */
	m->target_ids = 1;
	m->source_ids = 1;

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
	if(ret != 0)
		MAPPA_ERROR(m, "mappa target add %s failed\n", t.name);

	int num_devs = ctlra_probe(c, mappa_accept_func, m);
	MAPPA_INFO(m, "mappa connected to %d devices\n", num_devs);

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
	return -1;
}

int32_t
mappa_load_bindings(struct mappa_t *m, const char *file)
{
	ini_t *config = ini_load(file);
	if(!config)
		return -EINVAL;

	const char *name = 0;
	const char *email = 0;
	const char *org = 0;
	ini_sget(config, "author", "name", NULL, &name);
	ini_sget(config, "author", "email", NULL, &email);
	ini_sget(config, "author", "organization", NULL, &org);
	MAPPA_INFO(m, "Loading config file %s\n", file);
	MAPPA_INFO(m, "  Name: %s\n", name);
	MAPPA_INFO(m, "  Email: %s\n", email);
	MAPPA_INFO(m, "  Org: %s\n", org);

	/* TODO: lookup dev id by vendor/device/serial */
	struct dev_t *dev = TAILQ_FIRST(&m->dev_list);

	for(int l = 0; l < 5; l++) {
		const char *value = 0;
		uint32_t errors = 0;
		char layer[32];
		snprintf(layer, sizeof(layer), "layer.%u", l);

		/* get layer name: layers without names are invalid */
		ini_sget(config, layer, "name", NULL, &value);
		if(!value)
			continue;
		printf("layer name %s\n", value);

		/* check if layer exists, free it if yes */
		/* recreate a lut for this layer */
		struct lut_t *lut;
		TAILQ_FOREACH(lut, &dev->lut_list, tailq) {
			if(!lut->name) {
				MAPPA_ERROR(m, "lut %p has no name\n", lut);
				return -ENOBUFS;
			}
			if(strcmp(lut->name, value) != 0) {
				lut_destroy(lut);
				break;
			}
		}
		int ret = lut_create_add_to_dev(m, dev, &dev->ctlra_dev_info);
		if(!ret)
			return -ENOMEM;
		/* fresh clean lut available now */
		lut->name = strdup(value);

		/* strdup name into lut->name */

		value = 0;
		ini_sget(config, layer, "active_on_load", NULL, &value);
		if(value) {
			int active = atoi(value);
			printf("  active on load: %d\n", active);
		}

		for(int i = 0; i < 100; i++) {
			int ret;
			char key[32];

			/* sliders */
			snprintf(key, sizeof(key), "slider.%u", i);
			ini_sget(config, layer, key, NULL, &value);
			if(value) {
				ret = bind_config_to_target(m, dev->id, l,
							    CTLRA_EVENT_SLIDER,
							    i, value);
				if(ret != 0)
					errors++;
			}

			/* buttons */
			snprintf(key, sizeof(key), "button.%u", i);
			value = 0;
			ini_sget(config, layer, key, NULL, &value);
			if(value) {
				ret = bind_config_to_target(m, dev->id, l,
							    CTLRA_EVENT_BUTTON,
							    i, value);
				if(ret != 0) {
					MAPPA_WARN(m, "error mapping button %s\n", value);
					errors++;
				}
			}

			/* lights */
			snprintf(key, sizeof(key), "light.%u", i);
			value = 0;
			ini_sget(config, layer, key, NULL, &value);
			if(value) {
				uint32_t sid = mappa_get_source_id(m, value);
				if(ret != 0) {
					MAPPA_WARN(m, "invalid light id %s\n",
						   value);
					errors++;
				} else {
					ret = mappa_bind_source_to_ctlra(m, 0,
							 l, i, sid);
					if(ret != 0) {
						MAPPA_WARN(m, "bind source %s failed\n",
							   value);
						errors++;
					}
				}
			}
		}

		if(errors)
			MAPPA_ERROR(m, "error mapping control on layer %d, count %d\n",
				    l, errors);
	}

	ini_free(config);

	return 0;
}
