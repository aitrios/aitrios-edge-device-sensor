senscord_rpicam_imx500_wrap = subproject('senscord-rpicam-imx500', required: false)

senscord_rpicam_imx500_src = meson.source_root() + '/subprojects/senscord-rpicam-imx500'
senscord_rpicam_imx500_build_dir = join_paths(meson.build_root(), 'senscord-rpicam-imx500_build')
senscord_rpicam_imx500_install_dir = get_option('prefix')
senscord_rpicam_imx500_package_name = 'senscord-edc-rpi'

build_senscord_rpicam_imx500 = custom_target(
  'build_senscord-rpicam-imx500',
  output: ['libcomponent_libcamera_image.so'],
  command: [
    'bash', '-lc',
    'set -ex && ' +
    'mkdir -p ' + senscord_rpicam_imx500_build_dir + ' && ' +
    'cd ' + senscord_rpicam_imx500_build_dir + ' && ' +
    'cmake ' + senscord_rpicam_imx500_src +
      ' -DCMAKE_INSTALL_PREFIX=' + senscord_rpicam_imx500_install_dir +
      ' -DCMAKE_PREFIX_PATH=' + senscord_rpicam_imx500_install_dir +
      ' && ' +
    'cmake --build . --parallel $(nproc) --verbose'
  ],
  capture: false,
  console: true,
  build_by_default: true
)

run_target('install_senscord-rpicam-imx500',
  command: [
	'../../script/install-senscord-rpicam-imx500.sh',
	'-b', senscord_rpicam_imx500_build_dir,
	'-s', senscord_rpicam_imx500_src,
	'-i', senscord_rpicam_imx500_install_dir
  ],
)

# A custom target to run the script packaging the output into a .deb file for
# installation on the Raspberry Pi.
run_target(
	'deb',
	command : [
		'../../script/mk-deb.sh',
		'-a', target_machine.cpu_family(),
		'-V', meson.project_version(),
		'-d', senscord_rpicam_imx500_install_dir,
		'-p', senscord_rpicam_imx500_package_name
	]
)
