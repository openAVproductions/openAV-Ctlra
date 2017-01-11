/* hack for TCC to not need stdint.h, which causes a segfault on compile
 * the second time the script is compiled */
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef unsigned int uint16_t;
typedef int int32_t;

#include "event.h"

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
	//printf("script func, num events %d\n", num_events);
	for(uint32_t i = 0; i < num_events; i++) {
		struct ctlra_event_t *e = events[i];
		switch(e->type) {
		case CTLRA_EVENT_BUTTON:
			switch(e->button.id) {
			case 52: printf("Flux\n"); break;
			case 53: printf("Deck\n"); break;
			case 54: printf("Shift\n"); break;
			default: printf("button %d\n", e->button.id); break;
			}
			break;
		case CTLRA_EVENT_SLIDER:
			switch(e->slider.id) {
			case 62: printf("slider 1 event %f\n", e->slider.value); break;
			default: printf("slider %d\n", e->slider.id); break;
			}
			break;
		default: break;
		}
	}
}
