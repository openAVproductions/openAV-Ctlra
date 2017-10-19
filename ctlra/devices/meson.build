devices_src = files('3dconnexion.c',
                    'firmata.c',
                    'ni_kontrol_f1.c',
                    'ni_kontrol_d2.c',
                    'ni_kontrol_x1_mk2.c',
                    'ni_kontrol_s2_mk2.c',
                    'ni_kontrol_z1.c',
                    'ni_maschine_jam.c',
                    'ni_maschine_mikro_mk2.c')

if (get_option('avtka') == true)
  devices_src += files('avtka.c')
endif
