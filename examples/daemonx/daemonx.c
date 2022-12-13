#define _DEFAULT_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <fnmatch.h>
#include <time.h>

#include <cairo/cairo.h>

#include "ctlra.h"
#include "ctlra_cairo.h"
#include "midi.h"

static volatile uint32_t done, screen_count;

// AG: The num_dev counter keeps track of the number of connected devices, and
// the auto_exit option, when enabled, causes the program to exit
// automatically when that count drops to zero. This makes it easier to use
// the program in some situations (e.g., if the program is relaunched
// automatically via udev each time a certain device comes online).
static volatile int num_devs, auto_exit;

// AG: maximum number of devices to open (zero means any number).
static int max_devs;

// AG: Color palette for mapping velocities, controller values etc. to
// colors. For now, just map 0..7 to the customary ANSI terminal colors, we
// might do something more comprehensive in the future.

uint32_t ansi_colors[] =
  {
   0x00000000, // black
   0xffff0000, // red
   0xff00ff00, // green
   0xffffff00, // yellow
   0xff0000ff, // blue
   0xffff00ff, // magenta
   0xff00ffff, // cyan
   0xffffffff, // white
  };

static uint32_t ansi_col(uint8_t c)
{
  if (c > 7)
    return ansi_colors[7]; // white
  else
    return ansi_colors[c];
}

// AG: We employ a screen layout with 6 lines, each with 31 characters. This
// seems to work nicely with informative text, and still leaves room for
// timecode and meter displays, scribble strips and the like. In the current
// implementation, the text can be set and cleared using custom sysex messages
// in the MIDI input. Also, as an option, MCP timecode, meter displays and
// scribble strips can be constructed automagically from MCP feedback in the
// MIDI input. These are superimposed on the display, with the 2nd scribble
// line covering an unused extra line at the bottom. There's some unassigned
// space at the top as well, which might be used for future extensions.

#define SCREEN_ROWS 6
#define SCREEN_COLS 31

// AG: Bit masks for the various feedback options; these can also be set via
// the command line and custom sysex messages. The meaning of the different
// options is as follows:

// FB_LOCAL: enables local feedback (LEDs lighting up when you push a button,
// etc.) for all buttons including the grid and (on the Maschine Mk3 at least)
// the touchstrip and the up/left/right/down LEDs of the main push encoder.

// FB_MIDI: enables MIDI feedback for all of the above (receiving the
// corresponding note message in MIDI input lights the LED in the color
// indicated by the velocity value, using the colors from the ANSI color
// palette above).

// FB_MCP: enables some special kinds of MCP (Mackie control protocol)
// feedback messages in MIDI input. Currently, this comprises timecode and
// meter feedback, as well as setting the scribble strips and the
// record/solo/mute indicators for each mixer channel.

enum { FB_NONE = 0, FB_LOCAL = 1, FB_MIDI = 2, FB_MCP = 4, FB_ALL = 7 };
uint8_t feedback;

// There's a second bitmask for enabling and disabling individual elements.
// These are all enabled by default, but can be turned on and off individually
// for customization purposes.

// FB_GRID, FB_BUTTONS, FB_CONTROLS: enables (local and MIDI) feedback for the
// grid, all other buttons, and all other controls (sliders).

// FB_TEXT, FB_TIMECODE, FB_STRIPS, FB_RSM: enables various display elements
// (ordinary text output, MCP timecode, MCP scribble+meter strips. MCP
// rec/solo/mute indicators).

enum { FB_NO_ITEMS = 0, FB_GRID = 1, FB_BUTTONS = 2, FB_CONTROLS = 4,
       FB_TEXT = 8, FB_TIMECODE = 0x10, FB_STRIPS = 0x20, FB_RSM = 0x40,
       FB_ALL_ITEMS = 0x7f };
uint8_t feedback_items = FB_ALL_ITEMS;

/* a struct to pass around as userdata to callbacks */
struct daemon_t {
	struct ctlra_dev_t* dev;
	struct ctlra_midi_t *midi;
	// local copy of feedback options
	uint8_t feedback, feedback_items;
	// keep track of feedback items
	int grid_size, button_count, encoder_count, slider_count;
	// grid (pads on grid #0)
	uint32_t *grid_col;
	uint8_t *grid;
	// buttons
	uint32_t *but_col;
	uint8_t *but;
	// encoders+sliders
	uint32_t *ctl_col;
	uint8_t *ctl;
	// display text
	int char_width, char_height, ascent;
	uint8_t screen_fg[2][SCREEN_ROWS][SCREEN_COLS];
	uint8_t screen_bg[2][SCREEN_ROWS][SCREEN_COLS];
	uint8_t screen_idx[2][SCREEN_ROWS][SCREEN_COLS];
	char screen_text[2][SCREEN_ROWS][SCREEN_COLS+1];
	// MCP timecode display
	char timecode_frames[4], timecode_s[3], timecode_m[3], timecode_h[4];
	// MCP scribble strips
	uint32_t scribble_col[8];
	char scribble_text[8][2][8];
	// MCP meter display
	uint8_t meter[8];
	// MCP rec/solo/mute
	uint8_t rsm_status[3][8];
	// screenshot flag
	uint8_t screenie[2];
	// screen initialization and finalization
	uint8_t screen_init[2], screen_fini[2];
	// cairo surface and context
        cairo_surface_t *img;
        cairo_t *cr;
	struct ctlra_dev_info_t info;
};

