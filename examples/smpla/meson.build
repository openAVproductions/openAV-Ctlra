example_src = files('smpla.c', 'sequencer.c', 'audio.c', 'sampler.c', 'audio_samples.c')

example_src += files(join_paths('zix','ring.c'))

thread_dep = dependency('threads')
dependencies += [m_dep, jack_dep, thread_dep]
