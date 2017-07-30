project('openav_ctlra', 'c')

cc  = meson.get_compiler('c')

subdir('ctlra')

pkg = import('pkgconfig')
pkg.generate(name: 'openav_ctlra',
              description: 'OpenAV Ctlra Library',
              version: '0.0.1',
              requires: ['libusb-1.0', 'alsa'],
              libraries: ctlra)

install_headers(ctlra_hdr, subdir : 'ctlra')
