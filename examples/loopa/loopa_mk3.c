/* hack for TCC to not need stdint.h, which causes a segfault on compile
 * the second time the script is compiled */
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef unsigned int uint16_t;
typedef int int32_t;

#include "loopa.h"
#include "event.h"

struct mk3_loopa_t {
	int loop_id;
};

static struct mk3_loopa_t self;

void script_feedback_func(struct ctlra_dev_t *dev, void *userdata)
{
	uint32_t l = loopa_rec_get(0) ? -1 : 0;
	ctlra_dev_light_set(dev, 42, l);
	l = loopa_play_get(0) ? -1 : 0;
	ctlra_dev_light_set(dev, 41, l);
	ctlra_dev_light_flush(dev, 0);

	float v = loopa_vol_get(0);
	//printf("v = %f\n", v);
}

/* Tell the host application what USB device this script is for */
void script_get_vid_pid(int *out_vid, int *out_pid)
{
	*out_vid = 0x17cc;
	*out_pid = 0x1600;
}

void script_event_func(struct ctlra_dev_t* dev,
                       unsigned int num_events,
                       struct ctlra_event_t** events,
                       void *userdata)
{

	//printf("script func, num events %d\n", num_events);
	for(uint32_t i = 0; i < num_events; i++) {
		struct ctlra_event_t *e = events[i];
		int pr = e->button.pressed;
		switch(e->type) {
		case CTLRA_EVENT_BUTTON:
			switch(e->button.id) {
			case 7: case 8: case 9: case 10:
			case 11: case 12: case 13: case 14:
				/* a b c d .. buttons */
				if(pr) {
					self.loop_id = e->button.id - 7;
					printf("loop id = %d\n", self.loop_id);
				} break;

			case 38: if(pr) loopa_reset(); break;
			case 41: if(pr) loopa_playing_toggle(); break;
			case 42: if(pr) loopa_record_toggle(); break;
			case 52: printf("Flux\n"); break;
			case 53: printf("Deck\n"); break;
			case 54: printf("Shift\n"); break;
			default: printf("button %d\n", e->button.id); break;
			}
			break;
		case CTLRA_EVENT_ENCODER:
			if(e->encoder.id != 0)
				loopa_vol_set(0, 1.0f);
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
