#!/bin/sh
#
# senscord-rpicam-imx500 install script
#
# Usage:
#   ./install_rpicam.sh -b BUILD_DIR -s SRC_DIR -i INSTALL_DIR
#

set -e

usage() {
  echo "Usage: $0 -b BUILD_DIR -s SRC_DIR -i INSTALL_DIR" >&2
  exit 1
}

# Parse options
while getopts "b:s:i:" opt; do
  case "$opt" in
    b) BUILD_DIR=$OPTARG ;;
    s) SRC_DIR=$OPTARG ;;
    i) INSTALL_DIR=$OPTARG ;;
    *) usage ;;
  esac
done

# Check required arguments
[ -z "$BUILD_DIR" ]   && { echo "Error: BUILD_DIR is required" >&2; usage; }
[ -z "$SRC_DIR" ]     && { echo "Error: SRC_DIR is required" >&2; usage; }
[ -z "$INSTALL_DIR" ] && { echo "Error: INSTALL_DIR is required" >&2; usage; }

# Change to build directory and run make install
cd "$BUILD_DIR"
make install

# Copy various files
if [ -d "$SRC_DIR/public/script" ]; then
  cp -r "$SRC_DIR"/public/script/* "$INSTALL_DIR"
fi

# ai_camera configuration files
mkdir -p "$INSTALL_DIR"/share/senscord
if [ -d "$SRC_DIR/public/assets/config/ai_camera" ]; then
  cp -r "$SRC_DIR/public/assets/config/ai_camera/"* \
        "$INSTALL_DIR/share/senscord"
else
  cp -r "$SRC_DIR"/public/unit_test/config/ai_camera/* \
     "$INSTALL_DIR"/share/senscord
fi

# Create directories for models and assets
mkdir -p "$INSTALL_DIR"/share/imx500-models
mkdir -p "$INSTALL_DIR"/share/rpi-camera-assets

# Copy models and assets
if [ -d "$SRC_DIR/public/assets/rpi-camera-assets" ]; then
  cp -r "$SRC_DIR/public/assets/rpi-camera-assets/"* \
        "$INSTALL_DIR/share/rpi-camera-assets"
else
  cp -r "$SRC_DIR"/public/unit_test/imx500-models/* \
        "$INSTALL_DIR"/share/imx500-models
  cp -r "$SRC_DIR"/public/unit_test/rpi-camera-assets/* \
        "$INSTALL_DIR"/share/rpi-camera-assets
fi

# Set permissions (continue even if it fails)
chmod -R 755 "$INSTALL_DIR"/bin/*    || true
chmod -R 755 "$INSTALL_DIR"/share/*  || true
# It should be 755, but right now, 777 is required for software control.
# The permissions should be changed after the software is changed.
chmod -R 777 "$INSTALL_DIR"/share/imx500-models || true
chmod -R 777 "$INSTALL_DIR"/share/rpi-camera-assets || true
chmod -R 755 "$INSTALL_DIR"/*.sh     || true

echo "Install Done:"
echo "  BUILD_DIR   = $BUILD_DIR"
echo "  SRC_DIR     = $SRC_DIR"
echo "  INSTALL_DIR = $INSTALL_DIR"
