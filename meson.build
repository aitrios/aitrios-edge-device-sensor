project('edc-sensor',
        'c', 'cpp',
        version: '0.1.8')

cmake = import('cmake')

if get_option('senscord_core_build')

subdir('subprojects-custom/wasm_v1_3_1')
subdir('subprojects-custom/senscord')

elif get_option('senscord_component_build')

subdir('subprojects-custom/senscord-rpicam-imx500')

else

# not supported

endif
