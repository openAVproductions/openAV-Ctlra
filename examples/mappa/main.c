#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "mappa.h"
#include "ini.h"

static volatile uint32_t done;

void sighndlr(int signal)
{
	done = 1;
	printf("\n");
}

static float the_value;

void sw_source_float_func_1(float *value,
			    void *token,
			    uint32_t token_size,
			    void *userdata)
{
	*value = the_value;
}

void sw_source_float_func_2(float *value,
			    void *token,
			    uint32_t token_size,
			    void *userdata)
{
	*value = 0.2;
}

/* 32 bytes of data for testing */
uint64_t token_test[] = {0x1234, 0x5678, 0xc0ffee, 0xcafe};

void sw_target_float_func(uint32_t target_id,
			  float value,
			  void *token,
			  uint32_t token_size,
			  void *userdata)
{
#if 1
	printf("%s: target id %d, value %f, token size %d\n",
	       __func__, target_id, value, token_size);
#endif
	if(token_size == 8) {
		uint64_t t = *(uint64_t *)token;
		assert(t == 0xcafe);
		assert(token_size == sizeof(uint64_t));
	} else if (token_size == sizeof(token_test)) {
		assert(memcmp(token, token_test, sizeof(token_test)) == 0);
	}
	the_value = value;
}

void tests(void);

int32_t
register_targets(struct mappa_t *m, void *userdata)
{
	int ret;

	struct mappa_target_t t = {
		.name = "t_1",
		.func = sw_target_float_func,
		.userdata = 0x0,
	};
	uint32_t tid;
	ret = mappa_target_add(m, &t, &tid, 0, 0);
	assert(ret == 0);

	int dev = 0;
	int layer = 0;
	int ctype = CTLRA_EVENT_SLIDER;
	int cid = 0;
	tid = mappa_get_target_id(m, "t_1");
	assert(tid != 0);
	ret = mappa_bind_ctlra_to_target(m, dev, layer, ctype, cid, tid);
	if(ret != 0)
		printf("MAP %d failed: cid %d\n", __LINE__, cid);

	/* target 2 */
	t.name = "t_2";
	uint64_t token = 0xcafe;
	ret = mappa_target_add(m, &t, &tid, &token, sizeof(uint64_t));
	assert(ret == 0);
	cid = 13;
	tid = mappa_get_target_id(m, "t_2");
	assert(tid != 0);
	ret = mappa_bind_ctlra_to_target(m, dev, layer, ctype, cid, tid);
	if(ret != 0)
		printf("MAP failed: cid %d\n", cid);

	t.name = "t_3";
	ret = mappa_target_add(m, &t, &tid, &token_test, sizeof(token_test));
	assert(ret == 0);
	cid = 12;
	tid = mappa_get_target_id(m, "t_3");
	assert(tid != 0);
	ret = mappa_bind_ctlra_to_target(m, dev, layer, ctype, cid, tid);
	if(ret != 0)
		printf("MAP failed: cid %d\n", cid);

	return 0;
}

int32_t
bind_from_file(struct mappa_t *m, const char *file)
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
	printf("\n\nConfig file:\nname: %s\nemail: %s\norg: %s\n", name, email, org);

	for(int l = 0; l < 5; l++) {
		char layer[32];
		snprintf(layer, sizeof(layer), "layer.%u", l);
		printf("Layer %u:\n", l);
		for(int i = 0; i < 100; i++) {
			char key[32];
			snprintf(key, sizeof(key), "slider.%u", i);

			const char *value = 0;
			ini_sget(config, layer, key, NULL, &value);

			/* TODO: can we error check this better? */
			if(!value)
				continue;

			/* lookup target, map id */
			uint32_t tid = mappa_get_target_id(m, value);
			if(tid == 0) {
				printf("invalid target %s, failed to map\n", value);
				continue;
			}

			int ret;
			ret = mappa_bind_ctlra_to_target(m, 0, 0, CTLRA_EVENT_SLIDER,
							 i, tid);
			printf("mapping layer %d slider %d to %s, tid = %d\n",
			       l, i, value, tid);
			assert(ret == 0);

		}
	}

	ini_free(config);


	return 0;
}

int32_t
bind_stuff(struct mappa_t *m, void *userdata)
{
	int dev = 0;
	int ctype = CTLRA_EVENT_BUTTON;
	int layer = 0;
	int cid = 0;
	int tid = mappa_get_target_id(m, "mappa:layer switch");
	int ret = mappa_bind_ctlra_to_target(m, dev, layer, ctype, cid, tid);
	assert(ret == 0);

	layer = 1;
	cid = 1;
	ret = mappa_bind_ctlra_to_target(m, dev, layer, ctype, cid, tid);
	assert(ret == 0);

{
	/****** Feedback ******/
	int dev = 0;
	int layer = 0;
	struct mappa_source_t fb = {
		.name = "test_fb_1",
		.func = sw_source_float_func_1,
		.userdata = 0,
	};
	uint32_t source_id;
	ret = mappa_source_add(m, &fb, &source_id, 0, 0);
	if(ret != 0)
		printf("MAP %d failed: source name %s\n", __LINE__, fb.name);

	ret = mappa_bind_source_to_ctlra(m, dev, layer, 0, source_id);
	if(ret != 0)
		printf("MAP %d failed: sid %d\n", __LINE__, source_id);

	/**** FB item 2 */
	fb.name = "test fb 2";
	fb.func = sw_source_float_func_2;
	ret = mappa_source_add(m, &fb, &source_id, 0, 0);
	if(ret != 0)
		printf("MAP %d failed: sid %d\n", __LINE__, source_id);
	layer = 1;
	ret = mappa_bind_source_to_ctlra(m, dev, layer, 0, source_id);
	if(ret != 0)
		printf("MAP %d failed: sid %d\n", __LINE__, source_id);
}

	return 0;
}

int main(int argc, char **argv)
{
	//tests();

	int ret;
	signal(SIGINT, sighndlr);

	/* create mappa context */
	struct mappa_t *m = mappa_create(NULL);
	if(!m)
		return -1;

	register_targets(m, 0x0);

	ret = bind_from_file(m, "mappa_z1.ini");
	assert(ret == 0);

	/* loop for testing */
	while(!done) {
		mappa_iter(m);
		usleep((1000 * 1000) / 60);
	}

	mappa_destroy(m);

	return 0;
}

void tests(void)
{
	int ret = 0;
	ret = setenv("CTLRA_VIRTUAL_VENDOR", "Native Instruments", 1);
	printf("errno %d\n", errno);
	assert(!ret);
	ret = setenv("CTLRA_VIRTUAL_DEVICE", "Kontrol Z1", 1);
	//ret = setenv("CTLRA_VIRTUAL_DEVICE", "Maschine Mikro Mk2", 1);
	printf("errno %d\n", errno);
	assert(!ret);

	struct mappa_t *m = mappa_create(NULL);
	assert(m);


	mappa_destroy(m);
}
