example_src = files('smpla.c', 'sequencer.c', 'audio.c', 'sampler.c', 'audio_samples.c')

example_src += files(join_paths('zix','ring.c'))

smpla_lv2_src = example_src + files('smpla_lv2.c')

example_src = example_src + files('jack_standalone.c')

thread_dep = dependency('threads')
dependencies += [m_dep, thread_dep, caira_dep]

# Smpla as LV2 plugin
# Needs to be built after Ctlra library is defined
# Don't link with JACK - this adds stdc++ if Jack2 is used
smpla_lv2 = library('smpla',
    [smpla_lv2_src],
    install : true,
    name_prefix : '', # backwards compatible with old build system
    install_dir : 'lib/lv2/smpla.lv2/',
    link_with : ctlra,
    dependencies: dependencies)

# Link the standalong JACK app with jack
dependencies += [jack_dep]

install_subdir('smpla.lv2', install_dir : 'lib/lv2/')

