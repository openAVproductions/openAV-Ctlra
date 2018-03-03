project('openav_ctlra', 'c')
add_project_arguments('-std=gnu99', language : 'c')

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

# To copy files to the build directory
configure_file(input : 'examples/loopa/loopa_mk3.c',
    output : 'loopa_mk3.c',
    configuration : configuration_data())

executable('simple',
           example_simple,
           include_directories: ctlra_includes,
           link_with: ctlra)

executable('avtka',
           example_avtka,
           include_directories: ctlra_includes,
           link_with: ctlra,
           dependencies: avtka_dep)

if get_option('midi')
  executable('daemon',
             example_daemon,
             include_directories: ctlra_includes,
             link_with: ctlra)

  executable('sequencer',
             example_seq,
             include_directories: ctlra_includes,
             link_with: ctlra)
endif

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
if(jack.found() and cairo.found() and sndfile.found() and not fluid.found())
executable('vegas_mode',
           example_vegas,
           include_directories: ctlra_includes,
           link_with: ctlra,
           dependencies: [jack, cairo, sndfile, fluid])
endif

# FAUST code requires powf()
m_dep = cc.find_library('m', required : false)

executable('loopa',
           example_loopa,
           include_directories: ctlra_includes,
           link_with: ctlra,
           link_args : ['-ltcc', '-ldl'],
           dependencies: [jack, cairo, sndfile, fluid, m_dep])

gtkdep = dependency('gtk+-3.0', required: false)
if gtkdep.found()
  executable('select_device',
      'examples/select_device/main.c',
      link_with : ctlra,
      dependencies : gtkdep)
endif

pkg = import('pkgconfig')
pkg.generate(name: 'openav_ctlra',
              description: 'OpenAV Ctlra Library',
              version: conf_data.get('version'),
              requires: required_libs,
              subdirs : 'ctlra',
              libraries: ctlra)
