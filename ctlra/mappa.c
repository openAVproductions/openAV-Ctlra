
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
	const struct ctlra_dev_info_t *info = dev->ctlra_dev_info;
	int count = info->control_count[CTLRA_FEEDBACK_ITEM];
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
		/* TODO: move to above loop */
		const struct ctlra_dev_info_t *info = dev->ctlra_dev_info;
		const int count = info->control_count[e->type];
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

int32_t
mappa_bind_source_to_ctlra(struct mappa_t *m, uint32_t ctlra_dev_id,
			   const char *layer, uint32_t fb_id,
			   uint32_t source_id)
{
	struct dev_t *dev = 0;
	DEV_FROM_MAPPA(m, dev, ctlra_dev_id);

	const struct ctlra_dev_info_t *info = dev->ctlra_dev_info;
	int control_count = info->control_count[CTLRA_EVENT_BUTTON];
	if(fb_id >= control_count) {
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
		if(strcmp(l->name, layer) == 0) {
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
lut_create(struct dev_t *dev, const struct ctlra_dev_info_t *info,
	   const char *name)
{
	struct lut_t * lut = calloc(1, sizeof(*lut));
	if(!lut) {
		printf("failed to calloc lut mem\n");
		return 0;
	}

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

	lut->name = strdup(name);
	return lut;

fail:
	MAPPA_ERROR(dev->self, "failed to create LUT for %s %s\n",
		    info->vendor, info->device);
	lut_destroy(lut);
	return 0;
}

struct lut_t *
lut_create_add_to_dev(struct mappa_t *m,
			      struct dev_t *dev,
			      const struct ctlra_dev_info_t *info,
			      const char *name)
{
	/* add LUT to this dev */
	struct lut_t *lut = lut_create(dev, info, name);
	if(!lut) {
		MAPPA_ERROR(m, "lut create failed, returned %p\n", lut);
		return 0;
	}
	TAILQ_INSERT_TAIL(&dev->lut_list, lut, tailq);
	return lut;
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
	dev->ctlra_dev_info = info;

	/* allocate memory for the the "flattened" lut of all the active
	 * layers */
	dev->active_lut = lut_create(dev, info, "active_lut");

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

	/* cleanup the active "flattened" lut */
	lut_destroy(dev->active_lut);

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

	/* TODO: extract layer name from token */
	const char *layer = "todo: fixme invalid";
	MAPPA_ERROR(m, "TODO: fix layer token lookup %s\n", "here");

	struct lut_t *l;
	TAILQ_FOREACH(l, &dev->lut_list, tailq) {
		if(strcmp(l->name, layer) == 0) {
			dev->active_lut = l;
			return;
		}
	}

	MAPPA_ERROR(m, "layer %s not found, switch failed\n", layer);
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

	/* TODO: Delay creation of targets until the layer becomes
	 * available. Make it possible to switch to it based on (dev,name)
	 * pair, as the layer applies only to a specific device instance,
	 * not any device in use.
	 *
	 * This allows a layer switch to be a "trigger" without args,
	 * avoiding any string parse/handling, or integer enumeration of
	 * layers.
	 *
	 * Internally, the target can register a token that will switch to
	 * the correct layer */
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
config_file_binding_create(struct dev_t *dev, struct lut_t *lut,
			   uint32_t event_type, uint32_t control_id,
			   const char *layer, const char *control_name)
{
	const char *target = 0;
	struct mappa_t *m = dev->self;
	ini_t *config = m->ini_file;
	ini_sget(config, layer, control_name, NULL, &target);
	if(!target) {
		MAPPA_WARN(m, "Failed to bind %s %s to target %s\n",
			   layer, control_name, target);
		return 1;
	}

	/* Lookup the target in the mappa target list, create binding
	 * to target from the layer's lut. Note this is *NOT* the same
	 * as programming something to dev->active_lut. */
	struct target_t *t;
	TAILQ_FOREACH(t, &m->target_list, tailq) {
		if(strcmp(t->target.name, target) == 0) {
			lut->target_types[event_type][control_id] = t;
			MAPPA_INFO(m, "success %s %s => %s\n", layer,
				   control_name, target);
			return 0;
		}
	}

	/* TODO: think about how to handle this better. What if the app
	 * doesn't expose a target *YET* - but it could add it dynamically
	 * later. In that case we don't want to reload the whole file
	 * (overhead) - so storing the target name, but marking as invalid
	 * (null callback?) might be enough. When an app adds a target, we
	 * have to "re-walk" all the luts that are currently active, and
	 * fold them down to the active_lut again - but that's less work
	 * than re-loading the file, but gives the user the plug-and-play
	 * experience that is required.
	 *
	 * Impl: instead of having the app call a rebind() function, or
	 * performing it every time a target is added (overhead), use a
	 * two counter approach, and when an event arrives and if the
	 * target_add_counter != event_seen_counter, rebind */
	MAPPA_WARN(m,"target %s not found\n", target);
	return 1;
}

static int32_t
config_file_bind_layer_type(struct dev_t *dev, struct lut_t *lut,
			    const char *layer, uint32_t event_type)
{
	struct mappa_t *m = dev->self;
	/* for this event type, pull out control_count, and iterate over
	 * them. Pull the "layer", "name" pair from the config file, and
	 * call a function to map the control if it was non-NULL */
	const struct ctlra_dev_info_t *info = dev->ctlra_dev_info;
	uint32_t control_count = info->control_count[event_type];
	int32_t errors = 0;
	const uint32_t buf_size = 256;
	char buf[buf_size];

	for(uint32_t i = 0; i < control_count; i++) {
		const char *control_name = ctlra_info_get_name(info,
							       event_type,
							       i);
		if(!control_name) {
			MAPPA_ERROR(m, "control type %d, index %d of device %s %s has no name\n",
				    event_type, i, info->vendor, info->device);
			continue;
		}
		const char *prefix = ctlra_event_type_names[event_type];
		int written = snprintf(buf, sizeof(buf), "%s.%s",
				       prefix, control_name);
		if(written < 0 || written >= buf_size) {
			MAPPA_ERROR(m, "failed to print control name %s with prefix %s\n",
				    control_name, prefix);
			continue;
		}

		int ret = config_file_binding_create(dev, lut, event_type,
						     i, layer, buf);
		errors += !!ret;
	}

	return errors;
}

/* returns 0 on success, or # of FAILED bindings otherwise */
static int32_t
config_file_bind_layer(struct mappa_t *m, struct dev_t *dev,
		       struct lut_t *lut, const char *layer)
{
	/* check the event type max, and pull the name of each control
	 * item from the Ctlra device code. Call a function to set up
	 * the mapping */
	int32_t total = 0;
	int32_t errors = 0;

	/* handle all event types here */
	const struct ctlra_dev_info_t *info = dev->ctlra_dev_info;
	uint32_t event_type_max = CTLRA_EVENT_T_COUNT;
	for(int e = 0; e < CTLRA_EVENT_T_COUNT; e++) {
		total += info->control_count[e];
		errors += config_file_bind_layer_type(dev, lut, layer, e);
	}

	if(errors)
		MAPPA_WARN(m, "Layer %s: %d bind errors, %d succesful bindings\n",
			   layer, errors, total - errors);

	return errors ? -1 : 0;
}

int32_t
mappa_load_bindings(struct mappa_t *m, const char *file)
{
	if(!m)
		return -EINVAL;

	/* reset state first */
	m->ini_file = NULL;

	ini_t *config = ini_load(file);
	if(!config)
		return -EINVAL;

	m->ini_file = config;

	/*
	const char *name = 0;
	const char *email = 0;
	const char *org = 0;
	ini_sget(config, "author", "name", NULL, &name);
	ini_sget(config, "author", "email", NULL, &email);
	ini_sget(config, "author", "organization", NULL, &org);
	*/

	/* TODO: lookup dev id by vendor/device/serial */
	struct dev_t *dev = TAILQ_FIRST(&m->dev_list);
	if(!dev)
		return -ENOSPC;

	int layer_count = 1;
	for(int i = 0; i < layer_count; i++) {
		/* TODO: destroy and re-create a lut for this layer */
		char layer_id[32];
		snprintf(layer_id, sizeof(layer_id), "layer.%u", i);
		const char *layer_name = 0;
		ini_sget(config, layer_id, "name", NULL, &layer_name);
		if(!layer_name) {
			MAPPA_ERROR(m, "Layer %d has no name - skipping\n", i);
			continue;
		}

		/* (re)create the lut for this layer */
		struct lut_t *lut = 0;
		TAILQ_FOREACH(lut, &dev->lut_list, tailq) {
			assert(lut->name);
			if(strcmp(lut->name, layer_name) == 0) {
				lut_destroy(lut);
				break;
			}
		}
		lut = lut_create_add_to_dev(m, dev, dev->ctlra_dev_info,
					    layer_name);
		if(!lut) {
			MAPPA_ERROR(m, "failed to create lut %s\n", "test");
			return -ENOMEM;
		}

		/* call "config file probe" function to pull ctlra names
		 * out of the device code, and attempt to map target */
		int ret = config_file_bind_layer(m, dev, lut, layer_id);
		if(ret) {
			MAPPA_ERROR(m, "layer %s had binding failures\n",
				    layer_name);
		}
	}

	/* free and reset temporary handle */
	if(m->ini_file)
		ini_free(m->ini_file);
	m->ini_file = 0;

	return 0;
}
