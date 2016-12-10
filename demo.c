#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "ctlr/ctlr.h"
#include "ctlr/devices/ni_maschine.h"
#include "ctlr/devices/ni_kontrol_z1.h"

static volatile uint32_t done;
static struct ctlr_dev_t* dev;

void demo_event_func(struct ctlr_dev_t* dev,
		     uint32_t num_events,
		     struct ctlr_event_t** events,
		     void *userdata)
{
	(void)dev;
	(void)userdata;
	for(uint32_t i = 0; i < num_events; i++) {
		char *pressed = 0;
		struct ctlr_event_t *e = events[i];
		const char *name = 0;
		switch(e->id) {
		case CTLR_EVENT_BUTTON:
			name = ctlr_dev_control_get_name(dev, e->button.id);
			printf("[%s] button %s (%d)\n",
			       e->button.pressed ? " X " : "   ",
			       name, e->button.id);
			ctlr_dev_light_set(dev, e->button.id, UINT32_MAX);
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
				int iter = (int)((e->slider.value+0.05) * 7.f);
				for(i = 0; i < iter; i++) {
					ctlr_dev_light_set(dev, 1 + i, UINT32_MAX);
					ctlr_dev_light_set(dev, 8 + i, UINT32_MAX);
				}
				for(; i < 7.0; i++) {
					ctlr_dev_light_set(dev, 1 + i, 0);
					ctlr_dev_light_set(dev, 8 + i, 0);
				}
			}

			break;
		case CTLR_EVENT_GRID:
			name = ctlr_dev_control_get_name(dev, e->button.id);
			if(e->grid.flags & CTLR_EVENT_GRID_BUTTON) {
				pressed = e->grid.pressed ? " X " : "   ";
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
	//int dev_id = CTLR_DEV_SIMPLE;
	int dev_id = CTLR_DEV_NI_KONTROL_Z1;
	//int dev_id = CTLR_DEV_NI_MASCHINE_MIKRO_MK2;
	void *userdata = 0x0;
	void *future = 0x0;
	dev = ctlr_dev_connect(dev_id, demo_event_func, userdata, future);
	if(!dev)
		return -1;

	ctlr_dev_light_set(dev, NI_KONTROL_Z1_LED_FX_ON_RIGHT, 0xff);
	ctlr_dev_light_set(dev, NI_KONTROL_Z1_LED_FX_ON_RIGHT, 0xff<< 16);

	uint32_t i = 8;
	while(i > 0)
	{
		ctlr_dev_poll(dev);
		i--;
	}

	uint32_t light_id = 30;
	const uint32_t BLINK  = (1 << 31);
	const uint32_t BRIGHT = (0x7F << 24);
	uint32_t light_status_r = BLINK | BRIGHT | (0xFF << 16) | (0x0 << 8) | (0x0);
#if 0
	uint32_t light_status_g = BLINK |  (0x0 << 16) | (0xFF << 8) | (0x0);
	uint32_t light_status_b = BLINK |  (0x0 << 16) | (0x0 << 8) | (0xFF);
	sleep(1);
	ctlr_dev_light_set(dev, 31, light_status_r);
	ctlr_dev_light_set(dev, 31+3, light_status_g);
	ctlr_dev_light_set(dev, 31+6, light_status_b);

	for(int i = 0; i < 3; i++) {
		ctlr_dev_light_set(dev, 9, light_status_b);
		usleep(800*500);
		printf("%d\n", i);
		light_status_b <<= 8;
	}
	sleep(1);
	(void)done;
#else
	printf("polling loop now..\n");
	while(!done) {
		ctlr_dev_poll(dev);
		usleep(100);
	}
#endif
	ctlr_dev_disconnect(dev);
	return 0;
}
