#include <stdio.h>
#include "ctlr/ctlr.h"

void demo_event_func(struct ctlr_dev_t* dev,
		     uint32_t num_events,
		     struct ctlr_event_t** events,
		     void *userdata)
{
	(void)dev;
	(void)userdata;
	for(uint32_t i = 0; i < num_events; i++) {
		switch(events[i]->id) {
		case CTLR_EVENT_BUTTON:
			printf("[%s] button %d\n", events[0]->button.pressed ? "X" : " ", events[0]->id);
			break;
		default:
			break;
		};
	}
}

int main()
{
#if 1
	int dev_id = CTLR_DEV_SIMPLE;
#else
	int dev_id = CTLR_DEV_NI_MASCHINE;
#endif
	void *userdata = 0x0;
	struct ctlr_dev_t *dev = ctlr_dev_connect(dev_id,
						  demo_event_func,
						  userdata, 0x0);
	uint32_t i = 5;

	while(i > 0)
	{
		ctlr_dev_poll(dev);
		i--;
	}

	ctlr_dev_disconnect(dev);
	return 0;
}
