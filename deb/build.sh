#!/bin/bash

set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# it depends on your build settings
cp $DIR/../build/release/bambooslacking $DIR/bambooslacking_1.0-1/usr/local/bin/bambooslacking

chmod a+x $DIR/bambooslacking_1.0-1/etc/init.d/bambooslacking \
$DIR/bambooslacking_1.0-1/usr/local/bin/bambooslacking \
$DIR/bambooslacking_1.0-1/DEBIAN/postinst \
$DIR/bambooslacking_1.0-1/DEBIAN/postrm

dpkg-deb --build $DIR/bambooslacking_1.0-1

# port binary files back to repo
cp $DIR/../build/release/bambooslacking /deb/bambooslacking_1.0-1/usr/local/bin/
cp $DIR/bambooslacking_1.0-1.deb /deb/