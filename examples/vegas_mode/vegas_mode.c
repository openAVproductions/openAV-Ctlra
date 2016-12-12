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

int main()
{
	signal(SIGINT, sighndlr);

	struct dummy_data dummy;

	struct ctlr_list_t ctlr_list;
	TAILQ_INIT(&ctlr_list);
	//int dev_id = CTLR_DEV_SIMPLE;
	//int dev_id = CTLR_DEV_NI_KONTROL_Z1;
	//int dev_id = CTLR_DEV_NI_KONTROL_X1_MK2;
	//int dev_id = CTLR_DEV_NI_MASCHINE_MIKRO_MK2;
	void *future = 0x0;
	struct ctlr_dev_t* ctlr;
	ctlr = ctlr_dev_connect(CTLR_DEV_NI_KONTROL_X1_MK2,
				kontrol_x1_func, &dummy, future);
	if(ctlr) {
		struct ctlr_t *dev = calloc(1, sizeof(struct ctlr_t));
		if(!dev) return -1;
		dev->ctlr = ctlr;
		dev->update_state = kontrol_x1_update_state;
		TAILQ_INSERT_TAIL(&ctlr_list, dev, list);
	}
	ctlr = ctlr_dev_connect(CTLR_DEV_NI_KONTROL_Z1, kontrol_z1_func,
				&dummy, future);
	if(ctlr) {
		struct ctlr_t *dev = calloc(1, sizeof(struct ctlr_t));
		if(!dev) return -1;
		dev->ctlr = ctlr;
		dev->update_state = kontrol_z1_update_state;
		TAILQ_INSERT_TAIL(&ctlr_list, dev, list);
	}

	int i = 0;
	struct ctlr_t* dev;
	TAILQ_FOREACH(dev, &ctlr_list, list) {
		printf("%d, %p, %p\n", i, dev->ctlr, dev->update_state);
		i++;
	}

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
