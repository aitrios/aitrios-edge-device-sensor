if get_option('target')  == 'raspi'
  subdir('raspberry_pi')
endif

if get_option('target') == 't4r'
  subdir('t4r')
endif

edc_sensor_main_impl_includes_internal += include_directories('.')
senscord_sources_main += files([
	'sensor_main.c'
])
