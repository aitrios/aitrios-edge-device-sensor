
senscord_config_h.set('CONFIG_SENSOR_FW_UPDATE_LIB_MAX_AI_MODEL_COUNT', 4)

subdir('common')

if get_option('target')  == 'raspi'
  subdir('imx500')
  subdir('raspberry_pi')
endif

if get_option('target') == 'othercamera'
  subdir('imx500')
  subdir('othercamera')
endif

edc_sensor_fw_update_lib_includes_internal += include_directories('.')
edc_sensor_fw_update_lib_sources += files([
  'sensor_fw_update_lib.c',
])
