#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include <iostream>
#include <cstdlib>

#include "RtMidi.h"

#include "ctlra.h"
#include "devices/ni_maschine_mikro_mk2.h"
#include "sequencer.h"

static volatile uint32_t done;
static struct ctlra_dev_t* dev;

static Sequencer *sequencer;

#define MODE_GROUP   0
#define MODE_PADS    1
#define MODE_PATTERN 2

struct col_t
{
	uint8_t green;
	uint8_t blue;
	uint8_t red;
};
static const struct col_t grp_col[] = {
	{0x00, 0x92,  254},
	{0xf9, 0x21, 0x00},
	{0x00, 0xf0, 0x00},
	{0x00, 0x00, 0xf0},
	{0xf0, 0xf0, 0xb0},
	{0x3f, 0x3f, 0xf0},
	{0xf0, 0x00, 0xf0},
	{0x00, 0xef, 0xd5},
	/* dark */
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
};

uint32_t col_dim(const struct col_t *col, float bri)
{
	struct col_t c;
	c.red   = col->red   * bri;
	c.green = col->green * bri;
	c.blue  = col->blue  * bri;
	uint32_t intCol = *(uint32_t *)&c;
	return intCol;
}


static struct mm_t
{
	/* 0 Group   : Selects active group
	 * 1 Pads    : Plays different midi notes
	 * 2 Pattern : Sequence a single note
	 */
	uint8_t mode;
	uint8_t mode_prev;
	uint8_t shift_pressed;

	/* GROUP */
	uint8_t grp_id; /* 0 - 7, selected group */

	/* Pads */
	uint8_t pads_pressed[16];

	/* Pattern */
	uint8_t pattern_pad_id; /* the pad that this pattern is playing */
} mm_static;

void demo_feedback_func(struct ctlra_dev_t *dev, void *d)
{
	/* group, pads, sequencer modes */
	struct mm_t *mm = &mm_static;
	const struct col_t *col = &grp_col[mm->grp_id];
	switch(mm->mode) {
	case MODE_PATTERN: {
		for(int i = 0; i < 16; i++) {
			int on = sequencer_get_step(sequencer, i);
			ctlra_dev_light_set(dev, NI_MASCHINE_MIKRO_MK2_LED_PAD_1 + i,
					    col_dim(col, on ? 1.0 : 0.00));
		}
		int step = sequencer_get_current_step( sequencer );
		int on = sequencer_get_step(sequencer, step % 16);
		ctlra_dev_light_set(dev, NI_MASCHINE_MIKRO_MK2_LED_PAD_1 + step,
				    on ?  0x007f7f7f : 0x000f0f0f );
		break;
		}
	case MODE_GROUP:
		for(int i = 0; i < 16; i++) {
			ctlra_dev_light_set(dev, NI_MASCHINE_MIKRO_MK2_LED_PAD_1 + i,
					    col_dim(&grp_col[i],
						    i == mm->grp_id ? 1.0 : 0.05));
		}
		break;
	case MODE_PADS:
		for(int i = 0; i < 16; i++) {
			ctlra_dev_light_set(dev, NI_MASCHINE_MIKRO_MK2_LED_PAD_1 + i,
					    col_dim(&grp_col[mm->grp_id],
						    mm->pads_pressed[i] ? 1.0 : 0.02));
		}
		break;
	}

	ctlra_dev_light_set(dev, NI_MASCHINE_MIKRO_MK2_LED_PAD_MODE,
			    mm->mode == MODE_PADS ? 0xffffffff : 0x3);
	ctlra_dev_light_set(dev, NI_MASCHINE_MIKRO_MK2_LED_PATTERN,
			    mm->mode == MODE_PATTERN ? 0xffffffff : 0x3);



	ctlra_dev_light_set(dev, NI_MASCHINE_MIKRO_MK2_LED_GROUP,
			    col_dim(&grp_col[mm->grp_id], 1.0));


	ctlra_dev_light_flush(dev, 1);
}