void daemon_feedback_func(struct ctlra_dev_t *dev, void *d)
{
	struct daemon_t *daemon = d;
        ctlra_midi_input_poll(daemon->midi);

	if (!(daemon->feedback)) return;

	/* lights for grids */
	if(daemon->grid_size && (daemon->feedback_items&FB_GRID)) {
	  for(int i = 0; i < daemon->grid_size; i++) {
	    int id = daemon->info.grid_info[0].info.params[0] + i;
	    uint32_t col = daemon->grid_col[i] * (daemon->grid[i] > 0);
	    ctlra_dev_light_set(dev, id, col);
	  }
	}
	/* lights for buttons */
	if(daemon->button_count && (daemon->feedback_items&FB_BUTTONS)) {
	  for(int i = 0; i < daemon->button_count; i++) {
	    struct ctlra_item_info_t *item =
	      &daemon->info.control_info[CTLRA_EVENT_BUTTON][i];
	    if(item->flags & CTLRA_ITEM_HAS_FB_ID) {
	      int id = item->fb_id;
	      uint32_t col = daemon->but_col[i] * (daemon->but[i] > 0);
	      ctlra_dev_light_set(dev, id, col);
	    }
	  }
	}
	// AG XXXTODO: feedback for sliders
	// AG: Special-casing the touchstrip of the Mk3.
	if (daemon->info.vendor_id == 0x17cc &&
	    daemon->info.device_id == 0x1600) {
	  if(daemon->slider_count && (daemon->feedback_items&FB_CONTROLS)) {
	    // The touchstrip has 25 LEDs, scale the controller value
	    // accordingly.
	    int val = daemon->ctl[daemon->encoder_count]/5;
	    int col = daemon->ctl_col[daemon->encoder_count];
	    for (int i = 0; i < 25; i++) {
	      int id = 62+i;
	      if (i < val)
		ctlra_dev_light_set(dev, id, col);
	      else
		ctlra_dev_light_set(dev, id, 0);
	    }
	  }
	}

	ctlra_dev_light_flush(dev, 0);
}

/* Rationale of the MIDI mapping: We want to accommodate both the different
   kinds of device controls (grid, buttons, encoders, sliders) and MCP
   feedback without producing any clashes. The easy part are the encoders and
   sliders which are mapped to corresponding MIDI controllers, starting at
   CC0, first the encoders then the sliders. On some devices like the Maschine
   Mk3, there's also a special control for the sustain pedal, which gets
   treated like a button, but is better represented as a MIDI controller, so
   we map it to CC64, the MIDI controller for the sustain pedal. Thus,
   assuming that there are n encoders and m sliders, the layout of the MIDI
   controllers is as follows (all on MIDI channel #1):

   - CC(0) .. CC(n-1): encoders
   - CC(n) .. CC(n+m-1): sliders
   - CC(64): pedal (if available)

   The mapping of buttons to note messages is a bit more intricate, since some
   MIDI notes (0..23) are used in MCP feedback, others by the grid (typically
   4x4, 8x8 in some cases) or by the other device buttons (some devices like
   the Maschine Mk3 have a substantial number of these in addition to the
   grid). It's hard to accommodate all of these on a single MIDI
   channel. Instead, considering that the grid buttons are often used to play
   a drumkit, we map these to notes on MIDI channel #10 (the GM drum channel),
   starting at note C3 = 36 (the GM kick drum). That way they will often work
   out of the box, and they can also be singled out easily and remapped if
   needed. The other buttons are mapped to MIDI notes on MIDI channel #1
   starting at C2 = 24, leaving room for the MCP rec/solo/mute feedback
   messages. The resulting layout looks like this (assuming a grid size of p
   and q other buttons):

   On MIDI channel #1:

   - Note 0  (C0) .. 23 (B1): reserved for MCP feedback
   - Note 24 (C2) .. 24+q-1: device buttons

   On MIDI channel #10:

   - Note 36 (C3) .. 36+p-1: Grid (drum pads)

   For introspection purposes, the layout parameters (n, m, p, q), as well as
   vendor and device id of the hardware are available by means of a special
   sysex message. The program recognized various sysex messages for special
   purposes, such as writing contents on the screen, and handling MCP
   feedback. The following types of sysex messages are available:

   (1) Non-realtime:

   F0 7E 7F 06 01 F7: identity request

   The reply takes the form F0 7E 7F 06 02 7D 0v 0v 0v 0v 0d 0d 0d 0d F7,
   where v v v v denotes the four hex digits of the vendor (most significant
   digit first), and d d d d the four hex digits of the device id. These are
   as reported by the ctlra library; usually this will be the usb vendor/
   device id, but may also be zero for devices not accessed through the usb
   library.

   (2) Custom (ctlra_daemon-specific):

   F0 7D 00 ss xx yy fg bg c1 c2 ... F7: print text to screen

   Outputs the given characters c1 c2 ... (7 bit ASCII) at position xx, yy
   (xx = column in the range 0..30, yy = row in the range 0..5) on screen ss
   (which must be 0 or 1 in the current implementation). fg and bg specify the
   foreground and background colors as indices 0..7 into the ANSI color
   palette (see above). The first null byte in the sequence c1 c2 ... causes
   the rest of the line to be cleared.

   F0 7D 01 ss F7: clear screen

   Clear screen ss (0 or 1). ss can also be omitted, in which case both
   screens are cleared.

   F0 7D 02 ss F7: take a screenshot

   Capture the contents of screen ss (0 or 1). ss can also be omitted, in
   which case both screens are captured. The output is written to a png file
   (named screen0.png for screen 0, and screen1.png for screen 1) in
   ctlra_daemon's working directory.

   F0 7D 40 vv mm F7: set feeback options

   vv must be 0 (off) or 1 on, and mm denotes the bitmask of feedback options
   (consisting of any of the FB values in the first enum above) to be turned
   off and on, respectively.

   F0 7D 41 vv mm F7: set feeback items

   vv must be 0 (off) or 1 on, and mm denotes the bitmask of feedback items
   (consisting of any of the FB values in the second enum above) to be turned
   off and on, respectively.

   F0 7D 42 01 F7: layout request

   Reports the control layout (numbers n, m, p, q, see above) in a sysex
   message of the following form: F0 7D 42 02 nn mm pp qq F7

   (3) MCP-compatible: The MCP message for setting the "scribble strips" is
   recognized and produces the appropriate output on both screens (mixer
   channel 1..4 on screen 0, channels 5..8 on screen 1). */

