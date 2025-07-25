sensor_ai_lib_includes_public = include_directories('include')
sensor_ai_lib_includes_internal = include_directories('.')
sensor_ai_lib_sources = files([])

subdir('src')

sensor_ai_lib = static_library(
	'sensor_ai_lib',
	sensor_ai_lib_sources,
	include_directories : [
		sensor_ai_lib_includes_internal,
		sensor_ai_lib_includes_public,
	]
)

sensor_ai_lib_dep = declare_dependency(
	link_with : sensor_ai_lib,
	include_directories : sensor_ai_lib_includes_public
)
