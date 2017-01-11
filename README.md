Ctlra - A C Library for Controller Support
==========================================

LibCtlra is a plain C library that supports easily programming with
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

Usage
-----

Download the source-code, compile using cmake:
```
mkdir build
cd build
cmake ..
make
./examples/simple
./examples/daemon
```

Your application can now statically link against this library.

Supported Devices
-----------------

This library currently supports the following devices:

- Native Instruments Traktor D2
- Native Instruments Traktor F1
- Native Instruments Traktor X1 (Mk2)
- Native Instruments Traktor Z1
- Native Instruments Maschine Mikro (Mk2)

Contact
-------

Harry van Haaren <harryhaaren@gmail.com>
OpenAV Productions http://openavproductions.com

