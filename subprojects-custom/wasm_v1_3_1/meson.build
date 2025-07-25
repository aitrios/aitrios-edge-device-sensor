wasm_v1_3_1_wrap = subproject('wasm-micro-runtime_v1_3_1', required: false)
wamr_v1_3_1_src = meson.source_root() + '/subprojects/wasm-micro-runtime_v1_3_1'
wamr_v1_3_1_prod_src = join_paths(wamr_v1_3_1_src, 'product-mini', 'platforms', 'linux')
wamr_v1_3_1_build_dir = join_paths(meson.build_root(), 'wamr_v1_3_1_build')
build_wamr_v1_3_1 = custom_target(
  'build_wamr_v1_3_1',
  output: ['libiwasm.so','iwasm'],
  command: [
    'bash', '-lc',
    'set -ex && ' +
    'mkdir -p ' + wamr_v1_3_1_build_dir + ' && ' +
    'cd ' + wamr_v1_3_1_build_dir + ' && ' +
    'cmake ' + wamr_v1_3_1_prod_src + ' ' +
      ' -DCMAKE_INSTALL_PREFIX=' + get_option('prefix') +
      ' -DWAMR_BUILD_LIB_WASI_THREADS=1' +
      ' -DCMAKE_VERBOSE_MAKEFILE=ON' +
      ' -DWAMR_BUILD_DEBUG_INTERP=1' +
      ' -DWAMR_BUILD_FAST_INTERP=0' +
      ' -DCMAKE_C_FLAGS="-DAPP_THREAD_STACK_SIZE_DEFAULT=10485760 -DAPP_THREAD_STACK_SIZE_MIN=10485760"' + ' && ' +
    'cmake --build . --parallel $(nproc) --verbose'
  ],
  capture: false,
  console: true,
  build_by_default: true
)

run_target('install_wamr',
  command: [
    'bash', '-c',
    'cd ' + wamr_v1_3_1_build_dir + ' && make install'
  ],
  depends: build_wamr_v1_3_1
)
