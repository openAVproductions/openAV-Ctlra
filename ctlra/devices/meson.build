devices_src = files('3dconnexion.c',
                    'ni_kontrol_d2.c',
                    'ni_kontrol_f1.c',
                    'ni_kontrol_s2_mk2.c',
                    'ni_kontrol_s5.c',
                    'ni_kontrol_x1_mk2.c',
                    'ni_kontrol_z1.c',
                    'ni_maschine_jam.c',
                    'ni_maschine_mk3.c',
                    'ni_maschine_mikro_mk2.c')

if get_option('midi')
  devices_src += files('midi_generic.c')
endif
if (get_option('firmata') == true)
  devices_src += files('firmata.c')
endif
if (get_option('avtka') == true)
  devices_src += files('avtka.c')
endif