void demo_event_func(struct ctlra_dev_t* dev,
                     uint32_t num_events,
                     struct ctlra_event_t** events,
                     void *userdata)
{
	struct mm_t *mm = &mm_static;
	/* See below, a RtMidiOut instance is passed as the device's
	 * userdata pointer when registering the event_func */
	RtMidiOut *midiout = (RtMidiOut *)userdata;

	std::vector<unsigned char> message(3);

	for(uint32_t i = 0; i < num_events; i++) {
		const char *pressed = 0;
		struct ctlra_event_t *e = events[i];
		int pr = 0;
		switch(e->type) {
		case CTLRA_EVENT_BUTTON:
			message[0] = e->button.pressed ? 0x90 : 0x80;
			message[1] = 60 + e->button.id;
			message[2] = e->button.pressed ? 0x70 : 0;
			midiout->sendMessage( &message );
			printf("button %d\n", e->button.id);
			pr = e->button.pressed;
			switch(e->button.id) {
			case  7: mm->shift_pressed = pr; break;
			case 22: mm->mode = MODE_PATTERN; break;
			case 23: mm->mode = MODE_PADS; break;
			case 8: /* Group */
				if(pr) {
					mm->mode_prev = mm->mode;
					mm->mode = 0;
				} else {
					mm->mode = mm->mode_prev;
				}
				break;
			}
			break;

		case CTLRA_EVENT_ENCODER:
			break;

		case CTLRA_EVENT_SLIDER:
			message[0] = 0xb0;
			message[1] = e->slider.id;
			message[2] = int(e->slider.value * 127.f);
			midiout->sendMessage( &message );
			break;

		case CTLRA_EVENT_GRID:
			pr = e->grid.pressed;
			if(mm->mode == MODE_GROUP) {
				/* select new group here */
				if(e->grid.pressed && e->grid.pos < 8) {
					mm->grp_id = e->grid.pos;
				}
			}
			if(mm->mode == MODE_PATTERN && pr) {
				if(mm->shift_pressed)
					mm->pattern_pad_id = e->grid.pos;
				else
					sequencer_toggle_step(sequencer, e->grid.pos);
			}
			mm->pads_pressed[e->grid.pos] = e->grid.pressed;
			break;
		default:
			break;
		};
	}

	ctlra_dev_light_flush(dev, 0);
}

void sighndlr(int signal)
{
	done = 1;
	printf("\n");
}

void remove_dev_func(struct ctlra_dev_t *dev, int unexpected_removal,
		     void *userdata)
{
	RtMidiOut *midiout = (RtMidiOut *)userdata;
	delete midiout;
}

int accept_dev_func(const struct ctlra_dev_info_t *info,
                    ctlra_event_func *event_func,
                    ctlra_feedback_func *feedback_func,
                    ctlra_remove_dev_func *remove_func,
                    void **userdata_for_event_func,
                    void *userdata)
{
	printf("daemon: accepting %s %s\n", info->vendor, info->device);

	*event_func = demo_event_func;
	*feedback_func = demo_feedback_func;
	*remove_func = remove_dev_func;

	/* MIDI output */
	RtMidiOut *midiout = new RtMidiOut(RtMidi::UNSPECIFIED, "Ctlra");
	unsigned int nPorts = midiout->getPortCount();
	if ( nPorts == 0 ) {
		std::cout << "No ports available!\n";
		return 0;
	}

	try {
		midiout->openVirtualPort(info->device);
	} catch (...) {
		printf("CtlrError: failed to open virtual midi port\n");
		return -1;
	}
	*userdata_for_event_func = midiout;

	return 1;
}

void seqEventCb(int frame, int note, int velocity, void* user_data )
{
	printf("%s: %d, %d : %d\n", __func__, frame, note, velocity);
}

#define SR 48000

int main()
{
	signal(SIGINT, sighndlr);

	sequencer = sequencer_new(SR);
	sequencer_set_callback(sequencer, seqEventCb, 0x0);
	sequencer_set_length(sequencer, SR * 1.2);
	sequencer_set_num_steps(sequencer, 16);
	sequencer_toggle_step(sequencer, 0);
	sequencer_toggle_step(sequencer, 4);
	sequencer_toggle_step(sequencer, 10);
	sequencer_toggle_step(sequencer, 12);

	mm_static.mode = MODE_PATTERN;

	struct ctlra_t *ctlra = ctlra_create(NULL);
	int num_devs = ctlra_probe(ctlra, accept_dev_func, 0x0);

	uint32_t sleep = SR / 128;

	while(!done) {
		ctlra_idle_iter(ctlra);
		sequencer_process( sequencer, 128 );
		usleep(1000 * 1000 / sleep);
	}

	ctlra_exit(ctlra);

	return 0;
}
