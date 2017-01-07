#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "ctlra/ctlra.h"

#include "libtcc.h"

static volatile uint32_t done;
static struct ctlra_dev_t* dev;

static void error_func(void *opaque, const char *msg)
{
	printf("TCC ERROR: %s\n", msg);
}

static void error(const char *msg)
{
	printf("%s\n", msg);
}

struct script_t {
	void *program;
	const char *path;
};

int script_compile_file(const char *file, struct script_t *script)
{
	TCCState *s;

	s = tcc_new();
	if(!s) {
		error("failed to create tcc context\n");
		return -2;
	}

	tcc_set_error_func(s, 0x0, error_func);
	tcc_set_options(s, "-g");
	tcc_set_output_type(s, TCC_OUTPUT_MEMORY);

	int ret = tcc_add_file(s, file);
	if(ret < 0) {
		printf("gracefully handling error now... \n");
		return -1;
	}

	script->program = malloc(tcc_relocate(s, NULL));
	if(!script->program)
		error("failed to alloc mem for program\n");
	ret = tcc_relocate(s, script->program);
	if(ret < 0)
		error("failed to relocate code to program memory\n");

	/*
	poll = (ctlr_poll)tcc_get_symbol(s, "d2_poll");
	if(!poll)
		error("failed to get de poll\n");

	handle = (void (*)(void*))tcc_get_symbol(s, "d2_handle");
	if(!handle)
		error("failed to get d2 handle\n");
	*/

	tcc_delete(s);

	/*
	uint32_t iter = 0;
	iter = poll(iter);

	struct event_t ev = { 0, 1 };
	if(iter == 1)
		ev.type = 1;
	handle(&ev);
	*/

	return 0;
}

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

	for(uint32_t i = 0; i < num_events; i++) {
		const char *pressed = 0;
		struct ctlra_event_t *e = events[i];
		const char *name = 0;
		static const char* grid_pressed[] = { " X ", "   " };
		switch(e->type) {
		case CTLRA_EVENT_BUTTON:
			name = ctlra_dev_control_get_name(dev, CTLRA_EVENT_BUTTON, e->button.id);
			printf("[%s] button %s (%d)\n",
			       e->button.pressed ? " X " : "   ",
			       name, e->button.id);
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
			break;

		case CTLRA_EVENT_GRID:
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
	printf("%s\n", __func__);
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
	*userdata_for_event_func = 0x0;

	return 1;
}

int main()
{
	signal(SIGINT, sighndlr);

	struct ctlra_t *ctlra = ctlra_create(NULL);
	int num_devs = ctlra_probe(ctlra, accept_dev_func, 0x0);

	while(!done) {
		ctlra_idle_iter(ctlra);
		usleep(100);
	}

	ctlra_exit(ctlra);

	return 0;
}
