# daemonx (extended daemon example)

This is an attempt to rework the ctlra daemon example so that it becomes more useful. It also serves as a showcase utilizing most ctlra features, and as a hacking playground to explore the graphical capabilities of ctlra.

In particular, this version of the daemon adds the capability to output text on the screen, improves the local feedback, and also adds optional MIDI and MCP (Mackie control protocol) feedback, including support for scribble strips, timecode and meter displays. For good measure, it also throws in some external configuration capabilities and a screenshot utility via custom sysex messages. It also adds a Ctlra prefix to the ALSA client name, so that it can be distinguished more easily from the dummy MIDI device created by ALSA itself.

Even if you don't utilize the MCP feedback and text output capabilities, daemonx offers a number of advantages over the original daemon program, most notably the options which enable you to select target devices and configure local and MIDI feedback to your liking.

**NOTE:** This hasn't been tested with anything but the Maschine Mk3 yet. The basic functionality including local and MIDI feedback should hopefully be portable across all devices supported by ctlra, but the screen output facilities most likely need some (or a lot of) work, depending on the particular device. Also, like the original daemon application, daemonx lacks integrated mappa support right now, so you will have to rely on external utilities to do any needed mapping of the controls on your device.

## Usage

ctlra_daemonx [-h] [-d *name*] [-f[*opts*]] [-i *opts*] [-n *count*] [-q]

## Options

-h
:   Print a short help message and exit.

-d *name*
:   Connect only to devices matching the given name; *name* can also be the vendor:device spec in hex, and may contain shell wildcards.

-f[*opts*]
:   Enable feedback (l, n or m; default: all), the available options are: l = local feedback on device (pushed buttons light up); n = normal feedback via MIDI input (MIDI notes light up buttons); m = MCP (Mackie) feedback via MIDI input (timecode, meterbridge, etc.)

-i *opts*
:   Disable individual feedback items, the available options are: g = grid, b = buttons, c = other controls, x = text, t = timecode, s = scribble/meter strips, r = RSM indicators

-n *count*
:   Set the maximum number of devices to open. This is often used in conjunction with `-d`. E.g., `-d 'Maschine*' -n1` will open the first available Maschine device only.

-q
:   Quit automatically if no devices are present, or if the device count drops to zero during operation (usually because devices are disconnected at run time).

## MIDI Bindings

Please check the extensive comments in daemonx.c for all the gory details. Basically, the encoders and sliders are mapped to control change messages on MIDI channel 1, the 4x4 grid to note messages on MIDI channel 10 (starting at note 36), and the remaining buttons to note messages on MIDI channel 1 (starting at note 24; notes 0 to 23 are reserved for MCP feedback, see below). If the corresponding feedback option is enabled (`-fn` on the command line), sending the same messages to the device will light the corresponding LEDs (if available). Feedback can also be handled locally (lighting a button when it is pressed), when using the `-fl` (local feedback) option.

daemonx recognizes the usual Mackie control messages for timecode (CC64 onward), meter display (channel pressure), rec/solo/mute indicators (notes 0 to 23), and the sysex message for the scribble strips. If the corresponding feedback option is enabled (`-fm` on the command line), these items will be displayed on the screen of the device if available.

The feedback options can be combined as needed; `-flmn` or just `-f` enables them all. However, combining local and MIDI feedback isn't recommended as it may cause clashes between the local feedback and MIDI input, so usually you'll either use `-fl`, `-fn`, or no feedback at all. You may also want to try `-fm` if you got a Maschine Mk3 or a sufficiently similar device, but if the screen looks garbled, it's better to leave that option disabled. Local and plain MIDI feedback should work on any device supported by ctlra, though. (If pressing a button or sending MIDI seems to light the wrong button, there's probably an issue with the device backend which should be reported.)

daemonx also recognizes its own sysex message format, employing the 0x7D manufacturer id reserved for educational use. These are used for introspection purposes, for setting the feedback options dynamically per device, and for rendering arbitrary text on the screen. The following types of sysex messages are available in the present implementation:

(1) Universal sysex, non-realtime:

F0 7E 7F 06 01 F7: identity request

The reply takes the form:

