#!/bin/bash

set -e

VERSION="1.2-1"
BS="bambooslacking"
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
BUILD="${DIR}/../build"
PKG="${BUILD}/${BS}_${VERSION}"
BUILD_TYPE="release"

cp -r "${DIR}/${BS}" "$PKG"
mkdir -p "${PKG}/usr/local/bin"
cp "${BUILD}/${BUILD_TYPE}/${BS}" "${PKG}/usr/local/bin/${BS}"
chmod a+x "${PKG}/etc/init.d/${BS}" "$PKG/usr/local/bin/${BS}" "${PKG}/DEBIAN/postinst" "${PKG}/DEBIAN/postrm"

ARCH=$(uname -m)
if [ "${ARCH}" == "aarch64" ]; then
  ARCH="arm64"
else
  ARCH="amd64"
fi

sed -i "s/Version:.*/Version: ${VERSION}/" "${PKG}/DEBIAN/control"
sed -i "s/Architecture:.*/Architecture: ${ARCH}/" "${PKG}/DEBIAN/control"

dpkg-deb --build "${PKG}"
