project('openav_ctlra', 'c')

conf_data = configuration_data()
conf_data.set('version', '0.1')

cc  = meson.get_compiler('c')

subdir('ctlra')

pkg = import('pkgconfig')
pkg.generate(name: 'openav_ctlra',
              description: 'OpenAV Ctlra Library',
              version: conf_data.get('version'),
              requires: ['libusb-1.0', 'alsa'],
              libraries: ctlra)

install_headers(ctlra_hdr, subdir : 'ctlra')

subdir('examples')
ctlra_includes = include_directories('ctlra')

executable('simple',
           example_simple,
           include_directories: ctlra_includes,
           link_with: ctlra)

executable('daemon',
           example_daemon,
           include_directories: ctlra_includes,
           link_with: ctlra)

executable('sequencer',
           example_seq,
           include_directories: ctlra_includes,
           link_with: ctlra)

cairo   = dependency('cairo', required: false)
sndfile = dependency('sndfile', required: false)
if(jack.found() and cairo.found() and sndfile.found())
executable('vegas_mode',
           example_vegas,
           include_directories: ctlra_includes,
           link_with: ctlra,
           dependencies: [jack, cairo, sndfile])
endif
