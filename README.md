# Ctlra - A C Library for Controller Support

Ctlra is a plain C library that supports easily programming with
hardware controllers like USB HID devices. Recently controllers have
become much more powerful and feature-rich devices, resulting in tight
integration between software and hardware controllers. This places a
burden on software developers, who must support individual devices.

This library makes it easier to access these powerful hardware control
surfaces trough a simple API, which exposes buttons, encoders, sliders,
and a grid as components of the controller device. The details of the
USB HID protocol, and the implementation of the device communication are
abstracted.

This library may be of interest if you are writing music, video or other
media software. Or if you just want to hack controller support into your
web-browser because you can...

## Usage

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/676339a2937046f28fdcd5697c1259c1)](https://www.codacy.com/app/harryhaaren/openAV-Ctlra?utm_source=github.com&utm_medium=referral&utm_content=openAVproductions/openAV-Ctlra&utm_campaign=badger)
[![Build Status](https://travis-ci.org/openAVproductions/openAV-Ctlra.svg?branch=master)](https://travis-ci.org/openAVproductions/openAV-Ctlra)

Download the source-code, compile using [Meson](http://mesonbuild.com/) and [Ninja](https://ninja-build.org/)
(which must be available on the system.)
```
meson build
cd build
ninja
./examples/ctlra_simple
```

Your application can now statically link against this library. Providing
a shared-library and backwards ABI compatilbility to enable new devices
without recompilation of the application are long-term goals, which can be
discussed when the initial API has been reviewed and used in a few serious
applications.

### Midi daemon examples

It can send midi messaged, using supported controller.

In root directory:

```
meson build -D midi=true -D examples=simple,daemon --reconfigure || meson build -D midi=true -D examples=simple,daemon
cd build
ninja
./examples/ctlra_daemon
```

And connect your controller. You should see midi ports, for example in jack patchbay.

## Supported Devices

This library currently supports the following devices:

- 3DConnexion SpaceMouse Pro (Wireless)
- Native Instruments Maschine Mikro (Mk2 only)
- Native Instruments Traktor D2
- Native Instruments Traktor F1
- Native Instruments Traktor S2 (Mk2 only)
- Native Instruments Traktor X1 (Mk2 only)
- Native Instruments Traktor Z1

Prototyping for several other devices is in progress, but not complete.
These devices include:

- Native Instruments Maschine Jam
- Nintend WiiMote (Original)
- Generic MIDI
- Generic OSC
- Arduino Serial

## Device Manufacturers

If you are or represent a manufacturer of a device, and wish to have your
device supported by Ctlra, please contact OpenAV for information on how to
best upstream support for your device to Ctlra.

## Contact

Harry van Haaren <harryhaaren@gmail.com>
OpenAV Productions http://openavproductions.com