static void init_timecode(struct daemon_t *daemon)
{
  if (!*daemon->timecode_frames)
    strcpy(daemon->timecode_frames, "   ");
  if (!*daemon->timecode_s)
    strcpy(daemon->timecode_s, "  ");
  if (!*daemon->timecode_m)
    strcpy(daemon->timecode_m, "  ");
  if (!*daemon->timecode_h)
    strcpy(daemon->timecode_h, "   ");
}

int daemon_input_cb(uint8_t nbytes, uint8_t * buffer, void *ud)
{
	struct daemon_t *daemon = ud;
	int status = buffer[0],
	  data = nbytes>1?buffer[1]:0,
	  val = nbytes>2?buffer[2]:0;
#if 0
	fprintf(stderr, "->");
	for (int i = 0; i < nbytes; i++)
	  fprintf(stderr, " %0x", buffer[i]);
	fprintf(stderr, "\n");
#endif

	// Turn note-offs into note-ons with velocity 0, to simplify
	// processing.
	if (status == 0x80) {
	  status = 0x90; val = 0;
	}
	// This reverses the mapping from daemon_event_func below in order to
	// light the corresponding leds, and also adds some convenient extras
	// such as timecode and scribble strip displays.
	switch (status) {
	case 0x99:
	  if (!(daemon->feedback&FB_MIDI)) break;
	  if (daemon->grid_size && data >= 36) {
	    // AG XXXFIXME: this only works with 4x4 grids
	    int pos = data-36;
	    if (pos >= daemon->grid_size) break;
	    int row = 3 - (pos / 4);
	    int col = (pos % 4);
	    pos = (row * 4) + col;
	    daemon->grid[pos] = val>0;
	    if (val>0)
	      daemon->grid_col[pos] = ansi_col(val);
	  }
	  break;
	case 0x90:
	  if (data < 24) {
	    // MCP rec/solo/mute feedback
	    if (!(daemon->feedback&FB_MCP)) break;
	    daemon->rsm_status[data/8][data%8] = val>0;
	  } else if (data-24 < daemon->button_count) {
	    // normal button feedback
	    if (!(daemon->feedback&FB_MIDI)) break;
	    daemon->but[data-24] = val>0;
	    if (val>0)
	      daemon->but_col[data-24] = ansi_col(val);
	  }
	  break;
	case 0xb0:
	  if (data >= daemon->encoder_count &&
	      data < daemon->encoder_count+daemon->slider_count) {
	    // slider feedback (no feedback for encoders)
	    if (!(daemon->feedback&FB_MIDI)) break;
	    daemon->ctl[data] = val;
	    if (val>0)
	      daemon->ctl_col[data] = 0xff0040ff;
	  } else if (data >= 64 && data <= 73) {
	    if (!(daemon->feedback&FB_MCP)) break;
	    // MCP-compatible time code feedback, from least to most signifact
	    // digits: cc64..66: frames/ticks; cc67..68: seconds/subdivision;
	    // cc69..70: minutes/beats; cc71..73: hours/bars. These are all
	    // encoded in ASCII, so we just record them here and leave the
	    // formatting up to the screen display routine.
	    init_timecode(daemon);
	    switch (data) {
	    case 64: case 65: case 66:
	      daemon->timecode_frames[66-data] = val;
	      break;
	    case 67: case 68:
	      daemon->timecode_s[68-data] = val;
	      break;
	    case 69: case 70:
	      daemon->timecode_m[70-data] = val;
	      break;
	    case 71: case 72: case 73:
	      daemon->timecode_h[73-data] = val;
	      break;
	    default:
	      break;
	    }
	  }
	  break;
	case 0xd0:
	  // MCP-compatible meter display
	  if (!(daemon->feedback&FB_MCP)) break;
	  daemon->meter[data/16] = data%16;
	  break;
	case 0xf0: // sysex
	  if (buffer[nbytes-1] != 0xf7) break;
	  if (data == 0x7d) {
	    // ctrlra-specific (these are always active even if all feedback
	    // options are disabled); byte #1 is id (0x7d for educational
	    // use), byte #2 denotes the function (see below), byte #3
	    // optionally the corresponding parameter (usually the screen
	    // index; this *must* be 0 or 1)
	    if (nbytes <= 3) break;
	    int fun = val, param = buffer[3], missing = param == 0xf7;
	    if (missing) param = 0; // missing param, treated as zero
	    if (param > 1) break;
	    switch (fun) {
	    case 0: {
	      // text output: x,y coordinates (0..30, 0..5), followed by ANSI
	      // color index of foreground and background color, and 7 bit
	      // ASCII chars (a null byte clears the rest of the line)
	      if (nbytes <= 8) break;
	      int x = buffer[4], y = buffer[5], fg = buffer[6], bg = buffer[7];
	      if (x >= SCREEN_COLS || y >= SCREEN_ROWS) break;
	      if (fg > 7) fg = 7;
	      if (bg > 7) bg = 7;
	      int l = strlen(daemon->screen_text[param][y]);
	      if (l < x)
		// pad with blanks
		memset(daemon->screen_text[param][y]+l, ' ', x-l);
	      for (int i = 8, j = x; i < nbytes-1 && j < SCREEN_COLS;
		   i++, j++) {
		daemon->screen_text[param][y][j] = buffer[i];
		daemon->screen_fg[param][y][j] = fg;
		daemon->screen_bg[param][y][j] = bg;
		daemon->screen_idx[param][y][j] = x;
	      }
	      break;
	    }
	    case 1:
	      // clear display
	      for (int l = 0; l < SCREEN_ROWS; l++) {
		if (param == 0 || missing)
		  daemon->screen_text[0][l][0] = 0;
		if (param == 1 || missing)
		  daemon->screen_text[1][l][0] = 0;
	      }
	      break;
	    case 2:
	      // take screenshot
	      if (param == 0 || missing)
		daemon->screenie[0] = 1;
	      if (param == 1 || missing)
		daemon->screenie[1] = 1;
	      break;
	    // other values < 0x40 reserved for future extensions
	    case 0x40: {
	      // set feedback options specified as a bitmask (param = 0
	      // disables, param = 1 enables the given options)
	      if (nbytes <= 4) break;
	      uint8_t mask = buffer[4];
	      if (param)
		daemon->feedback |= mask;
	      else
		daemon->feedback &= ~mask;
	      break;
	    }
	    case 0x41: {
	      // set feedback items specified as a bitmask (param = 0
	      // disables, param = 1 enables the given items)
	      if (nbytes <= 4) break;
	      uint8_t mask = buffer[4];
	      if (param)
		daemon->feedback_items |= mask;
	      else
		daemon->feedback_items &= ~mask;
	      break;
	    }
	    case 0x42: {
	      // F0 7D 42 01 F7: layout request
	      if (param != 1) break;
	      uint8_t n = daemon->encoder_count, m = daemon->slider_count;
	      uint8_t p = daemon->grid_size, q = daemon->button_count;
	      // F0 7D 42 02 nn mm pp qq F7: layout reply
	      uint8_t msg[9] = {0xf0, 0x7d, 0x42, 0x02, n, m, p, q, 0xf7};
	      ctlra_midi_output_write(daemon->midi, 9, msg);
	      break;
	    }
	    default:
	      break;
	    }
	  } else if (data == 0x7e && nbytes == 6 &&
		     buffer[3] == 0x06 && buffer[4] == 0x01) {
	    // F0 7E 7F 06 01 F7: identity request
	    uint16_t v = daemon->info.vendor_id, d = daemon->info.device_id;
	    uint8_t vd[4], dd[4];
	    for (int i = 0; i < 4; i++) {
	      vd[i] = v&0xf; v >>= 4;
	      dd[i] = d&0xf; d >>= 4;
	    }
	    // F0 7E 7F 06 02 7D 0v 0v 0v 0v 0d 0d 0d 0d F7: identity reply
	    uint8_t msg[15] = {0xf0, 0x7e, 0x7f, 0x06, 0x02, 0x7d,
			       vd[3], vd[2], vd[1], vd[0],
			       dd[3], dd[2], dd[1], dd[0], 0xf7};
	    ctlra_midi_output_write(daemon->midi, 15, msg);
	  } else if (data == 0 && val == 0 && nbytes > 3 && buffer[3] == 0x66) {
	    if (!(daemon->feedback&FB_MCP)) break;
	    // Mackie-compatible (we ignore the device number, function is
	    // 0x12 for scribble strip display)
	    int /*devno = nbytes>4?buffer[4]:0,*/ fun = nbytes>5?buffer[5]:0;
	    if (fun == 0x12 && nbytes > 6 && nbytes <= 15) {
	      // MCP scribble strip message: at most 8 bytes follow, first
	      // byte indicates the position, the rest is the actual text
	      // encoded in 7 bit ASCII. There are 2 lines, each with 8 labels
	      // (one for each mixer channel) with at most 7 chars each. The
	      // 2nd line is at an offset of 8*7=56.
	      int pos = buffer[6]; // chan * 7 + lineno * 56
	      int lineno = pos/56, chan = (pos%56)/7, offs = (pos%56)%7;
	      if (lineno > 1 || chan > 7)
		// position outside the valid range, bail out
		break;
	      // Since we have the scribble strips split up for easier
	      // processing in the screen display routine, we need to copy the
	      // data over and truncate it if necessary.
	      int l = nbytes-8, k = strlen(daemon->scribble_text[chan][lineno]);
	      if (l > 7-offs ) l = 7-offs; // number of bytes to copy
	      if (k < offs)
		// If the current text runs short, pad it with blanks so that
		// we can append our contents at the indicated offset.
		memset(daemon->scribble_text[chan][lineno]+k, ' ', offs-k);
	      // copy the data
	      strncpy(daemon->scribble_text[chan][lineno]+offs,
		      (char*)buffer+7, l);
	      // terminate with null byte
	      daemon->scribble_text[chan][lineno][offs+l] = 0;
	    }
	  }
	  break;
	default:
	  break;
	}
	return 0;
}

