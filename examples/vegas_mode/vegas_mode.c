#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include <sys/queue.h>

#include "ctlr.h"
#include "global.h"

static volatile uint32_t done;

void sighndlr(int signal)
{
	done = 1;
}

struct ctlr_t {
	TAILQ_ENTRY(ctlr_t) list;
	struct ctlr_dev_t* ctlr;
	update_state_cb update_state;
};
TAILQ_HEAD(ctlr_list_t, ctlr_t);

struct ctlr_supported_t {
	enum ctlr_dev_id_t dev_id;
	ctlr_poll_func ctlr_poll;
	update_state_cb update_state;
};

static const struct ctlr_supported_t ctlr_supported[] = {
	{CTLR_DEV_NI_KONTROL_Z1, kontrol_z1_func, kontrol_z1_update_state},
	{CTLR_DEV_NI_KONTROL_X1_MK2, kontrol_x1_func, kontrol_x1_update_state},
	{CTLR_DEV_NI_MASCHINE_MIKRO_MK2, mm_func, mm_update_state},
};
#define CTLR_SUPPORTED_SIZE (sizeof(ctlr_supported) /\
			     sizeof(ctlr_supported[0]))

int main()
{
	signal(SIGINT, sighndlr);

	struct dummy_data dummy;
	void *future = 0x0;

	struct ctlr_list_t ctlr_list;
	TAILQ_INIT(&ctlr_list);

	for(uint32_t i = 0; i < CTLR_SUPPORTED_SIZE; i++) {
		const struct ctlr_supported_t* sup = &ctlr_supported[i];
		printf("sup devid %d\n", sup->dev_id);
		struct ctlr_dev_t* ctlr = ctlr_dev_connect(sup->dev_id,
							   sup->ctlr_poll,
							   &dummy,
							   future);
		if(ctlr) {
			struct ctlr_t *dev = calloc(1, sizeof(struct ctlr_t));
			if(!dev) return -1;
			dev->ctlr = ctlr;
			dev->update_state = sup->update_state;
			TAILQ_INSERT_TAIL(&ctlr_list, dev, list);
		}
	}

	struct ctlr_t* dev;
	uint64_t controller_revision = 0;
	while(!done) {
		/* Poll all controllers */
		TAILQ_FOREACH(dev, &ctlr_list, list) {
			ctlr_dev_poll(dev->ctlr);
		}
		/* If revision of state is new, update state of controllers */
		if (dummy.revision != controller_revision) {
			TAILQ_FOREACH(dev, &ctlr_list, list) {
				dev->update_state(dev->ctlr, &dummy);
			}
			controller_revision = dummy.revision;
		}
		usleep(100);
	}

	/* Disconnect all */
	TAILQ_FOREACH(dev, &ctlr_list, list) {
		ctlr_dev_disconnect(dev->ctlr);
	}

	return 0;
}
