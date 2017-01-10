//#include <stdint.h>

#include "event.h"

typedef unsigned int uint32_t;

/* Tell the host application what USB device this script is for */
void script_get_vid_pid(int *out_vid, int *out_pid)
{
	*out_vid = 0x17cc;
	*out_pid = 0x1400;
}

void script_event_func(struct ctlra_dev_t* dev,
                       unsigned int num_events,
                       struct ctlra_event_t** events,
                       void *userdata)
{
	printf("script func, num events %d\n", num_events);
	for(uint32_t i = 0; i < num_events; i++) {
		printf("i = %d\n", i);
		struct ctlra_event_t *e = events[i];
		printf("event %p\n", e);
		uint32_t t = *(uint32_t*)e;
		printf("event as uint32 %d\n", t);
		printf("event type deref %d\n", e->type);
		if(t == 0) { // button 
			uint32_t b = *(((uint32_t*)e)+1);
			printf("script button  %d\n", b);
			printf("scrpt: button id %d\n", e->button.id);
		}
	}
#if 0
		switch(e->type) {
		case CTLRA_EVENT_BUTTON:
			break;
		}
	}
#endif
}
