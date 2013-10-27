#!/bin/sh

set -x

NCPU=4
export BUILDDIR=`pwd`

[ -e Makefile ] || \
env CFLAGS="-include `pwd`/compiler-workarounds.h" \
./setCrossEnvironment.sh \
../configure \
--host=arm-linux-androideabi \
--prefix=/usr \
--enable-debug \
--disable-xorg --disable-dmx --disable-xvfb --disable-xnest --disable-xquartz --disable-xwin \
--disable-xephyr --disable-xfake --disable-xfbdev --disable-unit-tests --disable-tslib \
--disable-shm --disable-mitshm --disable-dri --disable-dri2 --disable-glx --disable-xf86vidmode \
--enable-xsdl --enable-kdrive --enable-kdrive-kbd --enable-kdrive-mouse --enable-kdrive-evdev \
|| exit 1

ln -sf /usr/include/X11 include/X11
ln -sf ../../../sdl/project/jni/sdl-1.2/include include/SDL

{ ./setCrossEnvironment.sh nice -n19 make -j8 V=1 2>&1 || exit 1 ; } | tee build.log
