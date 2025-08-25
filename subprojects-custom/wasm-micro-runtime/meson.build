wasm_wrap = subproject('wasm-micro-runtime', required: false)
wamr_src = meson.source_root() + '/subprojects/wasm-micro-runtime'
wamr_prod_src = join_paths(wamr_src, 'product-mini', 'platforms', 'linux')
wamr_build_dir = join_paths(meson.build_root(), 'wamr_build')
build_wamr = custom_target(
  'build_wamr',
  output: ['libiwasm.so','iwasm', '**.so.*'],
  command: [
    'bash', '-lc',
    'set -ex && ' +
    'mkdir -p ' + wamr_build_dir + ' && ' +
    'cd ' + wamr_build_dir + ' && ' +
    'cmake ' + wamr_prod_src + ' ' +
      ' -DCMAKE_INSTALL_PREFIX=' + get_option('prefix') +
      ' -DWAMR_BUILD_LIB_WASI_THREADS=1' +
      ' -DCMAKE_VERBOSE_MAKEFILE=ON' +
      ' -DWAMR_BUILD_DEBUG_INTERP=1' +
      ' -DWAMR_BUILD_FAST_INTERP=0' +
      ' -DWAMR_BUILD_WASI_NN=1' +
      ' -DWAMR_BUILD_WASI_NN_TFLITE=1' +
      ' -DWAMR_BUILD_WASI_NN_ONNXRUNTIME=1' +
      ' -DWAMR_BUILD_WASI_NN_ENABLE_EXT=1' +
      ' -DWASI_NN_EXT_DELEGATE_PATH="/usr/lib/libvx_delegate.so"' +
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
    'cd ' + wamr_build_dir + ' && make install'
  ],
  depends: build_wamr
)