void daemon_event_func(struct ctlra_dev_t* dev,
		       uint32_t num_events,
		       struct ctlra_event_t** events,
		       void *userdata)
{
	struct daemon_t *daemon = userdata;
	struct ctlra_midi_t *midi = daemon->midi;
	uint8_t msg[3] = {0};

	for(uint32_t i = 0; i < num_events; i++) {
		struct ctlra_event_t *e = events[i];
		int ret;
		switch(e->type) {
		case CTLRA_EVENT_BUTTON:
			if (e->button.id >= daemon->button_count) {
			  if (e->button.id == daemon->button_count) {
			    // The Maschine Mk3 has the pedal under that
			    // index, not sure about other backends. We map
			    // this to the hold pedal (CC64).
			    msg[0] = 0xb0;
			    msg[1] = 64;
			    msg[2] = e->button.pressed ? 0x7f : 0;
			    ret = ctlra_midi_output_write(midi, 3, msg);
			  }
			  break;
			}
			if ((daemon->feedback&FB_LOCAL) &&
			    (daemon->feedback_items&FB_BUTTONS)) {
			  int id = e->button.id;
			  struct ctlra_item_info_t *item = &daemon->info.control_info[CTLRA_EVENT_BUTTON][id];
			  if(item && item->flags & CTLRA_ITEM_HAS_FB_ID) {
			    daemon->but_col[id] = 0xffffffff;
			    daemon->but[id] = e->button.pressed;
			  }
			}
			msg[0] = 0x90;
			msg[1] = e->button.id+24;
			msg[2] = e->button.pressed ? 0x7f : 0;
			ret = ctlra_midi_output_write(midi, 3, msg);
			break;

		case CTLRA_EVENT_ENCODER:
			msg[0] = 0xb0;
			msg[1] = e->encoder.id;
			{
			  int val =
			    (e->encoder.flags &
			     CTLRA_EVENT_ENCODER_FLAG_INT) ?
			    // Encoder value is integer, take as is.
			    e->encoder.delta :
			    // Encoder value is float, so we need to scale
			    // it. The maximum absolute value is 1.0 denoting
			    // a full rotation, which we map to 63 (the
			    // maximum encoder value in MIDI). We also round
			    // the result away from zero, to ensure a nonzero
			    // value for nonzero inputs.
			    (int)roundf(e->encoder.delta_float * 63.f +
					(e->encoder.delta_float>=0?0.5:-0.5));
			  msg[2] = (val >= 0)?val:64-val;
			}
			ret = ctlra_midi_output_write(midi, 3, msg);
			break;

		case CTLRA_EVENT_SLIDER:
			msg[0] = 0xb0;
			int id = daemon->encoder_count+e->slider.id;
			if ((daemon->feedback&FB_LOCAL) &&
			    (daemon->feedback_items&FB_CONTROLS)) {
			  int val = (int)(e->slider.value * 25.f);
			  daemon->ctl[id] = val;
			  if (val>0)
			    daemon->ctl_col[id] = 0xff0040ff;
			}
			int val = (int)(e->slider.value * 127.f);
			msg[1] = id;
			msg[2] = val;
			ret = ctlra_midi_output_write(midi, 3, msg);
			break;

		case CTLRA_EVENT_GRID: {
			msg[0] = 0x99; // GM drum channel
			int pos = e->grid.pos;
			if ((daemon->feedback&FB_LOCAL) &&
			    (daemon->feedback_items&FB_GRID)) {
			  daemon->grid_col[pos] = 0xff0040ff;
			  daemon->grid[pos] = e->grid.pressed;
			}
			/* transform grid */
			// AG XXXFIXME: this only works with 4x4 grids
			int row = 3 - (pos / 4);
			int col = (pos % 4);
		        pos = (row * 4) + col;
			msg[1] = pos + 36; /* GM kick drum note */
			msg[2] = e->grid.pressed ?
					e->grid.pressure * 127 : 0;
			ret = ctlra_midi_output_write(midi, 3, msg);
			break;
		}
		default:
			break;
		};
		// TODO: Error check midi writes
		(void) ret;
	}
}