F0 7E 7F 06 02 7D 0*v* 0*v* 0*v* 0*v* 0*d* 0*d* 0*d* 0*d* F7,

where *v v v v* denotes the four hex digits of the vendor (most significant digit first), and *d d d d* the four hex digits of the device id. These are as reported by the ctlra library; usually this will be the usb vendor/device id, but may also be zero for devices not accessed through the usb library.

(2) Custom (ctlra_daemonx-specific):

F0 7D 00 *ss* *xx* *yy* *fg* *bg* *c1* *c2* ... F7: print text to screen

Outputs the given characters *c1* *c2* ... (7 bit ASCII) at position *xx*, *yy* (*xx* = column in the range 0..30, *yy* = row in the range 0..5) on screen *ss* (which must be 0 or 1 in the current implementation). *fg* and *bg* specify the foreground and background colors as indices 0..7 into the ANSI color palette (0 = black, 1 = red, 2 = green, 3 = yellow, 4 = blue, 5 = magenta, 6 = cyan, 7 = white). The first null byte in the sequence *c1* *c2* ... causes the rest of the line to be cleared.

F0 7D 01 *ss* F7: clear screen

Clear screen *ss* (0 or 1). *ss* can also be omitted, in which case both screens are cleared.

F0 7D 02 *ss* F7: take a screenshot

Capture the contents of screen *ss* (0 or 1). *ss* can also be omitted, in which case both screens are captured. The output is written to a png file (named screen0.png for screen 0, and screen1.png for screen 1) in ctlra_daemonx's working directory.

F0 7D 40 *vv* *mm* F7: set feedback options

*vv* must be 0 (off) or 1 (on), and *mm* denotes the bitmask of feedback options (bits 0..3, corresponding to local, MIDI, and MCP feedback) to be turned off and on, respectively. These options are all disabled by default, but may be changed from the command line.

F0 7D 41 *vv* *mm* F7: set feedback items

*vv* must be 0 (off) or 1 (on), and *mm* denotes the bitmask of individual feedback items (bits 0..6, corresponding to grid, buttons, other controls, text, timecode, scribble/meter strips, and rec/solo/mute indicators) to be turned off and on, respectively. These options are all enabled by default, but may be changed from the command line.

F0 7D 42 01 F7: layout request

Reports the control layout in a sysex message of the following form: F0 7D 42 02 *nn* *mm* *pp* *qq* F7. The numbers *n*, *m*, *p* and *q* denote the numbers of encoders, sliders, grid buttons, and other buttons, respectively.

## Mackie Emulation

The MCP support in daemonx is bare-bones right now; it only covers the most essential MCP feedback messages. To get a full Mackie emulation, you will still have to map the buttons, sliders and encoders of your device to MCP and vice versa. The plan is to support this directly in daemonx using ctlra's mapping component mappa, but this hasn't been implemented yet.

However, for the time being you can already enjoy a usable Mackie emulation for the Maschine Mk3 by using daemonx together with [midizap][]. To do this, run midizap with the Maschine.midizaprc configuration included in the midizap source (see the [Maschine.midizaprc][] file for instructions).

[midizap]: https://github.com/agraef/midizap
[Maschine.midizaprc]: https://github.com/agraef/midizap/blob/master/examples/Maschine.midizaprc

## Bugs

Please report any bugs that you find. I've noticed the following snags on my Arch-based system running the latest libusb (YMMV).

- There seems to be a bug in libusb which makes ctlra hang when the device count drops to zero. (This isn't specific to the daemonx program, all examples included in the ctlra source exhibit this behavior on my system.) daemonx contains some code to work around this issue so that you can at least still interrupt the program with Ctrl+C when this happens. To enable that work-around, you need to configure ctlra from the toplevel source directory as follows: `meson build -Dusbfix=true`

- As exhibited by the Maschine Mk3 Mackie emulation mentioned above, the rendering of the graphical display on the device lags quite noticeably. I've been looking into this and I'm confident that the delay is neither due to daemonx nor the ctlra library. I don't see any plausible causes for delay in the Mk3 backend either, so I guess that it's another libusb-related issue or maybe the hardware itself. No work-around for this issue is known at this time.
