example_src = files('main.c')
gtk3_dep = dependency('gtk+-3.0', required: false)
if not gtk3_dep.found()
  gtk3_dep = disabler()
endif

dependencies = gtk3_dep
