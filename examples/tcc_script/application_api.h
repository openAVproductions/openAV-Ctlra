#pragma once

/* Functions that the script *MUST* implement */

/* Fill in the VID / PID pair this script is capable of servicing */
typedef void (*script_get_vid_pid)(int *out_vid, int *out_pid);

/* The script function that handles events. In the script implementation
 * of this function is where your "users" will be writing code */
typedef void (*script_event_func)(struct ctlra_dev_t* dev,
                                  unsigned int num_events,
                                  struct ctlra_event_t** events,
                                  void *userdata);

