/* hack for TCC to not need stdint.h, which causes a segfault on compile
 * the second time the script is compiled */
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef unsigned int uint16_t;
typedef int int32_t;

#include "loopa.h"
#include "event.h"

#define HIGH ((uint32_t)-1)
#define LOW  ((uint32_t)0xef000000)

enum MODES {
	MODE_INPUT,
	MODE_ARRANGE,
	MODE_SAMPLING,
	MODE_PLUGIN,
	MODE_CHANNEL,
	MODE_BROWSER,
};

struct mk3_loopa_t {
	int loop_id;
	float v;

	uint32_t mode;

	uint32_t col;

	/* MODE_INPUT variables */
	int selected_input_channel;
	float selected_input_last_max;
};

static struct mk3_loopa_t self;

static const uint32_t col_lut[] = {
	0xFF0000,
	0x00A2FF,
	0xFF5f00,
	0x0040FF,
	0x80FF00,
	0x8000FF,
	0x00FF4f,
	0xFF00BF,
};


static const uint32_t col_lut_16[] = {
	0xFF0000,
	0xFF5f00,
	0xFFBF00,
	0xDFFF00,
	0x80FF00,
	0x22FF00,
	0x00FF4f,
	0x00FFa2,

	0x00FFFF,
	0x00A2FF,
	0x0040FF,
	0x2200FF,
	0x8000FF,
	0xE100FF,
	0xFF00BF,
	0xFF005D,
};

void script_feedback_func(struct ctlra_dev_t *dev, void *userdata)
{
	uint32_t l = loopa_rec_get(0) ? -1 : 0;
	ctlra_dev_light_set(dev, 42, l);
	l = loopa_play_get(0) ? -1 : 0;
	ctlra_dev_light_set(dev, 41, l);

	float m = loopa_input_max(self.selected_input_channel);
	float last_max = self.selected_input_last_max *= 0.97;
	if(m > last_max)
		self.selected_input_last_max = m;

	/* up top left mode button LEDs */
	ctlra_dev_light_set(dev, 0, (self.mode == MODE_CHANNEL) ? HIGH : LOW);
	ctlra_dev_light_set(dev, 1, (self.mode == MODE_PLUGIN) ? HIGH : LOW);
	ctlra_dev_light_set(dev, 2, (self.mode == MODE_ARRANGE) ? HIGH : LOW);
	ctlra_dev_light_set(dev, 3, (self.mode == MODE_INPUT) ? HIGH : LOW);
	ctlra_dev_light_set(dev, 4, (self.mode == MODE_BROWSER) ? HIGH : LOW);
	//ctlra_dev_light_set(dev, 5, (self.mode == MODE_SAMPLING) ? self.col : self.col );

	/* A B C D E F G H  buttons */
	for(int i = 0; i < 8; i++)
		ctlra_dev_light_set(dev, 29 + i, col_lut[i]);

	for(int i = 0; i < 4; i++)
		ctlra_dev_light_set(dev, 58 + i, col_lut[i]);

	if(self.mode == MODE_INPUT) {
		for(int i = 0; i < 25; i++) {
			uint32_t col = 0xff00; /* green */
			if(i > 13) col = 0xff7f00; /* orange */
			if(i > 20) col = 0xff0000; /* red */
			uint32_t c = i / 25.f > last_max ? 0x00 : col;
			ctlra_dev_light_set(dev, 62 + i, c);
		}
		for(int i = 0; i < 16; i++)
			ctlra_dev_light_set(dev, 62 + 25 + i, self.col);

		ctlra_dev_light_set(dev, 12, self.selected_input_channel == 0 ?
					     0xff000000 : 0x03000000);
		ctlra_dev_light_set(dev, 13, self.selected_input_channel == 1 ?
					     0xff000000 : 0x03000000);
	}

	if(self.mode == MODE_ARRANGE) {
		for(int i = 0; i < 25; i++)
			ctlra_dev_light_set(dev, 62 + i, self.col);
		for(int i = 0; i < 16; i++)
			ctlra_dev_light_set(dev, 62 + 25 + i, self.col);
	}

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

int script_screen_redraw_func(struct ctlra_dev_t *dev, uint32_t screen_idx,
			      uint8_t *pixel_data, uint32_t bytes,
			      void *userdata)
{
	int flush = 1;

	memset(pixel_data, 0, bytes);

	if(self.mode == MODE_INPUT) {
		/* draw input selector / levels on screen */
		if(screen_idx == 1) {
			for(int i = 0; i < 240; i++) {
				int off = (self.selected_input_channel == 1) * 240;
				pixel_data[480*0  + i + off] = 0b111000;
				pixel_data[480*22 + i + off] = 0b111000;
			}
		}
		return 1;
	}

	if(screen_idx == 1)
		return 0;

	for(int i = 0; i < bytes; i++) {
		pixel_data[i] = 0;
	}

	float v = 1 - self.v;
	for(int i = 0; i < 272; i++) {
		pixel_data[(480*2) * i+110] = (i / 270.f) < v ? 0 : 0xff;
		pixel_data[(480*2) * i+111] = (i / 270.f) < v ? 0 : 0xff;
		pixel_data[(480*2) * i+112] = (i / 270.f) < v ? 0 : 0xff;
	}

	//printf("self.v = %f\n", self.v);

	return flush;
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
					self.col = col_lut[self.loop_id];
				} break;

			case 38: if(pr) loopa_reset(); break;
			case 41: if(pr) loopa_playing_toggle(); break;
			case 42: if(pr) loopa_record_toggle(); break;

			case 47: self.mode = MODE_SAMPLING;
				 self.col = 0x20407fff;
				 break;
			case 48: self.mode = MODE_INPUT; break;
			case 49: self.mode = MODE_PLUGIN; break;
			case 50: self.mode = MODE_CHANNEL; break;
			case 51: self.mode = MODE_ARRANGE; break;
			case 52: self.mode = MODE_BROWSER; break;

			/* select input mode */
			case 56: self.selected_input_channel = 0;
				break;
			case 57: self.selected_input_channel = 1;
				break;

			case 73: printf("boo\n");
				if(!pr) {
					if(loopa_rec_get(0) && !loopa_play_get(0)) {
						loopa_playing_toggle();
					}
					loopa_record_toggle();
				} break;
			default: printf("button %d\n", e->button.id); break;
			}
			break;
		case CTLRA_EVENT_ENCODER:
			if(e->encoder.id == 0)
				loopa_vol_set(0, 1.0f);
			else {
				self.v += e->encoder.delta_float;
				if(self.v > 1.f) self.v = 1.0f;
				if(self.v < 0.f) self.v = 0.0f;
				printf("%f\n", self.v);
				loopa_reverb(self.v);
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
