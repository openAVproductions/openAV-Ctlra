#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

/* Ctlra header */
#include "ctlra/ctlra.h"
/* TCC header */
#include "libtcc.h"
/* Application scripting API header */
#include "application_api.h"

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
	/* A pointer to memory malloced for the generated code */
	void *program;
	/* The path of the script file */
	char *filepath;
	/* TCC should recompile the recompile and update when set */
	uint8_t recompile;
	uint8_t compile_failed;

	/* Function pointer to get the USB device this script supports */
	script_get_vid_pid get_vid_pid;
	/* Function pointer to the scripts event handling function */
	script_event_func event_func;
};

void script_free(struct script_t *s)
{
	if(s->program)
		free(s->program);
	if(s->filepath)
		free(s->filepath);
	free(s);
}

int script_compile_file(struct script_t *script)
{
#warning TODO: cleanup existing program if script already exists

	TCCState *s;
	script->compile_failed = 1;

	s = tcc_new();
	if(!s) {
		error("failed to create tcc context\n");
		return -2;
	}

	tcc_set_error_func(s, 0x0, error_func);
	tcc_set_options(s, "-g");
	tcc_set_output_type(s, TCC_OUTPUT_MEMORY);

	int ret = tcc_add_file(s, script->filepath);
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

	script->get_vid_pid = (script_get_vid_pid)
	                      tcc_get_symbol(s, "script_get_vid_pid");
	if(!script->get_vid_pid)
		error("failed to find script_get_vid_pid function\n");

	script->event_func = (script_event_func)
	                      tcc_get_symbol(s, "script_event_func");
	if(!script->event_func)
		error("failed to find script_event_func function\n");

	printf("compiled ok\n");

	tcc_delete(s);

	/*
	uint32_t iter = 0;
	iter = poll(iter);

	struct event_t ev = { 0, 1 };
	if(iter == 1)
		ev.type = 1;
	handle(&ev);
	*/

	script->compile_failed = 0;
	return 0;
}

void tcc_feedback_func(struct ctlra_dev_t *dev, void *d)
{
	/* feedback like LEDs and Screen drawing based on application
	 * state goes here. No events should be sent to the application
	 * from this function - one-way App->Ctlra updates only */
}

void tcc_event_proxy(struct ctlra_dev_t* dev,
                     uint32_t num_events,
                     struct ctlra_event_t** events,
                     void *userdata)
{
	/* This is advanced usage of the event func - if you have not
	 * already looked at the daemon example, please do so first!
	 *
	 * This function acts as a proxy for TCC to re-route the calls,
	 * but also be able to check if a script has been updated. If it
	 * has, it can swap the pointer for the event-func here, and then
	 * neither Ctlra or the App need to know what happend */
	struct script_t *script = userdata;
	if(script->event_func)
		script->event_func(dev, num_events, events, userdata);
}

void sighndlr(int signal)
{
	done = 1;
	printf("\n");
}

void remove_dev_func(struct ctlra_dev_t *dev, int unexpected_removal,
                     void *userdata)
{
	struct script_t *script = userdata;
	printf("%s, script = %p\n", __func__, script);
	script_free(script);
}

int accept_dev_func(const struct ctlra_dev_info_t *info,
                    ctlra_event_func *event_func,
                    ctlra_feedback_func *feedback_func,
                    ctlra_remove_dev_func *remove_func,
                    void **userdata_for_event_func,
                    void *userdata)
{
	static int accepted;

	/* Just one ctlra for now */
	if(accepted)
		return 0;

	struct script_t *script = calloc(1, sizeof(struct script_t));
	if(!script) return 0;

	script->filepath = strdup("/tmp/ni_d2_script.c");

	script_compile_file(script);
	if(script->compile_failed) {
		printf("tcc: warning, compilation of script failed"
		       "refusing %s %s\n", info->vendor, info->device);
		script_free(script);
		return 0;
	}

	int vid = -1;
	int pid = -1;
	script->get_vid_pid(&vid, &pid);
	if(vid != info->vendor_id || pid != info->device_id) {
		script_free(script);
		return 0;
	}

	printf("tcc: accepting %s %s, script = %p\n",
	       info->vendor, info->device, script);
	*event_func = tcc_event_proxy;
	*feedback_func = tcc_feedback_func;
	*remove_func = remove_dev_func;
	*userdata_for_event_func = script;

	accepted = 1;

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
