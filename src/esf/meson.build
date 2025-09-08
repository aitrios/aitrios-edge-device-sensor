if meson.is_cross_build()
  sysroot_path = meson.get_cross_property('sys_root', '')
  senscord_includes_public = [ include_directories(sysroot_path + '/opt/senscord/include') ]
else
  senscord_includes_public = [ include_directories('/opt/senscord/include') ]
endif
senscord_includes_internal = []
senscord_sources = files([])

subdir('sensor_ai_lib')
subdir('sensor_fw_update_lib')
subdir('senscord_component_inference')
subdir('sensor_main')

configure_file(output : 'senscord_config.h',
	configuration : senscord_config_h,
)

senscord_arguments = [
	'-include', meson.current_build_dir() / 'senscord_config.h'
]

senscord_main = static_library(
	'senscord_main',
	senscord_sources_main,
	include_directories : [
		esf_includes_public,
        wasm_iwasm_inc,
		senscord_includes_internal,
		senscord_includes_public,
		sensor_ai_lib_includes_internal,
		sensor_ai_lib_includes_public,
		senscord_component_inference_includes_internal,
		senscord_component_inference_includes_public,
		utility_includes_public
	],
	dependencies : [
		sensor_ai_lib_dep
	],
	cpp_args : senscord_arguments,
	c_args : senscord_arguments
)

senscord_dep = declare_dependency(
	link_with : senscord_main,
	include_directories : [
		senscord_includes_public,
		sensor_ai_lib_includes_public,
		senscord_component_inference_includes_public
	]
)
