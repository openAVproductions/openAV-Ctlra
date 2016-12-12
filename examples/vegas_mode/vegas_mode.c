#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "ctlr.h"
#include "global.h"

static volatile uint32_t done;

void sighndlr(int signal)
{
	done = 1;
}

int main()
{
	signal(SIGINT, sighndlr);

	struct dummy_data dummy;

	struct ctlr_dev_t* dev_x1;
	struct ctlr_dev_t* dev_z1;
	//int dev_id = CTLR_DEV_SIMPLE;
	//int dev_id = CTLR_DEV_NI_KONTROL_Z1;
	//int dev_id = CTLR_DEV_NI_KONTROL_X1_MK2;
	//int dev_id = CTLR_DEV_NI_MASCHINE_MIKRO_MK2;
	void *future = 0x0;
	dev_x1 = ctlr_dev_connect(CTLR_DEV_NI_KONTROL_X1_MK2,
				  kontrol_x1_func,
				  &dummy, future);
	if(!dev_x1)
		return -1;
	dev_z1 = ctlr_dev_connect(CTLR_DEV_NI_KONTROL_Z1,
				  kontrol_z1_func,
				  &dummy, future);
	if(!dev_z1)
		return -1;

	printf("polling loop now..\n");
	uint64_t controller_revision = 0;
	while(!done) {
		ctlr_dev_poll(dev_x1);
		ctlr_dev_poll(dev_z1);
		if(dummy.revision != controller_revision) {
			kontrol_z1_update_state(dev_z1, &dummy);
			kontrol_x1_update_state(dev_x1, &dummy);
			controller_revision = dummy.revision;
		}
		usleep(100);
	}

	ctlr_dev_disconnect(dev_x1);
	ctlr_dev_disconnect(dev_z1);

	return 0;
}
