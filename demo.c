#include <stdio.h>
#include "ctlr/ctlr.h"

void demo_event_func(struct ctlr_dev_t* dev,
		     const struct ctlr_event_t* event,
		     void *userdata)
{
	(void)dev;
	(void)userdata;
	printf("event %d\n", event->id);
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
