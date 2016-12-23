// usleep() hack
#define _BSD_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>


#include <sys/queue.h>

#include "ctlra.h"
#include "global.h"

static volatile uint32_t done;

void sighndlr(int signal)
{
	done = 1;
}

struct ctlra_supported_t {
	enum ctlra_dev_id_t dev_id;
	ctlra_poll_func ctlra_poll;
	update_state_cb update_state;
};

static const struct ctlra_supported_t ctlra_supported[] = {
	{CTLRA_DEV_NI_KONTROL_D2, kontrol_d2_func, kontrol_d2_update_state},
	{CTLRA_DEV_NI_KONTROL_Z1, kontrol_z1_func, kontrol_z1_update_state},
	{CTLRA_DEV_NI_KONTROL_F1, kontrol_f1_func, kontrol_f1_update_state},
	{CTLRA_DEV_NI_KONTROL_X1_MK2, kontrol_x1_func, kontrol_x1_update_state},
	{CTLRA_DEV_NI_MASCHINE_MIKRO_MK2, mm_func, mm_update_state},
};
#define CTLRA_SUPPORTED_SIZE (sizeof(ctlra_supported) /\
			     sizeof(ctlra_supported[0]))

struct ctlra_t {
	TAILQ_ENTRY(ctlra_t) list;
	struct ctlra_dev_t* ctlra;
	update_state_cb update_state;
};
TAILQ_HEAD(ctlra_list_t, ctlra_t);

int main(int argc, char** argv)
{
	signal(SIGINT, sighndlr);

	struct dummy_data dummy;
	memset(&dummy, 0, sizeof(struct dummy_data));
	void *future = 0x0;

	if(argc > 1) {
		dummy.print_events = 1;
	}

	struct ctlra_list_t ctlra_list;
	TAILQ_INIT(&ctlra_list);

	for(uint32_t i = 0; i < CTLRA_SUPPORTED_SIZE; i++) {
		const struct ctlra_supported_t* sup = &ctlra_supported[i];
		struct ctlra_dev_t* ctlra = ctlra_dev_connect(sup->dev_id,
		                          sup->ctlra_poll,
		                          &dummy,
		                          future);
		if(ctlra) {
			struct ctlra_t *dev = calloc(1, sizeof(struct ctlra_t));
			if(!dev) return -1;
			dev->ctlra = ctlra;
			dev->update_state = sup->update_state;

			struct ctlra_dev_info_t info;
			ctlra_dev_get_info(dev->ctlra, &info);
			printf("Vegas: connected to ctlra %s %s\n",
			       info.vendor, info.device);

			TAILQ_INSERT_TAIL(&ctlra_list, dev, list);
		}
	}

	struct ctlra_t* dev;
	dummy.revision = 0;
	uint64_t controller_revision = 0;
	while(!done) {
		/* Poll all controllers */
		TAILQ_FOREACH(dev, &ctlra_list, list) {
			ctlra_dev_poll(dev->ctlra);
		}
		/* If revision of state is new, update state of controllers */
		if (dummy.revision != controller_revision) {
			TAILQ_FOREACH(dev, &ctlra_list, list) {
				dev->update_state(dev->ctlra, &dummy);
			}
			controller_revision = dummy.revision;
		}
		usleep(100);
	}

	/* Disconnect all */
	while ((dev = TAILQ_FIRST(&ctlra_list))) {
		TAILQ_REMOVE(&ctlra_list, dev, list);
		ctlra_dev_disconnect(dev->ctlra);
		free(dev);
	}

	return 0;
}
