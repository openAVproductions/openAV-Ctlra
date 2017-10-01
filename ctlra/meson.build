ctlra_hdr = files('ctlra.h', 'event.h')
ctlra_src = files('ctlra.c', 'event.c', 'usb.c', 'midi.c')

jack   = dependency('jack', required: false)
conf_data.set('jack', jack.found())

subdir('devices')

cargs = ['-Wno-unused-variable']

libusb = dependency('libusb-1.0')
conf_data.set('libusb', libusb.found())
alsa   = dependency('alsa', required: false)
conf_data.set('alsa', alsa.found())
gl     = dependency('gl', required: false)

devices_lib = static_library('ctlra_devices', devices_src,
    c_args: cargs,
    install : false,
    include_directories : firmatac_includes,
    link_args : '-Wl,--whole-archive',
    dependencies: [libusb, alsa, avtka_dep, gl, firmatac_dep])

ctlra = library('ctlra',
    [ctlra_src],
    c_args: cargs,
    install : true,
    link_whole : devices_lib,
    dependencies: [libusb, alsa, avtka_dep, gl])

configure_file(input : 'config.h.in',
               output : 'config.h',
               configuration : conf_data)