int32_t daemon_screen_redraw_func(struct ctlra_dev_t *dev,
				  uint32_t screen_idx,
				  uint8_t *pixel_data,
				  uint32_t bytes,
				  struct ctlra_screen_zone_t *redraw_zone,
				  void *userdata)
{
	struct daemon_t *daemon = userdata;

	if(!daemon->img) {
		//daemon->img = cairo_image_surface_create(CAIRO_FORMAT_RGB24,
		daemon->img = cairo_image_surface_create(CAIRO_FORMAT_RGB16_565,
							 480, 272);
		daemon->cr = cairo_create(daemon->img);
	}

	/* blank out bg */
        cairo_t *cr = daemon->cr;
	cairo_set_source_rgb(cr, 0, 0, 0);
	cairo_rectangle(cr, 0, 0, 480, 272);
	cairo_fill(cr);

	// AG XXXFIXME: We need to go to considerable lengths here just to get
	// the screen cleared before we exit; maybe this should be handled in
	// ctlra_exit() in some way?
	if (done) {
	  // simply clear the screen and bail out
	  if (!daemon->screen_fini[screen_idx]) {
	    ctlra_screen_cairo_to_device
	      (dev, screen_idx, pixel_data, bytes, redraw_zone, daemon->img);
	    daemon->screen_fini[screen_idx] = 1;
	    screen_count--;
	    return 1;
	  } else
	    // already done with finalization
	    return 0;
	} else if (!daemon->screen_init[screen_idx]) {
	  screen_count++;
	  daemon->screen_init[screen_idx] = 1;
	}

#if 0
	// uncomment this and edit as needed to choose a typeface different
	// from cairo's default sans serif font
	cairo_select_font_face(cr, "Mono",
			       CAIRO_FONT_SLANT_NORMAL,
			       CAIRO_FONT_WEIGHT_BOLD);
#endif
	cairo_set_font_size(cr, 24);
	if (!daemon->char_width) {
	  // estimate the average character width (should be about 15 with the
	  // default font, otherwise we're probably in trouble)
	  cairo_text_extents_t extents;
	  cairo_text_extents(cr, "0123456789", &extents);
	  daemon->char_width = round(extents.width/10.0);
	  //fprintf(stderr, "char width = %d\n", daemon->char_width);
	  cairo_font_extents_t fextents;
	  cairo_font_extents(cr, &fextents);
	  daemon->char_height = round(fextents.height);
	  daemon->ascent = round(fextents.ascent);
	}
	int cw = daemon->char_width, ch = daemon->char_height,
	  asc = daemon->ascent;

	if(screen_idx <= 1) {
	  if ((daemon->feedback_items&FB_TEXT)) {
	    // 6 lines of text
	    for (int i = 0; i < SCREEN_ROWS; i++) {
	      if (*daemon->screen_text[screen_idx][i]) {
		// AG: This is a super-simplistic implementation which spaces
		// columns and lines evenly on a cw x 30 grid. Which is easy to
		// use and works fine if all you want to do is throw up some
		// bits of colored text on the screen somewhere. We could do
		// something more comprehensive in order to properly deal with
		// proportional fonts, but that seems overkill and would become
		// much more complicated.
		char *s = daemon->screen_text[screen_idx][i];
		uint8_t fg = daemon->screen_fg[screen_idx][i][0];
		uint8_t bg = daemon->screen_bg[screen_idx][i][0];
		uint8_t idx = daemon->screen_idx[screen_idx][i][0];
		int j = 0, l = strlen(s);
		while (j < l) {
		  uint32_t fgcol = ansi_col(fg);
		  uint8_t fg_r = (fgcol&0xff0000)>>16,
		    fg_g = (fgcol&0xff00)>>8, fg_b = (fgcol&0xff);
		  uint32_t bgcol = ansi_col(bg);
		  uint8_t bg_r = (bgcol&0xff0000)>>16,
		    bg_g = (bgcol&0xff00)>>8, bg_b = (bgcol&0xff);
		  char buf[SCREEN_COLS+1];
		  int k = j;
		  while (k < SCREEN_COLS &&
			 daemon->screen_idx[screen_idx][i][k] == idx)
		    k++;
		  strncpy(buf, s+j, k-j); buf[k-j] = 0;
		  //fprintf(stderr, "-> '%s' at %d,%d with %0x, %0x\n", buf, j*cw+5, (i+2)*30, fgcol, bgcol);
		  cairo_text_extents_t extents;
		  cairo_text_extents(cr, buf, &extents);
		  int wd = round(extents.x_advance);
		  cairo_set_source_rgb(cr, bg_r/255.f, bg_g/255.f, bg_b/255.f);
		  cairo_rectangle(cr, j*cw+5, (i+2)*30-asc, wd, ch);
		  cairo_fill(cr);
		  cairo_set_source_rgb(cr, fg_r/255.f, fg_g/255.f, fg_b/255.f);
		  cairo_move_to(cr, j*cw+5, (i+2)*30);
		  cairo_show_text(cr, buf);
		  j = k;
		  if (k < SCREEN_COLS) {
		    fg = daemon->screen_fg[screen_idx][i][k];
		    bg = daemon->screen_bg[screen_idx][i][k];
		    idx = daemon->screen_idx[screen_idx][i][k];
		  }
		}
	      }
	    }
	  }
	  if (daemon->feedback&FB_MCP) {
	    // MCP timecode display
	    if (*daemon->timecode_h && screen_idx == 1 &&
		(daemon->feedback_items&FB_TIMECODE)) {
	      uint32_t col = ansi_col(7);
	      uint8_t r = (col&0xff0000)>>16,
		g = (col&0xff00)>>8, b = (col&0xff);
	      char time_display[14];
	      snprintf(time_display, 14, "%s %s %s %s",
		       daemon->timecode_h, daemon->timecode_m,
		       daemon->timecode_s, daemon->timecode_frames);
	      cairo_set_source_rgb(cr, r/255.f, g/255.f, b/255.f);
	      cairo_move_to(cr, 9*cw+5, 5*30);
	      cairo_show_text(cr, time_display);
	    }
	    // RSM status
	    if (daemon->feedback_items&FB_RSM) {
	      for (int i = 0; i < 4; i++) {
		int k = screen_idx*4+i;
		for (int j = 0; j < 3; j++) {
		  char *s = j==0?"R":j==1?"S":"M";
		  uint32_t col = ansi_col(daemon->rsm_status[j][k]?j+1:0);
		  uint8_t r = (col&0xff0000)>>16,
		    g = (col&0xff00)>>8, b = (col&0xff);
		  cairo_set_source_rgb(cr, r/255.f, g/255.f, b/255.f);
		  cairo_move_to(cr, (i*8+2*j)*cw+5, 30);
		  cairo_show_text(cr, s);
		}
	      }
	    }
	    // MCP scribble strips
	    if (daemon->feedback_items&FB_STRIPS) {
	      for (int i = 0; i < 4; i++) {
		int c = screen_idx*4;
		if (*daemon->scribble_text[c+i][0] ||
		    *daemon->scribble_text[c+i][1]) {
		  uint32_t col = daemon->scribble_col[c+i];
		  uint8_t r = (col&0xff0000)>>16,
		    g = (col&0xff00)>>8, b = (col&0xff);
		  cairo_set_source_rgb(cr, r/255.f, g/255.f, b/255.f);
		  cairo_move_to(cr, i*8*cw+5, 7*30);
		  cairo_show_text(cr, daemon->scribble_text[c+i][0]);
		  cairo_set_source_rgb(cr, r/255.f, g/255.f, b/255.f);
		  cairo_rectangle(cr, i*8*cw+5, 8*30-asc, 7*cw, ch);
		  cairo_fill(cr);
		  cairo_set_source_rgb(cr, 0, 0, 0);
		  cairo_move_to(cr, i*8*cw+5, 8*30);
		  cairo_show_text(cr, daemon->scribble_text[c+i][1]);
		}
	      }
	    }
	    // MCP meter display
	    if (daemon->feedback_items&FB_STRIPS) {
	      for (int i = 0; i < 4; i++) {
		int c = screen_idx*4;
		int val = daemon->meter[c+i];
		int map[16] = {0,1,1,2,2,3,3,4,4,5,5,6,7,8,8,8};
		for (int j = 0; j < 7; j++) {
		  int col = j>=map[val] ? ansi_colors[0] // black
		    : j>=6 ? ansi_colors[1] // red
		    : j>=4 ? ansi_colors[3] // yellow
		    : ansi_colors[2]; // green
		  uint8_t r = (col&0xff0000)>>16,
		    g = (col&0xff00)>>8, b = (col&0xff);
		  cairo_set_source_rgb(cr, r/255.f, g/255.f, b/255.f);
		  cairo_rectangle(cr, i*120+j*cw+1+5, 170, cw-2, 7);
		  cairo_fill(cr);
		}
	      }
	    }
	  }
	}
	if (daemon->screenie[screen_idx]) {
	  // take a screenshot, write to png file
	  char name[PATH_MAX];
	  snprintf(name, PATH_MAX, "screen%d.png", screen_idx);
	  cairo_surface_write_to_png(daemon->img, name);
	  daemon->screenie[screen_idx] = 0;
	}
	ctlra_screen_cairo_to_device(dev, screen_idx, pixel_data, bytes,
				     redraw_zone, daemon->img);

	return 1;
}

