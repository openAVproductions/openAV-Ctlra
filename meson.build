project('openav_ctlra', 'c')
add_project_arguments('-std=c99', language : 'c')

conf_data = configuration_data()
conf_data.set('version', '0.1')

cc  = meson.get_compiler('c')

required_libs = ['libusb-1.0', 'alsa']

ctlra_lib_incs = []

avtka_dep = dependency('openav_avtka', required: false)
if(get_option('avtka') == true)
    if (not avtka_dep.found())
      avtka_git = subproject('avtka')
      avtka_dep = avtka_git.get_variable('avtka_dep')
    endif

    add_project_arguments('-DHAVE_AVTKA', language : 'c')
    required_libs += 'openav_avtka'
endif
ctlra_lib_deps = avtka_dep

if get_option('firmata')
  firmatac = subproject('firmatac')
  firmata_dep = firmatac.get_variable('firmatac_dep')
  ctlra_lib_incs += include_directories('subprojects/firmatac/includes')
endif

subdir('ctlra')

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

if(get_option('avtka') == true)
 if(avtka_dep.found())
  executable('virt_dev',
             example_virt_dev,
             include_directories: ctlra_includes,
             dependencies: avtka_dep,
             link_with: ctlra)

  executable('device_test',
             example_dev_test,
             include_directories: ctlra_includes,
             dependencies: avtka_dep,
             link_with: ctlra)
  endif
endif

cairo   = dependency('cairo', required: false)
sndfile = dependency('sndfile', required: false)
fluid   = dependency('fluidsynth', required: false)
if(jack.found() and cairo.found() and sndfile.found() and fluid.found())
executable('vegas_mode',
           example_vegas,
           include_directories: ctlra_includes,
           link_with: ctlra,
           dependencies: [jack, cairo, sndfile, fluid])
endif

pkg = import('pkgconfig')
pkg.generate(name: 'openav_ctlra',
              description: 'OpenAV Ctlra Library',
              version: conf_data.get('version'),
              requires: required_libs,
              subdirs : 'ctlra',
              libraries: ctlra)
