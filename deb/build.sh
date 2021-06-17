#!/bin/bash

set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
VERSION="1.1-1"

# it depends on your build settings
mkdir -p $DIR/bambooslacking_$VERSION/usr/local/bin
cp $DIR/../build/release/bambooslacking $DIR/bambooslacking_$VERSION/usr/local/bin/bambooslacking

chmod a+x $DIR/bambooslacking_$VERSION/etc/init.d/bambooslacking \
$DIR/bambooslacking_$VERSION/usr/local/bin/bambooslacking \
$DIR/bambooslacking_$VERSION/DEBIAN/postinst \
$DIR/bambooslacking_$VERSION/DEBIAN/postrm

dpkg-deb --build $DIR/bambooslacking_$VERSION

# port binary files back to repo in case vagrant deployment is being used
VAGRANT_DIR="/deb/bambooslacking_$VERSION"
if [ -d "$VAGRANT_DIR" ]; then
cp $DIR/../build/release/bambooslacking $VAGRANT_DIR/usr/local/bin/
cp $DIR/bambooslacking_$VERSION.deb /deb/
fi