// AG XXXFIXME: Work around a bug with libusb_handle_events_timeout_completed
// taking forever if no device is online. You may want to enable this if the
// program becomes unresponsive (can't be interrupted and won't exit
// automatically with the -q option) as soon as the device count drops to
// zero. (This causes immediate exit from the program without properly
// finalizing ctlra, so only use this if you're running into this bug.)
//#define TEMP_USB_FIX 1

void sighndlr(int signal)
{
	done = 1;
	printf("\n");
#if TEMP_USB_FIX
	if (num_devs <= 0) exit(0);
#endif
}

void daemon_remove_func(struct ctlra_dev_t *dev, int unexpected_removal,
			void *userdata)
{
	struct daemon_t *daemon = userdata;
	ctlra_midi_destroy(daemon->midi);
	cairo_destroy(daemon->cr);
	cairo_surface_destroy(daemon->img);
	if (daemon->grid) free(daemon->grid);
	if (daemon->grid_col) free(daemon->grid_col);
	if (daemon->but) free(daemon->but);
	if (daemon->but_col) free(daemon->but_col);
	if (daemon->ctl) free(daemon->ctl);
	if (daemon->ctl_col) free(daemon->ctl_col);
	printf("daemon: removing %s %s\n",
	       daemon->info.vendor, daemon->info.device);
	fflush(0);
	free(daemon);
	if (--num_devs <= 0 && auto_exit) done = 1;
#if TEMP_USB_FIX
	if (num_devs <= 0 && auto_exit) exit(0);
#endif
}

