#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include <iostream>
#include <cstdlib>
#include "rtmidi/RtMidi.h"

#include "ctlr/ctlr.h"

static volatile uint32_t done;
static struct ctlr_dev_t* dev;
static RtMidiOut *midiout;

void demo_event_func(struct ctlr_dev_t* dev,
                     uint32_t num_events,
                     struct ctlr_event_t** events,
                     void *userdata)
{
	(void)dev;
	(void)userdata;

	/* reserve 3 bytes */
	std::vector<unsigned char> message(3);

	for(uint32_t i = 0; i < num_events; i++) {
		const char *pressed = 0;
		struct ctlr_event_t *e = events[i];
		const char *name = 0;
		switch(e->id) {
		case CTLR_EVENT_BUTTON:
			name = ctlr_dev_control_get_name(dev, e->button.id);
			printf("[%s] button %s (%d)\n",
			       e->button.pressed ? " X " : "   ",
			       name, e->button.id);
			ctlr_dev_light_set(dev, e->button.id, UINT32_MAX);
			message[0] = 0xb0;
			message[1] = e->button.id;
			message[2] = int(e->button.pressed * 127.f);
			midiout->sendMessage( &message );
			break;
		case CTLR_EVENT_ENCODER:
			name = ctlr_dev_control_get_name(dev, e->button.id);
			printf("[%s] encoder %s (%d)\n",
			       e->encoder.delta > 0 ? " ->" : "<- ",
			       name, e->button.id);
			break;
		case CTLR_EVENT_SLIDER:
			name = ctlr_dev_control_get_name(dev, e->button.id);
			printf("[%03d] slider %s (%d)\n",
			       (int)(e->slider.value * 100.f),
			       name, e->slider.id);
			if(e->slider.id == 11) {
				uint32_t iter = (int)((e->slider.value+0.05) * 7.f);
				for(i = 0; i < iter; i++) {
					ctlr_dev_light_set(dev, 1 + i, UINT32_MAX);
					ctlr_dev_light_set(dev, 8 + i, UINT32_MAX);
				}
				for(; i < 7.0; i++) {
					ctlr_dev_light_set(dev, 1 + i, 0);
					ctlr_dev_light_set(dev, 8 + i, 0);
				}
			}
			message[0] = 0xb0;
			message[1] = e->slider.id;
			message[2] = int(e->slider.value * 127.f);
			midiout->sendMessage( &message );
			break;

		case CTLR_EVENT_GRID:
			static const char* grid_pressed[] = { " X ", "   " };
			name = ctlr_dev_control_get_name(dev, e->button.id);
			if(e->grid.flags & CTLR_EVENT_GRID_BUTTON) {
				pressed = grid_pressed[e->grid.pressed];
			} else {
				pressed = "---";
			}
			printf("[%s] grid %d", pressed, e->grid.pos);
			if(e->grid.flags & CTLR_EVENT_GRID_PRESSURE)
				printf(", pressure %1.3f", e->grid.pressure);
			printf("\n");
			break;
		default:
			break;
		};
	}

	ctlr_dev_light_flush(dev, 0);
}

void sighndlr(int signal)
{
	ctlr_dev_grid_light_set(dev, 0, 0, 0xFFFF0000);
	ctlr_dev_disconnect(dev);
	exit(0);
}

int main()
{
	signal(SIGINT, sighndlr);

	midiout = new RtMidiOut();
	unsigned int nPorts = midiout->getPortCount();
	if ( nPorts == 0 ) {
		std::cout << "No ports available!\n";
		return 0;
	}
	midiout->openPort( 1 );

	//int dev_id = CTLR_DEV_SIMPLE;
	enum ctlr_dev_id_t dev_id = CTLR_DEV_NI_KONTROL_Z1;
	//int dev_id = CTLR_DEV_NI_MASCHINE_MIKRO_MK2;
	void *userdata = 0x0;
	void *future = 0x0;
	dev = ctlr_dev_connect(dev_id, demo_event_func, userdata, future);
	if(!dev)
		return -1;

	uint32_t i = 8;
	while(i > 0) {
		ctlr_dev_poll(dev);
		i--;
	}

	uint32_t light_id = 30;
	const uint32_t BLINK  = (1 << 31);
	const uint32_t BRIGHT = (0x7F << 24);
	uint32_t light_status_r = BLINK | BRIGHT | (0xFF << 16) | (0x0 << 8) | (0x0);

	printf("polling loop now..\n");
	while(!done) {
		ctlr_dev_poll(dev);
		usleep(100);
	}

	ctlr_dev_disconnect(dev);
	delete midiout;
	return 0;
}
