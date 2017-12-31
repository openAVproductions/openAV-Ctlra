#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>

/* Ctlra header */
#include "ctlra.h"
/* TCC header */
#include "libtcc.h"
/* Application scripting API header */
#include "application_api.h"

#include "loopa.h"

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

static int file_modify_time(const char *path, time_t *new_time)
{
	if(new_time == 0)
		return -1;
	struct stat file_stat;
	int err = stat(path, &file_stat);
	if (err != 0) {
		return -2;
	}
	*new_time = file_stat.st_mtime;
	return 0;
}

struct script_t {
	/* A pointer to memory malloced for the generated code */
	void *program;
	/* The path of the script file */
	char *filepath;
	/* TCC should recompile the recompile and update when set */
	uint8_t recompile;
	uint8_t compile_failed;
	/* Time the script file was last modified */
	time_t time_modified;

	/* Function pointer to get the USB device this script supports */
	script_get_vid_pid get_vid_pid;
	/* Function pointer to the scripts event handling function */
	script_event_func event_func;
	/* Function pointer to the scripts feedback handling function */
	script_feedback_func feedback_func;
	script_screen_redraw_func screen_redraw_func;

	/* The malloc() / free() memory from the script */
	void *script_ud;
};

void script_free(struct script_t *s)
{
	if(s->program)
		free(s->program);
	if(s->filepath)
		free(s->filepath);
	free(s);
}

void script_reset(struct script_t *s)
{
	s->event_func = 0x0;
	s->get_vid_pid = 0x0;
	if(s->program)
		free(s->program);
}

struct loopa_symbol_t {
	char name[64];
	void *func_ptr;
};

static struct loopa_symbol_t loopa_symbols[] = {
	{"loopa_reset", loopa_reset},
	{"loopa_playing_toggle", loopa_playing_toggle},
	{"loopa_record_toggle", loopa_record_toggle},
	{"loopa_play_get", loopa_play_get},
	{"loopa_reverb", loopa_reverb},
	{"loopa_rec_get", loopa_rec_get},
	{"loopa_vol_get", loopa_vol_get},
	{"loopa_vol_set", loopa_vol_set},
	{"loopa_playing", loopa_playing},
	{"loopa_recording", loopa_recording},
	{"ctlra_dev_light_set", ctlra_dev_light_set},
	{"ctlra_dev_light_flush", ctlra_dev_light_flush},
	{"ctlra_dev_screen_get_data", ctlra_dev_screen_get_data},
};

#define LOOPA_SYMBOLS_SIZE (sizeof(loopa_symbols) / sizeof(loopa_symbols[0]))

int script_compile_file(struct script_t *script)
{
	script_reset(script);

	printf("tcc_script example: compiling %s\n", script->filepath);
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

	for(int i = 0; i < LOOPA_SYMBOLS_SIZE; i++) {
		if (tcc_add_symbol(s, loopa_symbols[i].name,
					 loopa_symbols[i].func_ptr)) {
			error("failed to insert get rec() symbol\n");
			tcc_delete(s);
			return -1;
		}
	}

	int ret;
	ret = tcc_add_file(s, script->filepath);
	if(ret < 0) {
		printf("gracefully handling error now... \n");
		return -1;
	}

	script->program = malloc(tcc_relocate(s, NULL));
	if(!script->program) {
		error("failed to alloc mem for program\n");
		return -1;
	}

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

	script->feedback_func = (script_feedback_func)
	                      tcc_get_symbol(s, "script_feedback_func");
	if(!script->feedback_func)
		error("failed to find script_feedback_func\n");

	script->screen_redraw_func = (script_screen_redraw_func)
	                      tcc_get_symbol(s, "script_screen_redraw_func");
	if(!script->feedback_func)
		error("failed to find script_feedback_func\n");

	tcc_delete(s);

	int err = file_modify_time(script->filepath,
				   &script->time_modified);
	if(err) {
		printf("%s: error getting file modified time\n", __func__);
		return -1;
	}

	script->compile_failed = 0;
	return 0;
}

void tcc_feedback_func(struct ctlra_dev_t *dev, void *userdata)
{
	/* feedback like LEDs and Screen drawing based on application
	 * state goes here. No events should be sent to the application
	 * from this function - one-way App->Ctlra updates only */

	struct script_t *script = userdata;
	if(script->feedback_func)
		script->feedback_func(dev, userdata);
}


int tcc_screen_redraw_proxy(struct ctlra_dev_t *dev, uint32_t screen_idx,
			    uint8_t *pixel_data, uint32_t bytes,
			    void *userdata)
{
	struct script_t *script = userdata;

	if(script->screen_redraw_func)
		return script->screen_redraw_func(dev, screen_idx,
						  pixel_data, bytes, userdata);

	/* zero means no redraw */
	return 0;
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

	static int do_once;
	if(!do_once) {
		int32_t ret = ctlra_dev_screen_register_callback(dev, 30,
								 tcc_screen_redraw_proxy,
								 script);
		if(ret)
			printf("WARNING: Failed to register screen callback\n");
	}


	/* Check if we need to recompile script based on modified time of
	 * the script file, comparing with the compiled modified time */
	time_t new_time;
	int err = file_modify_time(script->filepath,
				   &new_time);
	if(err) {
		printf("%s: error getting file modified time\n", __func__);
	}
	if(new_time > script->time_modified) {
		printf("tcc: recompiling script %s\n", script->filepath);
		// trigger recompile, which will update modified timestamp
		script_compile_file(script);
	}

	/* Handle events */
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

	script->filepath = strdup("loopa_mk3.c");

	script_compile_file(script);
	if(script->compile_failed) {
		printf("tcc: warning, compilation of script failed "
		       "refusing %s %s\n", info->vendor, info->device);
		script_free(script);
		return 0;
	}

	if(!script->get_vid_pid) {
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

	loopa_init();

	while(!done) {
		ctlra_idle_iter(ctlra);
		usleep(10 * 1000);
	}

	loopa_exit();

	ctlra_exit(ctlra);

	return 0;
}
