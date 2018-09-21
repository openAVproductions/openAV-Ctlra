# Portable math library linking
m_dep = cc.find_library('m', required : false)

# Example specific deps - these *MUST* be required : false
jack_dep = dependency('jack', required: false)
sndfile_dep    = dependency('sndfile', required: false)
fluidsynth_dep = dependency('fluidsynth', required: false)


foreach name: get_option('examples').split(',')
  example_src = []
  dependencies = []
  c_args = []
  link_args = []
  install = false

  subdir(name)

  executable('ctlra_' + name,
             example_src,
             include_directories: ctlra_includes,
             dependencies : dependencies,
             install : install,
             c_args : c_args,
             link_args : link_args,
             link_with: ctlra)
endforeach


example_loopa = files('loopa/loopa.c', 'loopa/main.c')

# To properly install resources into /usr/share etc
#install_data(sources : 'loopa/loopa_mk3.c', install_dir :
#    join_paths(get_option('sharedstatedir'), '.'))

example_seq    = files('sequencer/seq.c', 'sequencer/sequencer.c')
example_vegas = files('vegas_mode/vegas_mode.c',
                       'vegas_mode/d2.c',
                       'vegas_mode/audio.c',
                       'vegas_mode/jam.c',
                       'vegas_mode/mm_mk2.c',
                       'vegas_mode/maschine3.c',
                       'vegas_mode/x1.c',
                       'vegas_mode/z1.c',
                       'vegas_mode/f1.c')
