ctlra_hdr = files('ctlra.h', 'event.h')
ctlra_src = files('ctlra.c', 'event.c', 'usb.c')

jack   = dependency('jack', required: false)
conf_data.set('jack', jack.found())

subdir('devices')

cargs = ['-Wno-unused-variable']

libusb = dependency('libusb-1.0')
gl     = dependency('gl', required: false)

conf_data.set('libusb', libusb.found())
conf_data.set('alsa', midi_dep.found())
conf_data.set('cairo', cairo_dep.found())

ctlra_lib_deps_impl = [libusb, gl]

if avtka_dep.found()
  ctlra_lib_deps_impl += avtka_dep
endif

if get_option('firmata')
  ctlra_lib_deps_impl += firmata_dep
endif

if midi_dep.found()
  ctlra_lib_deps_impl += midi_dep
  ctlra_src += files('midi.c')
endif

devices_lib = static_library('ctlra_devices', devices_src,
    c_args: cargs,
    install : false,
    include_directories : ctlra_lib_incs,
    link_args : '-Wl,--whole-archive',
    dependencies: ctlra_lib_deps_impl)

ctlra = library('ctlra',
    [ctlra_src],
    c_args: cargs,
    install : true,
    link_whole : devices_lib,
    dependencies: ctlra_lib_deps_impl)

configure_file(input : 'config.h.in',
               output : 'config.h',
               configuration : conf_data)
