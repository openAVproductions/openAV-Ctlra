/*
 * Copyright (c) 2018, OpenAV Productions,
 * Harry van Haaren <harryhaaren@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef OPENAV_CTLRA_MAPPA
#define OPENAV_CTLRA_MAPPA

#ifdef __cplusplus
extern "C" {
#endif

#include <ctlra/ctlra.h>
#include <ctlra/event.h>

/** @file
 * Mappa header file of the Ctlra library. This header provides an way to
 * provide powerful multi-layer mappings for an application without the
 * application being aware of the complexity involved.
 */

/** Structure that represents a mappa context.
 * A mappa context is used to convert generic events from the Ctlra
 * device into a form that the "host" software-application can consume.
 *
 * Expected usage of mappa is that there is one mappa_t context in the
 * whole application, and any Ctlra devices are attached to it. This
 * gives mappa a view of the "world" of controllers, and allows for
 * use-cases like one Ctlra device button changing the mappings on another
 * Ctlra device.
 */
struct mappa_t;

struct mappa_opts_t {
	uint32_t todo;
	/* do/do-not create ctlra context?
	 * Ignore specific controllers that host implements natively?
	 * ... ?
	 */
};

/** Initialize a mappa context */
struct mappa_t *mappa_create(struct mappa_opts_t *opts);

/** Run an iteration of the mappa / ctlra combo */
int32_t mappa_iter(struct mappa_t *m);

/* The callback function that must be implemented by the software
 * application. This callback will be called when a hardware event occurs
 * that has an active mapping to the software target.
 *
 * Two values are provided, the target integer id and the float value.
 * This allows for various methods of host implementation of callbacks:
 *  1) implement a single callback for each control (use macros :)
 *  2) implement a single global callback, and switch on the target_id
 *
 * With these two models available applications can choose how granular
 * to split the input handling, which is a tradeoff of developer effort
 * to create large numbers of functions, and breaking of encapsulation
 * if a global function is used for all input parameter changes.
 *
 * A void pointer for userdata is provided at a per-target level allowing
 * the application to pass a target specific class instance. This enables
 * casting to the class and calling a method locally without any lookup.
 * A global pointer can be passed instead to update the value by lookup.
 */
typedef void (*mappa_target_float_func)(uint32_t target_id,
					float value,
					void *token,
					uint32_t token_size,
					void *userdata);

/* Callback function to be implemented by the host/app. This will be
 * called when a particular source value is required to be displayed
 * on a control surface.
 */
typedef void (*mappa_source_float_func)(float *value,
					void *token,
					uint32_t token_size,
					void *userdata);

/** mappa_target_t
 * A mappa_target_t is the structure that represents a destination for
 * hardware input. The target itself is a software value, which the
 * hardware can be mapped to. Eg: A filter cutoff value for a synth.
 *
 * Name is human readable form of the thing that this represents.
 * TODO: reserve characters : - #
 *
 * Eg: Name "Drum Track" Item "Mid Eq"
 *     func: &track_eq  // host implemented callback, invoked on event
 *     userdata: &this  // class instance pointer, passed back in callback
 */
/* IMPL: target_t's are stored in a linked list (expandable) and the
 * string data is copied from the host (reliable). This linked list is
 * *NOT* used for event-to-function lookup, so no performance concern.
 *
 * The lookup of the target to execute for any input event is determined
 * by device->active_layer->target_array[event->id] for performance.
 */
struct mappa_target_t {
	/* unique name to present to a human */
	char *name;
	/* callback function and userdata */
	mappa_target_float_func func;
	void *userdata;
};

/* data structure to expose a source of feedback information to mappa.
 * This structure must be filled in by the host/application, and then
 * registered with a mappa instance. The callback will be called when
 * the source value is required.
 */
struct mappa_source_t {
	char *name;
	mappa_source_float_func func;
	void *userdata;
};

/** Add the target to the mappa instance, push it to the UI for
 * visibility, and allow ctlra inputs to be mapped to this target.
 * All targets are always available, the application is responsible for
 * calling target_remove() before freeing the resources that it uses.
 *
 * The application is not aware of the multi-layer functionality in the
 * mappa library - this is encapsulated and not exposed to keep the host
 * implementation simple, and the multi-layer complexity inside mappa.
 *
 * @param[out] target_id Passed back to callback, ID for target_remove().
 *                       NOTE: This value may *NOT* be expected to be
 *                       consistent between runs. To identify the target
 *                       using an app-known variable, use the token.
 * @param token_size Size of the token being passed in
 * @param token A chunk of data that will be returned to the callback with
 *              the userdata pointer. Put parameters to a function here, or
 *              any other data that is useful for your application.
 *
 * @retval 0 Successfully added target
 */
int32_t mappa_target_add(struct mappa_t *m,
			 struct mappa_target_t *t,
			 uint32_t *target_id,
			 void *token,
			 uint32_t token_size);

/** Remove a target from the mappa instance
 * @param m Mappa instance
 * @param unregister_id The id of the target to remove. See *mappa_target_add*
 * @param item_id The item we want to remove from *group_id*
 * @retval 0 Successfully removed target
 * @retval -EINVAL Invalid target *id* provided
 */
int32_t mappa_target_remove(struct mappa_t *m, uint32_t target_id);

/** Register a source of feedback information to the controllers.
 * Examples include play/stop state, track level meters, FX on/off states,
 * float values such as EQ, Filter Cutoffs etc. These values can be mapped
 * by the user to any control surface - LED lights, moving faders etc. The
 * data should be provided in a way that allows the user to easily map it.
 *
 * TODO: link to value mushers here
 */
int32_t mappa_source_add(struct mappa_t *m,
			 struct mappa_source_t *t,
			 uint32_t *source_id,
			 void *token,
			 uint32_t token_size);

int32_t mappa_source_remove(struct mappa_t *m, uint32_t source_id);

/* Load a config file with bindings */
int32_t mappa_load_bindings(struct mappa_t *m, const char *file);

/* cleanup a mappa instance */
void mappa_destroy(struct mappa_t *m);

#ifdef __cplusplus
}
#endif

#endif /* OPENAV_CTLRA_MAPPA */
