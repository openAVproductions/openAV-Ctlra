// usleep() hack
#define _BSD_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "ctlra.h"
#include "global.h"

static volatile uint32_t done;

void sighndlr(int signal)
{
	done = 1;
}

struct ctlra_supported_t {
	uint32_t vendor_id;
	uint32_t device_id;
	ctlra_event_func ctlra_poll;
	ctlra_feedback_func update_state;
};

static const struct ctlra_supported_t ctlra_supported[] = {
	{0x17cc, 0x1400, kontrol_d2_func, kontrol_d2_update_state},
	{0x17cc, 0x1210, kontrol_z1_func, kontrol_z1_update_state},
	//{0x17cc, 0x12 TODO,kontrol_f1_func, kontrol_f1_update_state},
	{0x17cc, 0x1220, kontrol_x1_func, kontrol_x1_update_state},
	//{0x17cc, 0x12, mm_func, mm_update_state},
};
#define CTLRA_SUPPORTED_SIZE (sizeof(ctlra_supported) /\
			     sizeof(ctlra_supported[0]))

int accept_dev_func(const struct ctlra_dev_info_t *info,
		    ctlra_event_func *event_func,
		    ctlra_feedback_func *feedback_func,
		    void **userdata_for_event_func,
		    void *userdata)
{
	/* Application *MUST* handle each device that it knows - or provide
	 * a generic function for handling events and writing feedback. */
	for(unsigned i = 0; i < CTLRA_SUPPORTED_SIZE; i++) {
		if(info->vendor_id == ctlra_supported[i].vendor_id &&
		   info->device_id == ctlra_supported[i].device_id) {
			*event_func = ctlra_supported[i].ctlra_poll;
			*feedback_func = ctlra_supported[i].update_state;
			*userdata_for_event_func = userdata;
			printf("App: accepting %s %s (%x:%x)\n",
			       info->vendor, info->device,
			       info->vendor_id, info->device_id);
			/* Accept */
			return 1;
		}
	}
	printf("App: Rejecting controller %s %s (%x:%x), no App for this device\n",
	       info->vendor, info->device,
	       info->vendor_id, info->device_id);
	/* Deny */
	return 0;
}

int main(int argc, char** argv)
{
	signal(SIGINT, sighndlr);

	struct dummy_data dummy;
	memset(&dummy, 0, sizeof(struct dummy_data));
	void *future = 0x0;

	if(argc > 1) {
		dummy.print_events = 1;
	}

	struct ctlra_t *ctlra = ctlra_create(NULL);

	int num_devs = ctlra_probe(ctlra, accept_dev_func, &dummy);
	printf("probe num_devs %d\n", num_devs);

	audio_init(&dummy);

	while(!done) {
		ctlra_idle_iter(ctlra);
		usleep(100);
	}

	audio_exit();

	ctlra_exit(ctlra);

	return 0;
}
