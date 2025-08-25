# Build

## Environment (Recommended)
 - platform: linux/arm64 (including raspberry Pi)
 - devcontainer
  - see [this](../.devcontainer/README.md)

## Clean Files

```
rm -rf builddir*
rm -rf subprojects/senscord
rm -rf subprojects/senscord-rpicam-imx500
rm -rf subprojects/wasm-micro-runtime
```

## Build and Install SensCord Core

**Note: Build directory name must be "builddir"**

```
meson setup -Dsenscord_core_build=true -Dsenscord_component_build=false --prefix=${HOME}/senscord_output builddir
ninja -v -C builddir
ninja -v -C builddir install_senscord
ninja -v -C builddir install_wamr
```

## Build and Install SensCord Component

**Note: Build SensCord Core is required to build SensCord Component and prefix must be the same as SensCord Core's**

```
meson setup -Dsenscord_core_build=false -Dsenscord_component_build=true --prefix=${HOME}/senscord_output builddir_component
ninja -v -C builddir_component
ninja -v -C builddir_component install_senscord-rpicam-imx500
```

```
ninja -v -C builddir_component deb
```

