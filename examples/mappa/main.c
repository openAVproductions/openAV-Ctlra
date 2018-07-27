#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <ctlra/mappa.h>

static volatile uint32_t done;

void sighndlr(int signal)
{
	done = 1;
	printf("\n");
}

static float the_value;

void sw_source_float_func_1(float *value, void *token, uint32_t token_size,
			    void *userdata)
{
	*value = the_value;
}

void sw_source_float_func_2(float *value, void *token, uint32_t token_size,
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
	printf("%s: target id %u, value %f, token size %d\n",
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

	/* target 2 */
	t.name = "t_2";
	uint64_t token = 0xcafe;
	ret = mappa_target_add(m, &t, &tid, &token, sizeof(uint64_t));
	assert(ret == 0);

	t.name = "t_3";
	ret = mappa_target_add(m, &t, &tid, &token_test, sizeof(token_test));
	assert(ret == 0);

	return 0;
}

int32_t
register_feedback(struct mappa_t *m, void *userdata)
{
	/****** Feedback ******/
	struct mappa_source_t fb = {
		.name = "test fb 1",
		.func = sw_source_float_func_1,
		.userdata = 0,
	};
	uint32_t source_id;
	int ret = mappa_source_add(m, &fb, &source_id, 0, 0);
	if(ret != 0)
		printf("MAP %d failed: source name %s\n", __LINE__, fb.name);

	/**** FB item 2 */
	fb.name = "test fb 2";
	fb.func = sw_source_float_func_2;
	ret = mappa_source_add(m, &fb, &source_id, 0, 0);
	if(ret != 0)
		printf("MAP %d failed: sid %u\n", __LINE__, source_id);

	fb.name = "test fb 3";
	fb.func = sw_source_float_func_2;
	ret = mappa_source_add(m, &fb, &source_id, 0, 0);
	if(ret != 0)
		printf("MAP %d failed: sid %d\n", __LINE__, source_id);

	/* verify removal */
	ret = mappa_source_remove(m, source_id);
	assert(ret == 0);
	ret = mappa_source_remove(m, source_id);
	assert(ret == -EINVAL);

	return 0;
}

int main(int argc, char **argv)
{
	signal(SIGINT, sighndlr);

	/* create mappa context */
	struct mappa_t *m = mappa_create(NULL, "ctlra_mappa", "unique_str");
	if(!m)
		return -1;

	register_targets(m, 0x0);
	register_feedback(m, 0x0);

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

	struct mappa_t *m = mappa_create(NULL, "test", "no_unique");
	assert(m);


	mappa_destroy(m);
}
