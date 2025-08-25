#!/bin/sh

# SPDX-FileCopyrightText: 2023-2024 Sony Semiconductor Solutions Corporation
#
# SPDX-License-Identifier: Apache-2.0

set -e

arch=amd64
version=0.0.0
package_name=senscord-package

usage()
{
    echo usage: mk-deb[-a arch][-V version][-p package_name] >&2
    exit 1
}

while test $# -gt 0; do
    case "$1" in
    -a)
        arch=${2?`usage`}
        shift 2
        ;;
    -V)
        version=${2?`usage`}
        shift 2
        ;;
    -d)
        buildroot=${2?`usage`}
        shift 2
        ;;
    -p)
        package_name=${2?`usage`}
        shift 2
        ;;
    *)
        usage
        ;;
    esac
done

cd "${MESON_SOURCE_ROOT}"

rm -rf "${buildroot}/dist"
trap "rm -rf ${buildroot}/dist $$.tmp" EXIT HUP INT TERM

case $arch in
aarch64)
    debarch=arm64
    ;;
x86_64)
    debarch=amd64
    ;;
esac

# create deb package
mkdir -p "${buildroot}/dist/DEBIAN"

cat > "${buildroot}/dist/DEBIAN/control" <<EOF
Package: $package_name
Section: contrib/misc
Version: $version
Priority: optional
Architecture: $debarch
Maintainer: Yushi Oka <Yushi.Oka@sony.com>
Depends: libcamera0.5
Description: AITRIOS Edge Device Core
 This package provides the Sony AITRIOS Edge Device Core
EOF

mkdir -p "${buildroot}/dist/opt/senscord/bin"
mkdir -p "${buildroot}/dist/opt/senscord/lib"
mkdir -p "${buildroot}/dist/opt/senscord/python"
mkdir -p "${buildroot}/dist/opt/senscord/share"
mkdir -p "${buildroot}/dist/opt/senscord/include"

cp -a "${buildroot}/bin/."    "${buildroot}/dist/opt/senscord/bin/"
cp -a "${buildroot}/lib/."    "${buildroot}/dist/opt/senscord/lib/"
cp -a "${buildroot}/python/." "${buildroot}/dist/opt/senscord/python/"
cp -a "${buildroot}/share/."  "${buildroot}/dist/opt/senscord/share/"
cp -a "${buildroot}/include/."  "${buildroot}/dist/opt/senscord/include/"
cp -a "/usr/local/include/." "${buildroot}/dist/opt/senscord/include/"
cp -a "/usr/local/lib/." "${buildroot}/dist/opt/senscord/lib/"

dpkg-deb --build "${buildroot}/dist" "${buildroot}/${package_name}-${version}_$debarch.deb"
