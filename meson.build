project('openav_ctlra', 'c')
add_project_arguments('-std=gnu99', language : 'c')

conf_data = configuration_data()
conf_data.set('version', '0.1')

cc  = meson.get_compiler('c')

required_libs = ['libusb-1.0', 'alsa', 'm']

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
ctlra_lib_deps = []
ctlra_lib_deps += avtka_dep

m_dep = cc.find_library('m', required : false)
ctlra_lib_deps += m_dep

if get_option('firmata')
  firmatac = subproject('firmatac')
  firmata_dep = firmatac.get_variable('firmatac_dep')
  ctlra_lib_incs += include_directories('subprojects/firmatac/includes')
endif

if cc.has_header('libtcc.h')
  conf_data.set('HAVE_TCC', 1)
else
  conf_data.set('HAVE_TCC', 0)
endif

midi_dep = dependency('alsa', required: false)
if not midi_dep.found()
  midi_dep = disabler()
  message('MIDI = disabler')
endif

cairo_dep = dependency('cairo', required: false)
if(cairo_dep.found())
  #add_project_arguments('-DCTLRA_HAVE_CAIRO', language : 'c')
endif

subdir('ctlra')

install_headers(ctlra_hdr, subdir : 'ctlra')
ctlra_includes = include_directories('ctlra')

if (get_option('examples') != '')
	subdir('examples')
endif

# To copy files to the build directory
configure_file(input : 'examples/loopa/loopa_mk3.c',
    output : 'loopa_mk3.c',
    configuration : configuration_data())

# generate configuration file
configure_file(input : join_paths('ctlra', 'config.h.in'),
               output : 'config.h',
               configuration : conf_data)

pkg = import('pkgconfig')
pkg.generate(name: 'openav_ctlra',
              description: 'OpenAV Ctlra Library',
              version: conf_data.get('version'),
              requires: required_libs,
              subdirs : 'ctlra',
              libraries: ctlra)
