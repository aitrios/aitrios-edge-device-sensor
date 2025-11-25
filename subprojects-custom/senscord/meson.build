senscord_wrap = subproject('senscord', required: false)
senscord_src = meson.source_root() + '/subprojects/senscord'
senscord_build_dir = join_paths(meson.build_root(), 'senscord_build')
senscord_install_dir = get_option('prefix')
wamr_src = meson.source_root() + '/subprojects/wasm-micro-runtime'
wamr_link_target = wamr_src
wamr_link_target_build = join_paths(wamr_link_target, 'build')

build_senscord = custom_target(
  'build_senscord',
  output: ['libsenscord.so'],
  command: [
    'bash', '-lc',
    'set -ex && ' +
    'mkdir -p ' + senscord_build_dir + ' && ' +
    'cd ' + senscord_build_dir + ' && ' +
    'cmake ' + senscord_src +
      ' -DCMAKE_INSTALL_PREFIX=' + senscord_install_dir +
      ' -DCMAKE_PREFIX_PATH=' + senscord_install_dir +
      ' -DSENSCORD_API_WASM=ON' +
      ' -DSENSCORD_LOG_ENABLED=ON' +
      ' -DSENSCORD_API_PYTHON=ON' +
      ' -DSENSCORD_SERVER_WEBSOCKET=ON' +
      ' -DSENSCORD_AITRIOS_SPECIFIC_ENABLED=ON' +
      ' -DWAMR_ROOT_DIR=' + wamr_link_target +
      ' -DWAMR_IWASM_LIBRARY=' + wamr_link_target_build + '/libiwasm.so' +
      ' && ' +
    'cmake --build . --parallel $(nproc) --verbose'
  ],
  depends: [ build_wamr ],
  capture: false,
  console: true,
  build_by_default: true
)

run_target('install_senscord',
  command: [
    'bash', '-c',
    'cd ' + senscord_build_dir + ' && make install'
  ],
  depends: build_senscord
)
