#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include <iostream>
#include <cstdlib>

#include "RtMidi.h"

#include "ctlra.h"

static volatile uint32_t done;
static struct ctlra_dev_t* dev;

void demo_feedback_func(struct ctlra_dev_t *dev, void *d)
{
	/* feedback like LEDs and Screen drawing based on application
	 * state goes here. No events should be sent to the application
	 * from this function - one-way App->Ctlra updates only */
}

void demo_event_func(struct ctlra_dev_t* dev,
                     uint32_t num_events,
                     struct ctlra_event_t** events,
                     void *userdata)
{
	/* Events from the Ctlra device are handled here. They should be
	 * decoded, and events sent to the application. Note that this
	 * function must *NOT* send feedback to the device. Instead, the
	 * feedback_func() above should be used to send feedback */

	/* See below, a RtMidiOut instance is passed as the device's
	 * userdata pointer when registering the event_func */
	RtMidiOut *midiout = (RtMidiOut *)userdata;

	std::vector<unsigned char> message(3);

	for(uint32_t i = 0; i < num_events; i++) {
		const char *pressed = 0;
		struct ctlra_event_t *e = events[i];
		const char *name = 0;
		switch(e->type) {
		case CTLRA_EVENT_BUTTON:
			name = ctlra_dev_control_get_name(dev, CTLRA_EVENT_BUTTON, e->button.id);
			printf("[%s] button %s (%d)\n",
			       e->button.pressed ? " X " : "   ",
			       name, e->button.id);
			message[0] = e->button.pressed ? 0x90 : 0x80;
			message[1] = 60 + e->button.id;
			message[2] = e->button.pressed ? 0x70 : 0;
			midiout->sendMessage( &message );
			break;

		case CTLRA_EVENT_ENCODER:
			name = ctlra_dev_control_get_name(dev, CTLRA_EVENT_ENCODER,  e->button.id);
			printf("[%s] encoder %s (%d)\n",
			       e->encoder.delta > 0 ? " ->" : "<- ",
			       name, e->button.id);
			break;

		case CTLRA_EVENT_SLIDER:
			name = ctlra_dev_control_get_name(dev, CTLRA_EVENT_SLIDER, e->button.id);
			printf("[%03d] slider %s (%d)\n",
			       (int)(e->slider.value * 100.f),
			       name, e->slider.id);
			message[0] = 0xb0;
			message[1] = e->slider.id;
			message[2] = int(e->slider.value * 127.f);
			midiout->sendMessage( &message );
			break;

		case CTLRA_EVENT_GRID:
			static const char* grid_pressed[] = { " X ", "   " };
			name = ctlra_dev_control_get_name(dev, CTLRA_EVENT_GRID,
			                                  e->button.id);
			if(e->grid.flags & CTLRA_EVENT_GRID_FLAG_BUTTON) {
				pressed = grid_pressed[e->grid.pressed];
			} else {
				pressed = "---";
			}
			printf("[%s] grid %d", pressed, e->grid.pos);
			if(e->grid.flags & CTLRA_EVENT_GRID_FLAG_PRESSURE)
				printf(", pressure %1.3f", e->grid.pressure);
			printf("\n");
			message[0] = e->grid.pressed ? 0x90 : 0x80;
			message[1] = e->grid.pos + 36; /* default GM drum
							  mapping */
			message[2] = e->grid.pressed ? 0x70 : 0;
			midiout->sendMessage( &message );
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

int main()
{
	signal(SIGINT, sighndlr);

	struct ctlra_t *ctlra = ctlra_create(NULL);
	int num_devs = ctlra_probe(ctlra, accept_dev_func, 0x0);

	while(!done) {
		ctlra_idle_iter(ctlra);
		usleep(10 * 1000);
	}

	ctlra_exit(ctlra);

	return 0;
}
