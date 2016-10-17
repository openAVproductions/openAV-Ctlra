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
			printf("[%s] button %d\n",
			       events[i]->button.pressed ? " X " : "   ",
			       events[i]->id);
			break;
		case CTLR_EVENT_ENCODER:
			printf("[%s] encoder %d\n",
			       events[i]->encoder.delta > 0 ? "-=>" : "<=-",
			       events[i]->id);
			break;
		case CTLR_EVENT_GRID:
			printf("[%s] grid %d : pressure %.3f\n",
			       events[i]->grid.pressed ? " X " : "   ",
			       events[i]->id, events[i]->grid.pressure);
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
	uint32_t i = 8;

	while(i > 0)
	{
		ctlr_dev_poll(dev);
		i--;
	}

	ctlr_dev_disconnect(dev);
	return 0;
}