static int devmatch(const struct ctlra_dev_info_t *info,
		    const char *pattern)
{
  char devname[1024];
  snprintf(devname, 1024, "%s %s", info->vendor, info->device);
  if (!fnmatch(pattern, devname, 0))
    return 1;
  snprintf(devname, 1024, "%04x:%04x", info->vendor_id, info->device_id);
  if (!fnmatch(pattern, devname, 0))
    return 1;
  return 0;
}

int accept_dev_func(struct ctlra_t *ctlra,
		    const struct ctlra_dev_info_t *info,
		    struct ctlra_dev_t *dev,
                    void *userdata)
{
	char *devname = userdata;
	if (max_devs && num_devs >= max_devs)
	  return 0;
	if (devname && *devname && !devmatch(info, devname))
	  return 0;

	printf("daemon: accepting %s %s (%04x:%04x)\n",
	       info->vendor, info->device,
	       info->vendor_id, info->device_id);
	struct daemon_t *daemon = calloc(1, sizeof(struct daemon_t));
	if(!daemon)
		goto fail;

	const char *prefix = "Ctlra ";
	char *buf = alloca(strlen(info->device)+strlen(prefix)+1);
	daemon->midi = ctlra_midi_open(strcat(strcpy(buf, prefix),
					      info->device),
				       daemon_input_cb,
				       daemon);

	daemon->info = *info;
	daemon->feedback = feedback;
	daemon->feedback_items = feedback_items;
	daemon->grid_size = info->control_count[CTLRA_EVENT_GRID] > 0 ?
	  info->grid_info[0].x * info->grid_info[0].y : 0;
	daemon->button_count = info->control_count[CTLRA_EVENT_BUTTON];
	daemon->slider_count = info->control_count[CTLRA_EVENT_SLIDER];
	daemon->encoder_count = info->control_count[CTLRA_EVENT_ENCODER];
	if(daemon->grid_size) {
		daemon->grid = calloc(daemon->grid_size, sizeof(uint8_t));
		daemon->grid_col = calloc(daemon->grid_size, sizeof(uint32_t));
		if (!daemon->grid || !daemon->grid_col)
			goto fail;
	}
	if(daemon->button_count) {
		daemon->but = calloc(daemon->button_count, sizeof(uint8_t));
		daemon->but_col = calloc(daemon->button_count, sizeof(uint32_t));
		if (!daemon->but || !daemon->but_col)
			goto fail;
	}
	if(daemon->encoder_count+daemon->slider_count) {
		int count = daemon->encoder_count+daemon->slider_count;
		daemon->ctl = calloc(count, sizeof(uint8_t));
		daemon->ctl_col = calloc(count, sizeof(uint32_t));
		if (!daemon->ctl || !daemon->ctl_col)
			goto fail;
	}

	if(!daemon->midi)
		goto fail;

	for (int i = 0; i < 8; i++) {
	  // AG: Currently there's no way to set these from outside, so pick
	  // some colorful defaults.
	  daemon->scribble_col[i] = ansi_colors[i?i:7];
	}

	// Default text to display.
	strcpy(daemon->screen_text[0][0], "Ctlra");
	strcpy(daemon->screen_text[0][1], "openavproductions.com");
	strncpy(daemon->screen_text[1][0], info->vendor, SCREEN_COLS);
	strncpy(daemon->screen_text[1][1], info->device, SCREEN_COLS);
	for (int i = 0; i < 2; i++) {
	  for (int j = 0; j < 2; j++) {
	    memset(&daemon->screen_fg[i][j][0], 7, SCREEN_COLS);
	  }
	}

	ctlra_dev_set_event_func(dev, daemon_event_func);
	ctlra_dev_set_feedback_func(dev, daemon_feedback_func);
	ctlra_dev_set_screen_feedback_func(dev, daemon_screen_redraw_func);
	ctlra_dev_set_remove_func(dev, daemon_remove_func);
	ctlra_dev_set_callback_userdata(dev, daemon);

	return 1;
fail:
	printf("alloc/open failure\n");
	if(daemon) {
		if (daemon->grid) free(daemon->grid);
		if (daemon->grid_col) free(daemon->grid_col);
		if (daemon->but) free(daemon->but);
		if (daemon->but_col) free(daemon->but_col);
		if (daemon->ctl) free(daemon->ctl);
		if (daemon->ctl_col) free(daemon->ctl_col);
		free(daemon);
	}

	return 0;
}

