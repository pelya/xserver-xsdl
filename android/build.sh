#!/bin/sh

set -x

NCPU=8
export BUILDDIR=`pwd`
NDK=`which ndk-build`
NDK=`dirname $NDK`
NDK=`readlink -f $NDK`


[ -e pixman-0.30.2/libpixman.so ] || {
[ -e pixman-0.30.2 ] || curl http://cairographics.org/releases/pixman-0.30.2.tar.gz | tar xvz || exit 1
cd pixman-0.30.2

env CFLAGS="-I`pwd`/../../../sdl/project/jni/png/include -I$NDK/sources/android/cpufeatures" \
../setCrossEnvironment.sh \
./configure \
--host=arm-linux-androideabi \

../setCrossEnvironment.sh \
nice make -j$NCPU V=1 2>&1 || exit 1

cd $BUILDDIR
}



[ -e Makefile ] || \
env CFLAGS="-include `pwd`/compiler-workarounds.h" \
./setCrossEnvironment.sh \
../configure \
--host=arm-linux-androideabi \
--prefix=/usr \
--disable-xorg --disable-dmx --disable-xvfb --disable-xnest --disable-xquartz --disable-xwin \
--disable-xephyr --disable-xfake --disable-xfbdev --disable-unit-tests --disable-tslib \
--disable-shm --disable-mitshm --disable-dri --disable-dri2 --disable-glx --disable-xf86vidmode \
--enable-xsdl --enable-kdrive --enable-kdrive-kbd --enable-kdrive-mouse --enable-kdrive-evdev \
|| exit 1

ln -sf /usr/include/X11 include/X11
ln -sf ../../../sdl/project/jni/sdl-1.2/include include/SDL

{ ./setCrossEnvironment.sh nice -n19 make -j$NCPU V=1 2>&1 || exit 1 ; } | tee build.log
