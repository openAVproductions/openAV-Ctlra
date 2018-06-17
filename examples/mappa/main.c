#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "mappa.h"

static volatile uint32_t done;

void sighndlr(int signal)
{
	done = 1;
	printf("\n");
}

void sw_source_float_func_1(float *value,
			    void *token, uint32_t token_size,
			    void *userdata)
{
	*value = 1;
}

void sw_source_float_func_2(float *value,
			    void *token, uint32_t token_size,
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
	printf("%s: target id %d, value %f, token size %d\n", __func__,
	       target_id, value, token_size);
	if(token_size == 8) {
		uint64_t t = *(uint64_t *)token;
		assert(t == 0xcafe);
		assert(token_size == sizeof(uint64_t));
	} else if (token_size == sizeof(token_test)) {
		assert(memcmp(token, token_test, sizeof(token_test)) == 0);
		uint64_t *td = token;
		printf("%lx\n", td[3]);
	}
}

void tests(void);

int32_t
bind_callback(struct mappa_t *m, void *userdata)
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
	ret = mappa_bind_ctlra_to_target(m, dev, layer, ctype, cid, tid);
	assert(ret == 0);

	/* target 2 */
	t.name = "t_2";
	uint64_t token = 0xcafe;
	ret = mappa_target_add(m, &t, &tid, &token, sizeof(uint64_t));
	assert(ret == 0);
	cid = 13;
	ret = mappa_bind_ctlra_to_target(m, dev, layer, ctype, cid, tid);
	if(ret != 0)
		printf("MAP failed: cid %d\n", cid);

	t.name = "t_3";
	ret = mappa_target_add(m, &t, &tid, &token_test, sizeof(token_test));
	assert(ret == 0);
	cid = 12;
	ret = mappa_bind_ctlra_to_target(m, dev, layer, ctype, cid, tid);
	if(ret != 0)
		printf("MAP failed: cid %d\n", cid);


	/****** Feedback ******/
	struct mappa_source_t fb = {
		.name = "test_fb_1",
		.func = sw_source_float_func_1,
		.userdata = 0,
	};
	uint32_t source_id;
	ret = mappa_source_add(m, &fb, &source_id, 0, 0);
	assert(ret == 0);

	/**** map feedback *****/
	ret = mappa_bind_source_to_ctlra(m, dev, layer, 0, "test_fb_1");

#if 0

	fb.name = "test_fb_2";
	fb.func = sw_source_float_func_2,
	ret = mappa_sw_source_add(m, &fb);
	assert(ret == 0);

	/**** map feedback *****/
	int dev = 0;
	int layer = 0;
	ret = mappa_bind_source_to_ctlra(m, dev, layer, 0, "test_fb_1");
	layer = 1;
	ret = mappa_bind_source_to_ctlra(m, dev, layer, 0, "test_fb_2");

	/* map a few controls */
	int control = 2;
	int group = 0;
	int item = 7;
	layer = 0;
	ret = mappa_bind_ctlra_to_target(m, dev, CTLRA_EVENT_SLIDER, control,
					 group, item, layer);

	control = 13;
	group = 1;
	item = 1;
	ret = mappa_bind_ctlra_to_target(m, dev, CTLRA_EVENT_SLIDER, control,
					 group, item, layer);

	control = 2;
	group = 1;
	item = 2;
	ret = mappa_bind_ctlra_to_target(m, dev, CTLRA_EVENT_BUTTON, control,
					 group, item, layer);

	control = 0;
	group = 0;
	item = 0;
	ret = mappa_bind_ctlra_to_target(m, dev, CTLRA_EVENT_BUTTON, control,
					 group, item, layer);

	/* layer 1 bindings */
	control = 1;
	group = 0;
	item = 0;
	layer = 1;
	ret = mappa_bind_ctlra_to_target(m, dev, CTLRA_EVENT_BUTTON, control,
					 group, item, layer);

	control = 13;
	group = 1;
	item = 3;
	ret = mappa_bind_ctlra_to_target(m, dev, CTLRA_EVENT_SLIDER, control,
					 group, item, layer);

	/**** dev 1 *****/
	dev = 1;
	control = 1;
	group = 1;
	item = 2;
	layer = 0;
	ret = mappa_bind_ctlra_to_target(m, dev, CTLRA_EVENT_SLIDER, control,
					 group, item, layer);
#endif

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

	bind_callback(m, 0x0);

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
