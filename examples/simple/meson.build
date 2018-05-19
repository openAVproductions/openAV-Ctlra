example_src = files('simple.c')
if cairo_dep.found()
  dependencies += cairo_dep
else
  example_src += files('sera.c')
endif
