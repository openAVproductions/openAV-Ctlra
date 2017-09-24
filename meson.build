project('openav_ctlra', 'c')
add_project_arguments('-std=c99', language : 'c')

conf_data = configuration_data()
conf_data.set('version', '0.1')

cc  = meson.get_compiler('c')

required_libs = ['libusb-1.0', 'alsa']

avtka   = dependency('openav_avtka', required: false)
if(avtka.found() and get_option('avtka') == true)
  add_global_arguments('-DHAVE_AVTKA', language : 'c')
  required_libs += 'openav_avtka'
endif

subdir('ctlra')

#['-Wl,--whole-archive'] + devices_lib + ['-Wl,--no-whole-archive']

pkg = import('pkgconfig')
pkg.generate(name: 'openav_ctlra',
              description: 'OpenAV Ctlra Library',
              version: conf_data.get('version'),
              requires: required_libs,
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

if(avtka.found())
executable('virt_dev',
           example_virt_dev,
           include_directories: ctlra_includes,
           dependencies: avtka,
           link_with: ctlra)

executable('device_test',
           example_dev_test,
           include_directories: ctlra_includes,
           dependencies: avtka,
           link_with: ctlra)
endif

cairo   = dependency('cairo', required: false)
sndfile = dependency('sndfile', required: false)
if(jack.found() and cairo.found() and sndfile.found())
executable('vegas_mode',
           example_vegas,
           include_directories: ctlra_includes,
           link_with: ctlra,
           dependencies: [jack, cairo, sndfile])
endif
