#!/bin/sh

set -x

export BUILDDIR=`pwd`

NCPU=4

NDK=`which ndk-build`
NDK=`dirname $NDK`
NDK=`readlink -f $NDK`


[ -e libpixman-1.a ] || {
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
make -j$NCPU V=1 2>&1 || exit 1

cd $BUILDDIR
ln -sf $BUILDDIR/pixman-0.30.2/pixman/.libs/libpixman-1.a $BUILDDIR/libpixman-1.a
}

[ -e X11/Xfuncproto.h ] || {
curl http://cgit.freedesktop.org/xorg/proto/x11proto/snapshot/xproto-7.0.24.tar.gz | tar xvz || exit 1
ln -sf xproto-7.0.24 X11
cd X11
$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=arm-linux-androideabi \
|| exit 1
cd $BUILDDIR
}

[ -e X11/fonts/fontstruct.h ] || {
curl http://cgit.freedesktop.org/xorg/proto/fontsproto/snapshot/fontsproto-2.1.2.tar.gz | tar xvz || exit 1
ln -sf ../fontsproto-2.1.2 X11/fonts
cd X11/fonts
$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=arm-linux-androideabi \
|| exit 1
cd $BUILDDIR
}

[ -e libfontenc.a ] || {
[ -e libfontenc-1.1.2 ] || curl http://cgit.freedesktop.org/xorg/lib/libfontenc/snapshot/libfontenc-1.1.2.tar.gz | tar xvz || exit 1

cd libfontenc-1.1.2

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR -include strings.h" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=arm-linux-androideabi \
|| exit 1

cp -f `which libtool` ./

$BUILDDIR/setCrossEnvironment.sh \
sh -c 'ln -sf $CC gcc'

env PATH=`pwd`:$PATH \
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 2>&1 || exit 1

cd $BUILDDIR
ln -sf ../libfontenc-1.1.2/include/X11/fonts/fontenc.h X11/fonts/
ln -sf libfontenc-1.1.2/src/.libs/libfontenc.a ./
}

[ -e xtrans-1.2.7 ] || {
[ -e xtrans-1.2.7 ] || curl http://cgit.freedesktop.org/xorg/lib/libxtrans/snapshot/xtrans-1.2.7.tar.gz | tar xvz || exit 1

cd xtrans-1.2.7

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR -include strings.h" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=arm-linux-androideabi \
|| exit 1

cd $BUILDDIR
ln -sf ../xtrans-1.2.7 X11/Xtrans
}

ln -sf $BUILDDIR/../../sdl/project/obj/local/armeabi-v7a/libfreetype.a $BUILDDIR/

[ -e libXfont.a ] || {
[ -e libXfont-1.4.6 ] || curl http://cgit.freedesktop.org/xorg/lib/libXfont/snapshot/libXfont-1.4.6.tar.gz | tar xvz || exit 1

cd libXfont-1.4.6

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR \
-include strings.h \
-I$BUILDDIR/../../sdl/project/jni/freetype/include \
-DNO_LOCALE" \
LDFLAGS="-L$BUILDDIR" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=arm-linux-androideabi \
|| exit 1

cp -f `which libtool` ./

$BUILDDIR/setCrossEnvironment.sh \
sh -c 'ln -sf $CC gcc'

env PATH=`pwd`:$PATH \
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 2>&1 || exit 1

cd $BUILDDIR
ln -sf libXfont-1.4.6/src/.libs/libXfont.a ./
for F in libXfont-1.4.6/include/X11/fonts/* ; do
ln -sf ../$F X11/fonts/
done
}

[ -e libXau.a ] || {
[ -e libXau-1.0.8 ] || curl http://cgit.freedesktop.org/xorg/lib/libXau/snapshot/libXau-1.0.8.tar.gz | tar xvz || exit 1

cd libXau-1.0.8

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR \
-include strings.h" \
LDFLAGS="-L$BUILDDIR" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=arm-linux-androideabi \
|| exit 1

cp -f `which libtool` ./

$BUILDDIR/setCrossEnvironment.sh \
sh -c 'ln -sf $CC gcc'

env PATH=`pwd`:$PATH \
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 2>&1 || exit 1

cd $BUILDDIR
ln -sf libXau-1.0.8/.libs/libXau.a ./
ln -sf ../libXau-1.0.8/include/X11/Xauth.h X11/
}



[ -e libXdmcp.a ] || {
[ -e libXdmcp-1.1.1 ] || curl http://cgit.freedesktop.org/xorg/lib/libXdmcp/snapshot/libXdmcp-1.1.1.tar.gz | tar xvz || exit 1

cd libXdmcp-1.1.1

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR \
-include strings.h" \
LDFLAGS="-L$BUILDDIR" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=arm-linux-androideabi \
|| exit 1

cp -f `which libtool` ./

$BUILDDIR/setCrossEnvironment.sh \
sh -c 'ln -sf $CC gcc'

env PATH=`pwd`:$PATH \
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 2>&1 || exit 1

cd $BUILDDIR
ln -sf libXdmcp-1.1.1/.libs/libXdmcp.a ./
ln -sf ../libXdmcp-1.1.1/include/X11/Xdmcp.h X11/
}


ln -sf $BUILDDIR/../../sdl/project/libs/armeabi-v7a/libsdl-1.2.so $BUILDDIR/libSDL.so
ln -sf $NDK/sources/android/libportable/libs/armeabi-v7a/libportable.a $BUILDDIR/libpthread.a # dummy
ln -sf $NDK/sources/android/libportable/libs/armeabi-v7a/libportable.a $BUILDDIR/libts.a # dummy

[ -e Makefile ] || \
env CFLAGS="-include strings.h -include linux/time.h \
	-isystem$BUILDDIR \
	-I$BUILDDIR/pixman-0.30.2/pixman \
	-I$BUILDDIR/../../sdl/project/jni/sdl-1.2/include" \
LDFLAGS="-L$BUILDDIR" \
./setCrossEnvironment.sh \
LIBS="-lfontenc -lfreetype" \
../configure \
--host=arm-linux-androideabi \
--prefix=/usr \
--disable-xorg --disable-dmx --disable-xvfb --disable-xnest --disable-xquartz --disable-xwin \
--disable-xephyr --disable-xfake --disable-xfbdev --disable-unit-tests --disable-tslib \
--disable-shm --disable-mitshm --disable-dri --disable-dri2 --disable-glx --disable-xf86vidmode \
--enable-xsdl --enable-kdrive --enable-kdrive-kbd --enable-kdrive-mouse --enable-kdrive-evdev \
|| exit 1

{ ./setCrossEnvironment.sh make -j$NCPU V=1 2>&1 || exit 1 ; } | tee build.log
