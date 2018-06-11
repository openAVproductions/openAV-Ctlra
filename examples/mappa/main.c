#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>

#include "mappa.h"

static volatile uint32_t done;

void sighndlr(int signal)
{
	done = 1;
	printf("\n");
}

void sw_target_float_func(uint32_t group_id, uint32_t target_id,
			  float value, void *userdata)
{
	printf("%s: (%d : %d) %f\n", __func__, group_id, target_id, value);
}

int main(int argc, char **argv)
{
	signal(SIGINT, sighndlr);

	/* create mappa context */
	struct mappa_t *m = mappa_create(NULL);
	if(!m)
		return -1;

	char *group = "group";
	char *item = "item";


	struct mappa_sw_target_t tar = {
		.group_name = group,
		.item_name = item,
		.group_id = 0,
		.item_id = 0,
		.func = sw_target_float_func,
		.userdata = 0x0,
	};

	/* add target */
	int32_t ret = mappa_sw_target_add(m, &tar);
	assert(ret == 0);

	int i = 0;
	while(!done && i++ < 10) {
		//mappa_run(m);
		usleep(10 * 1000);
	}

	mappa_destroy(m);

	return 0;
}
