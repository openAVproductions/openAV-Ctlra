ctlra_hdr = files('ctlra.h', 'event.h')
ctlra_src = files('ctlra.c', 'usb.c', 'midi.c')

add_global_arguments('-Wno-unused-variable', language : 'c')

subdir('devices')

libusb = dependency('libusb-1.0')
alsa   = dependency('alsa')

ctlra = static_library('ctlra',
    [ctlra_src, devices_src],
    install : true,
    dependencies: [libusb, alsa])