int main(int argc, char **argv)
{
  int opt;
  char *devname = 0;
  while ((opt = getopt(argc, argv, "d:f::i:hn:q")) != -1) {
    switch (opt) {
    case 'd':
      devname = optarg;
      break;
    case 'n':
      max_devs = atoi(optarg);
      break;
    case 'f':
      if (optarg && *optarg) {
	for (char *a = optarg; *a; a++)
	  switch (*a) {
	  case 'l': feedback |= FB_LOCAL; break;
	  case 'n': feedback |= FB_MIDI; break;
	  case 'm': feedback |= FB_MCP; break;
	  default:
	    fprintf(stderr, "%s: unrecognized option character '%c'. Try -h for help.\n", argv[0], *a);
	    exit(1);
	  }
      } else
	feedback = FB_ALL;
      break;
    case 'i':
      if (optarg && *optarg) {
	for (char *a = optarg; *a; a++)
	  switch (*a) {
	  case 'g': feedback_items &= ~FB_GRID; break;
	  case 'b': feedback_items &= ~FB_BUTTONS; break;
	  case 'c': feedback_items &= ~FB_CONTROLS; break;
	  case 'x': feedback_items &= ~FB_TEXT; break;
	  case 't': feedback_items &= ~FB_TIMECODE; break;
	  case 's': feedback_items &= ~FB_STRIPS; break;
	  case 'r': feedback_items &= ~FB_RSM; break;
	  default:
	    fprintf(stderr, "%s: unrecognized option character '%c'. Try -h for help.\n", argv[0], *a);
	    exit(1);
	  }
      }
      break;
    case 'q':
      auto_exit = 1;
      break;
    case 'h':
    default:
      printf("Usage: %s [-h] [-d name] [-f[opts]] [-i opts] [-n count] [-q]\n", argv[0]);
      printf("-h: print this message and exit\n");
      printf("-d: connect only to devices matching the given name\n");
      printf("    name can also be the vendor:device spec in hex, and may contain wildcards\n");
      printf("-f: enable feedback (l, n or m; default: all), options are:\n");
      printf("    l = local feedback on device (pushed buttons light up)\n");
      printf("    n = normal feedback via MIDI input (MIDI notes light up buttons)\n");
      printf("    m = MCP (Mackie) feedback via MIDI input (timecode, meterbridge, etc.)\n");
      printf("-i: disable individual feedback items, options are:\n");
      printf("    g = grid, b = buttons, c = other controls\n");
      printf("    x = text, t = timecode, s = scribble/meter strips, r = RSM indicators\n");
      printf("-n: maximum number of devices to open\n");
      printf("-q: quit automatically if no devices are present\n");
      exit(0);
    }
  }

  signal(SIGINT, sighndlr);

  struct ctlra_t *ctlra = ctlra_create(NULL);
  num_devs = ctlra_probe(ctlra, accept_dev_func, devname);
  if (num_devs <= 0 && auto_exit) {
    fprintf(stderr, "daemon: ERROR: couldn't open any devices, exiting\n");
    exit(1);
  }
  printf("daemon: connected devices: %d\n", num_devs);

  while(!done || screen_count > 0) {
    ctlra_idle_iter(ctlra);
    usleep(1000);
  }

  // AG: Another awful kludge here. The screens have already been cleared at
  // this point, but apparently we need to give it some extra time so that any
  // remaining data can be flushed. I found that if we don't do this, the
  // screens might not be properly reinitialized and aren't usable any more if
  // we rerun daemonx afterwards. At least that's what happens on my Maschine
  // Mk3, YMMV. XXXFIXME: This really should be handled in ctlra_exit() in
  // some way, maybe we need an extra screen finalization callback there?
  struct timespec now, then;
  int err = clock_gettime(CLOCK_MONOTONIC_RAW, &then);
  while(!err) {
    if ((err = clock_gettime(CLOCK_MONOTONIC_RAW, &now))) break;
    double secs = now.tv_sec - then.tv_sec;
    double nanos = now.tv_nsec - then.tv_nsec;
    if (secs*1e9 + nanos > 1e9) break;
    ctlra_idle_iter(ctlra);
    usleep(100000);
  }

  ctlra_exit(ctlra);

  return 0;
}
