#pragma once

/* Functions that the script *MUST* implement */

/* Fill in the VID / PID pair this script is capable of servicing */
typedef void (*script_get_vid_pid)(int *out_vid, int *out_pid);

