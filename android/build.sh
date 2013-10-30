#!/bin/sh

set -x

export BUILDDIR=`pwd`

NCPU=4

NDK=`which ndk-build`
NDK=`dirname $NDK`
NDK=`readlink -f $NDK`


[ -e pixman-0.30.2/pixman/.libs/libpixman-1.so ] || {
[ -e pixman-0.30.2 ] || curl http://cairographics.org/releases/pixman-0.30.2.tar.gz | tar xvz || exit 1
cd pixman-0.30.2

# -I`pwd`/../../../sdl/project/jni/png/include -L`pwd`/../../../sdl/project/

env CFLAGS="-I$NDK/sources/android/cpufeatures" \
LDFLAGS="-L$NDK/sources/android/libportable/libs/armeabi -lportable" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=arm-linux-androideabi \
--disable-arm-iwmmxt

cd pixman

$BUILDDIR/setCrossEnvironment.sh \
nice make -j$NCPU V=1 2>&1 || exit 1

cd $BUILDDIR
}


ln -sf $BUILDDIR/../../sdl/project/libs/armeabi/libsdl-1.2.so $BUILDDIR/libSDL.so
#ln -sf $BUILDDIR/pixman-0.30.2/pixman/.libs/libpixman-1.so $BUILDDIR/libpixman-1.so
ln -sf $BUILDDIR/pixman-0.30.2/pixman/.libs/libpixman-1.a $BUILDDIR/libpixman-1.a
ln -sf $NDK/sources/android/libportable/libs/armeabi/libportable.a $BUILDDIR/libpthread.a # dummy
ln -sf $NDK/sources/android/libportable/libs/armeabi/libportable.a $BUILDDIR/libts.a # dummy

[ -e Makefile ] || \
env CFLAGS="-include `pwd`/compiler-workarounds.h \
	-I$BUILDDIR/pixman-0.30.2/pixman \
	-I$BUILDDIR/../../sdl/project/jni/sdl-1.2/include" \
LDFLAGS="-L$BUILDDIR/../../sdl/project/libs/armeabi -L$BUILDDIR" \
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
