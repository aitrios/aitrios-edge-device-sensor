if meson.is_cross_build()
    sysroot_path = meson.get_cross_property('sys_root', '/')
else 
    sysroot_path = ''
endif

wasm_wrap = subproject('wasm-micro-runtime', required: false)
wamr_src = meson.source_root() + '/subprojects/wasm-micro-runtime'
wamr_prod_src = join_paths(wamr_src, 'product-mini', 'platforms', 'linux')
wamr_build_dir = join_paths(meson.build_root(), 'wamr_build')

# Determine if building for T4R
is_t4r_build = get_option('senscord_core_t4r_build') or get_option('senscord_component_t4r_build')

# External delegate options - only enabled for T4R
external_delegate_options = is_t4r_build ? \
  ' -DWAMR_BUILD_WASI_NN_ENABLE_EXTERNAL_DELEGATE=1' + \
  ' -DWAMR_BUILD_WASI_NN_EXTERNAL_DELEGATE_PATH="/usr/lib/libvx_delegate.so"' + \
  ' -DCMAKE_SYSROOT=' + sysroot_path  + '/' + \
  ' -Donnxruntime_INCLUDE_DIR=' + sysroot_path + '/usr/include/onnxruntime/core/session' + \
  ' -Donnxruntime_LIBRARY=' + sysroot_path + '/usr/lib/libonnxruntime.so' \
  : ''

message('external_delegate_options: @0@'.format(external_delegate_options))
message('================================')

# if cross compiling and onnxruntime is available in sysroot, link against it.
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
      external_delegate_options +
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
