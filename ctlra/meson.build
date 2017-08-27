ctlra_hdr = files('ctlra.h', 'event.h')
ctlra_src = files('ctlra.c', 'event.c', 'usb.c', 'midi.c')

jack   = dependency('jack', required: false)
conf_data.set('jack', jack.found())

subdir('devices')

libusb = dependency('libusb-1.0')
conf_data.set('libusb', libusb.found())
alsa   = dependency('alsa', required: false)
conf_data.set('alsa', alsa.found())

ctlra = static_library('ctlra',
    [ctlra_src, devices_src],
    c_args: '-Wno-unused-variable',
    install : true,
    dependencies: [libusb, alsa, avtka])

configure_file(input : 'config.h.in',
               output : 'config.h',
               configuration : conf_data)
