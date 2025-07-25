
edc_sensor_fw_update_lib_includes_public = []
edc_sensor_fw_update_lib_includes_internal = []
edc_sensor_fw_update_lib_impl_includes_internal = []
edc_sensor_fw_update_lib_sources = files([])
edc_sensor_fw_update_lib_impl_sources = files([])
edc_sensor_fw_update_lib_includes_public += include_directories('include')

subdir('src')
senscord_includes_public += edc_sensor_fw_update_lib_includes_public
senscord_includes_internal += edc_sensor_fw_update_lib_includes_internal
senscord_includes_internal += edc_sensor_fw_update_lib_impl_includes_internal
senscord_sources_main += edc_sensor_fw_update_lib_sources
senscord_sources_main += edc_sensor_fw_update_lib_impl_sources