#!/bin/sh

set -x

export BUILDDIR=`pwd`

NCPU=4
uname -s | grep -i "linux" && NCPU=`cat /proc/cpuinfo | grep -c -i processor`

NDK=`which ndk-build`
NDK=`dirname $NDK`
NDK=`readlink -f $NDK`

[ -z "$TARGET_ARCH" ] && TARGET_ARCH=armeabi-v7a
[ -z "$TARGET_HOST" ] && TARGET_HOST=arm-linux-androideabi

export enable_malloc0returnsnull=true # Workaround for buggy autotools

# =========== android-shmem ===========

[ -e libandroid-shmem.a ] || {

[ -e ../android-shmem/LICENSE ] || {
	cd ../..
	git submodule update --init android/android-shmem || exit 1
	cd $BUILDDIR
} || exit 1
[ -e ../android-shmem/libancillary/ancillary.h ] || {
	cd ../android-shmem
	git submodule update --init libancillary || exit 1
	cd $BUILDDIR
} || exit 1

$BUILDDIR/setCrossEnvironment.sh \
env NDK=$NDK \
sh -c '$CC $CFLAGS \
	-I ../android-shmem \
	-I ../android-shmem/libancillary \
	-c ../android-shmem/*.c && \
	ar rcs libandroid-shmem.a *.o && \
	rm -f *.o' \
|| exit 1
cd $BUILDDIR
} || exit 1

# =========== xsproto ===========

[ -e X11/Xfuncproto.h ] || {
curl http://cgit.freedesktop.org/xorg/proto/x11proto/snapshot/xproto-7.0.24.tar.gz | tar xvz || exit 1
ln -sf xproto-7.0.24 X11
cd X11
patch -p0 < ../../xproto.diff || exit 1
$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1
cd $BUILDDIR
} || exit 1

# =========== fontsproto ===========

[ -e X11/fonts/fontstruct.h ] || {
curl http://cgit.freedesktop.org/xorg/proto/fontsproto/snapshot/fontsproto-2.1.2.tar.gz | tar xvz || exit 1
ln -sf ../fontsproto-2.1.2 X11/fonts
cd X11/fonts
$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1
cd $BUILDDIR
} || exit 1

# =========== xtrans ===========

[ -e xtrans-1.2.7 ] || {
[ -e xtrans-1.2.7 ] || curl http://cgit.freedesktop.org/xorg/lib/libxtrans/snapshot/xtrans-1.2.7.tar.gz | tar xvz || exit 1

cd xtrans-1.2.7

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR -include strings.h" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=$TARGET_HOST \
|| exit 1

cd $BUILDDIR
ln -sf ../xtrans-1.2.7 X11/Xtrans
} || exit 1

# =========== xextproto ===========

[ -e X11/extensions/geproto.h ] || {
curl http://cgit.freedesktop.org/xorg/proto/xextproto/snapshot/xextproto-7.2.1.tar.gz | tar xvz || exit 1
ln -sf ../xextproto-7.2.1 X11/extensions
cd X11/extensions
$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1
cd $BUILDDIR
} || exit 1

# =========== inputproto ===========

[ -e X11/extensions/XI.h ] || {
curl http://cgit.freedesktop.org/xorg/proto/inputproto/snapshot/inputproto-2.3.tar.gz | tar xvz || exit 1
cd inputproto-2.3
$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1
cd $BUILDDIR
for F in inputproto-2.3/*.h ; do
ln -sf ../$F X11/extensions/
done
} || exit 1

# =========== kbproto ===========

[ -e X11/extensions/XKBproto.h ] || {
curl http://cgit.freedesktop.org/xorg/proto/kbproto/snapshot/kbproto-1.0.6.tar.gz | tar xvz || exit 1
cd kbproto-1.0.6
$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1
cd $BUILDDIR
for F in kbproto-1.0.6/*.h ; do
ln -sf ../$F X11/extensions/
done
} || exit 1

# =========== xineramaproto ===========

[ -e X11/extensions/panoramiXproto.h ] || {
curl http://cgit.freedesktop.org/xorg/proto/xineramaproto/snapshot/xineramaproto-1.2.1.tar.gz | tar xvz || exit 1
cd xineramaproto-1.2.1
$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1
cd $BUILDDIR
for F in xineramaproto-1.2.1/*.h ; do
ln -sf ../$F X11/extensions/
done
} || exit 1

# =========== renderproto ===========

[ -e X11/extensions/renderproto.h ] || {
curl http://cgit.freedesktop.org/xorg/proto/renderproto/snapshot/renderproto-0.11.1.tar.gz | tar xvz || exit 1
cd renderproto-0.11.1
$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1
cd $BUILDDIR
for F in renderproto-0.11.1/*.h ; do
ln -sf ../$F X11/extensions/
done
} || exit 1

# =========== xfixesproto ===========

[ -e X11/extensions/xfixesproto.h ] || {
curl http://cgit.freedesktop.org/xorg/proto/fixesproto/snapshot/fixesproto-5.0.tar.gz | tar xvz || exit 1
cd fixesproto-5.0
$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1
cd $BUILDDIR
for F in fixesproto-5.0/*.h ; do
ln -sf ../$F X11/extensions/
done
} || exit 1

# =========== damageproto ===========

[ -e X11/extensions/damageproto.h ] || {
curl http://cgit.freedesktop.org/xorg/proto/damageproto/snapshot/damageproto-1.2.1.tar.gz | tar xvz || exit 1
cd damageproto-1.2.1
$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1
cd $BUILDDIR
for F in damageproto-1.2.1/*.h ; do
ln -sf ../$F X11/extensions/
done
} || exit 1

# =========== compositeproto ===========

[ -e X11/extensions/compositeproto.h ] || {
curl http://cgit.freedesktop.org/xorg/proto/compositeproto/snapshot/compositeproto-0.4.2.tar.gz | tar xvz || exit 1
cd compositeproto-0.4.2
$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1
cd $BUILDDIR
for F in compositeproto-0.4.2/*.h ; do
ln -sf ../$F X11/extensions/
done
} || exit 1

# =========== bigreqsproto ===========

[ -e X11/extensions/bigreqsproto.h ] || {
curl http://cgit.freedesktop.org/xorg/proto/bigreqsproto/snapshot/bigreqsproto-1.1.2.tar.gz | tar xvz || exit 1
cd bigreqsproto-1.1.2
$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1
cd $BUILDDIR
for F in bigreqsproto-1.1.2/*.h ; do
ln -sf ../$F X11/extensions/
done
} || exit 1

# =========== scrnsaverproto ===========

[ -e X11/extensions/saver.h ] || {
curl http://cgit.freedesktop.org/xorg/proto/scrnsaverproto/snapshot/scrnsaverproto-1.2.2.tar.gz | tar xvz || exit 1
cd scrnsaverproto-1.2.2
$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1
cd $BUILDDIR
for F in scrnsaverproto-1.2.2/*.h ; do
ln -sf ../$F X11/extensions/
done
} || exit 1

# =========== videoproto ===========

[ -e X11/extensions/Xv.h ] || {
curl http://cgit.freedesktop.org/xorg/proto/videoproto/snapshot/videoproto-2.3.2.tar.gz | tar xvz || exit 1
cd videoproto-2.3.2
$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1
cd $BUILDDIR
for F in videoproto-2.3.2/*.h ; do
ln -sf ../$F X11/extensions/
done
} || exit 1

# =========== resourceproto ===========

[ -e X11/extensions/XResproto.h ] || {
curl http://cgit.freedesktop.org/xorg/proto/resourceproto/snapshot/resourceproto-1.2.0.tar.gz | tar xvz || exit 1
cd resourceproto-1.2.0
$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1
cd $BUILDDIR
for F in resourceproto-1.2.0/*.h ; do
ln -sf ../$F X11/extensions/
done
} || exit 1

# =========== xcmiscproto ===========

[ -e X11/extensions/xcmiscproto.h ] || {
curl http://cgit.freedesktop.org/xorg/proto/xcmiscproto/snapshot/xcmiscproto-1.2.2.tar.gz | tar xvz || exit 1
cd xcmiscproto-1.2.2
$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1
cd $BUILDDIR
for F in xcmiscproto-1.2.2/*.h ; do
ln -sf ../$F X11/extensions/
done
} || exit 1

# =========== randrproto ===========

[ -e X11/extensions/randr.h ] || {
curl http://cgit.freedesktop.org/xorg/proto/randrproto/snapshot/randrproto-1.4.0.tar.gz | tar xvz || exit 1
cd randrproto-1.4.0
$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1
cd $BUILDDIR
for F in randrproto-1.4.0/*.h ; do
ln -sf ../$F X11/extensions/
done
} || exit 1

# =========== xf86bigfontproto ===========

[ -e X11/extensions/xf86bigfproto.h ] || {
curl http://cgit.freedesktop.org/xorg/proto/xf86bigfontproto/snapshot/xf86bigfontproto-1.2.0.tar.gz | tar xvz || exit 1
cd xf86bigfontproto-1.2.0
$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1
cd $BUILDDIR
for F in xf86bigfontproto-1.2.0/*.h ; do
ln -sf ../$F X11/extensions/
done
} || exit 1

# =========== recordproto ===========

[ -e X11/extensions/recordproto.h ] || {
curl http://cgit.freedesktop.org/xorg/proto/recordproto/snapshot/recordproto-1.14.2.tar.gz | tar xvz || exit 1
cd recordproto-1.14.2
$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1
cd $BUILDDIR
for F in recordproto-1.14.2/*.h ; do
ln -sf ../$F X11/extensions/
done
} || exit 1

# =========== libpixman-1.a ===========

[ -e libpixman-1.a ] || {
curl http://cairographics.org/releases/pixman-0.30.2.tar.gz | tar xvz || exit 1
cd pixman-0.30.2

env CFLAGS="-I$NDK/sources/android/cpufeatures" \
LDFLAGS="-L$NDK/sources/android/libportable/libs/$TARGET_ARCH -lportable" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=$TARGET_HOST \
--disable-arm-iwmmxt

cd pixman
touch *.S

$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 2>&1 || exit 1

cd $BUILDDIR
ln -sf $BUILDDIR/pixman-0.30.2/pixman/.libs/libpixman-1.a $BUILDDIR/libpixman-1.a
} || exit 1

# =========== libfontenc.a ===========

[ -e libfontenc.a ] || {
curl http://cgit.freedesktop.org/xorg/lib/libfontenc/snapshot/libfontenc-1.1.2.tar.gz | tar xvz || exit 1

cd libfontenc-1.1.2

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR -include strings.h" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=$TARGET_HOST \
--prefix=$TARGET_DIR/usr \
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
} || exit 1

# =========== libXfont.a ===========

ln -sf $BUILDDIR/../../../../../../obj/local/$TARGET_ARCH/libfreetype.a $BUILDDIR/

[ -e libXfont.a ] || {
curl http://cgit.freedesktop.org/xorg/lib/libXfont/snapshot/libXfont-1.4.6.tar.gz | tar xvz || exit 1

cd libXfont-1.4.6

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR \
-include strings.h \
-I$BUILDDIR/../../../../../../jni/freetype/include \
-DNO_LOCALE" \
LDFLAGS="-L$BUILDDIR" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=$TARGET_HOST \
--prefix=$TARGET_DIR/usr \
|| exit 1

cp -f `which libtool` ./
#sed -i 's/pic_flag=.*/pic_flag=""/g' libtool

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
} || exit 1

# =========== libXau.a ==========

[ -e libXau.a ] || {
curl http://cgit.freedesktop.org/xorg/lib/libXau/snapshot/libXau-1.0.8.tar.gz | tar xvz || exit 1

cd libXau-1.0.8

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR \
-include strings.h" \
LDFLAGS="-L$BUILDDIR" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=$TARGET_HOST \
--prefix=$TARGET_DIR/usr \
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
} || exit 1

# =========== libXdmcp.a ==========

[ -e libXdmcp.a ] || {
curl http://cgit.freedesktop.org/xorg/lib/libXdmcp/snapshot/libXdmcp-1.1.1.tar.gz | tar xvz || exit 1

cd libXdmcp-1.1.1

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR \
-include strings.h" \
LDFLAGS="-L$BUILDDIR" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=$TARGET_HOST \
--prefix=$TARGET_DIR/usr \
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
} || exit 1

# =========== xcbproto ===========
[ -e proto-1.8 ] || {
curl http://cgit.freedesktop.org/xcb/proto/snapshot/proto-1.8.tar.gz | tar xvz || exit 1
cd proto-1.8
$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 2>&1 || exit 1
cd $BUILDDIR
} || exit 1

# =========== libxcb.a ==========

[ -e libxcb.a ] || {
curl http://cgit.freedesktop.org/xcb/libxcb/snapshot/libxcb-1.10.tar.gz | tar xvz || exit 1

cd libxcb-1.10

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR \
-include strings.h" \
LDFLAGS="-L$BUILDDIR" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=$TARGET_HOST \
--prefix=$TARGET_DIR/usr \
|| exit 1

cp -f `which libtool` ./

$BUILDDIR/setCrossEnvironment.sh \
sh -c 'ln -sf $CC gcc'

env PATH=`pwd`:$PATH \
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 2>&1 || exit 1

cd $BUILDDIR
ln -sf libxcb-1.10/src/.libs/libxcb.a ./
mkdir -p xcb
ln -sf ../libxcb-1.10/src/xcb.h xcb/
ln -sf ../libxcb-1.10/src/xproto.h xcb/
ln -sf ../libxcb-1.10/src/xcbext.h xcb/
} || exit 1

[ -e libandroid_support.a ] || {
mkdir -p android_support
cd android_support
$BUILDDIR/setCrossEnvironment.sh \
env NDK=$NDK \
sh -c '$CC $CFLAGS -Drestrict=__restrict__ -ffunction-sections -fdata-sections \
	-I $NDK/sources/android/support/include \
	-c $NDK/sources/android/support/src/musl-multibyte/*.c && \
	ar rcs ../libandroid_support.a *.o' \
|| exit 1
cd $BUILDDIR
} || exit 1

# =========== libX11.a ==========

[ -e libX11.a ] || {
curl http://cgit.freedesktop.org/xorg/lib/libX11/snapshot/libX11-1.6.2.tar.gz | tar xvz || exit 1

cd libX11-1.6.2

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR \
			-isystem$BUILDDIR/../android-shmem \
			-include strings.h" \
LDFLAGS="-L$BUILDDIR" \
$BUILDDIR/setCrossEnvironment.sh \
LIBS="-lXau -lXdmcp -landroid_support -landroid-shmem" \
./configure \
--host=$TARGET_HOST \
--prefix=$TARGET_DIR/usr \
|| exit 1

cp -f `which libtool` ./

$BUILDDIR/setCrossEnvironment.sh \
sh -c 'ln -sf $CC gcc'

echo "all: makekeys" > src/util/Makefile
echo "makekeys:" >> src/util/Makefile
echo "	/usr/bin/gcc makekeys.c -o makekeys -I /usr/include" >> src/util/Makefile

env PATH=`pwd`:$PATH \
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 2>&1 || exit 1

cd $BUILDDIR
ln -sf libX11-1.6.2/src/.libs/libX11.a ./
for F in libX11-1.6.2/include/X11/*.h ; do
ln -sf ../$F X11
done
} || exit 1

# =========== libXext.a ==========

[ -e libXext.a ] || {
PKGURL=http://cgit.freedesktop.org/xorg/lib/libXext/snapshot/libXext-1.3.2.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
curl $PKGURL | tar xvz || exit 1

cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR \
-include strings.h" \
LDFLAGS="-L$BUILDDIR" \
LIBS="-lxcb -lXau -lXdmcp -landroid_support" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=$TARGET_HOST \
--prefix=$TARGET_DIR/usr \
|| exit 1

cp -f `which libtool` ./

$BUILDDIR/setCrossEnvironment.sh \
sh -c 'ln -sf $CC gcc'

env PATH=`pwd`:$PATH \
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 2>&1 || exit 1

cd $BUILDDIR
ln -sf $PKGDIR/src/.libs/libXext.a ./
for F in $PKGDIR/include/X11/extensions/*.h ; do
ln -sf ../$F X11/extensions/
done
} || exit 1

# =========== libXrender.a ==========

[ -e libXrender.a ] || {
PKGURL=http://cgit.freedesktop.org/xorg/lib/libXrender/snapshot/libXrender-0.9.8.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
curl $PKGURL | tar xvz || exit 1

cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR \
-include strings.h" \
LDFLAGS="-L$BUILDDIR" \
LIBS="-lxcb -lXau -lXdmcp -landroid_support" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=$TARGET_HOST \
--prefix=$TARGET_DIR/usr \
|| exit 1

cp -f `which libtool` ./

$BUILDDIR/setCrossEnvironment.sh \
sh -c 'ln -sf $CC gcc'

env PATH=`pwd`:$PATH \
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 2>&1 || exit 1

cd $BUILDDIR
ln -sf $PKGDIR/src/.libs/libXrender.a ./
for F in $PKGDIR/include/X11/extensions/*.h ; do
ln -sf ../$F X11/extensions/
done
} || exit 1

# =========== libXrandr.a ==========

[ -e libXrandr.a ] || {
PKGURL=http://cgit.freedesktop.org/xorg/lib/libXrandr/snapshot/libXrandr-1.4.2.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
curl $PKGURL | tar xvz || exit 1

cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR \
-include strings.h" \
LDFLAGS="-L$BUILDDIR" \
LIBS="-lxcb -lXau -lXdmcp -landroid_support" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=$TARGET_HOST \
--prefix=$TARGET_DIR/usr \
|| exit 1

cp -f `which libtool` ./

$BUILDDIR/setCrossEnvironment.sh \
sh -c 'ln -sf $CC gcc'

env PATH=`pwd`:$PATH \
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 2>&1 || exit 1

cd $BUILDDIR
ln -sf $PKGDIR/src/.libs/libXrandr.a ./
for F in $PKGDIR/include/X11/extensions/*.h ; do
ln -sf ../$F X11/extensions/
done
} || exit 1

# =========== libxkbfile.a ==========

[ -e libxkbfile.a ] || {
curl http://cgit.freedesktop.org/xorg/lib/libxkbfile/snapshot/libxkbfile-1.0.8.tar.gz | tar xvz || exit 1

cd libxkbfile-1.0.8

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR \
-include strings.h" \
LDFLAGS="-L$BUILDDIR" \
LIBS="-lxcb -lXau -lXdmcp -landroid_support" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=$TARGET_HOST \
--prefix=$TARGET_DIR/usr \
|| exit 1

cp -f `which libtool` ./

$BUILDDIR/setCrossEnvironment.sh \
sh -c 'ln -sf $CC gcc'

env PATH=`pwd`:$PATH \
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 2>&1 || exit 1

cd $BUILDDIR
ln -sf libxkbfile-1.0.8/src/.libs/libxkbfile.a ./
for F in libxkbfile-1.0.8/include/X11/extensions/*.h ; do
ln -sf ../$F X11/extensions/
done
} || exit 1

# =========== xkbcomp binary ==========

[ -e xkbcomp ] || {
curl http://cgit.freedesktop.org/xorg/app/xkbcomp/snapshot/xkbcomp-1.2.4.tar.gz | tar xvz || exit 1

cd xkbcomp-1.2.4

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR \
-include strings.h" \
LDFLAGS="-L$BUILDDIR" \
LIBS="-lxcb -lXau -lXdmcp -landroid_support -lX11" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=$TARGET_HOST \
--prefix=$TARGET_DIR/usr \
|| exit 1

cp -f `which libtool` ./

$BUILDDIR/setCrossEnvironment.sh \
sh -c 'ln -sf $CC gcc'

env PATH=`pwd`:$PATH \
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 2>&1 || exit 1

cd $BUILDDIR
cp -f xkbcomp-1.2.4/xkbcomp ./
$BUILDDIR/setCrossEnvironment.sh \
sh -c '$STRIP xkbcomp'
} || exit 1

# =========== libICE.a ==========

[ -e libICE.a ] || {
PKGURL=http://cgit.freedesktop.org/xorg/lib/libICE/snapshot/libICE-1.0.8.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
curl $PKGURL | tar xvz || exit 1
cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

#LIBS="-lxcb -lXau -lXdmcp -landroid_support" \

env CFLAGS="-isystem$BUILDDIR \
-include strings.h" \
LDFLAGS="-L$BUILDDIR" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=$TARGET_HOST \
--prefix=$TARGET_DIR/usr \
|| exit 1

cp -f `which libtool` ./

$BUILDDIR/setCrossEnvironment.sh \
sh -c 'ln -sf $CC gcc'

env PATH=`pwd`:$PATH \
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 2>&1 || exit 1

cd $BUILDDIR
ln -sf $PKGDIR/src/.libs/libICE.a ./
ln -sf ../$PKGDIR/include/X11/ICE X11/
} || exit 1

# =========== libSM.a ==========

[ -e libSM.a ] || {
PKGURL=http://cgit.freedesktop.org/xorg/lib/libSM/snapshot/libSM-1.2.2.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
curl $PKGURL | tar xvz || exit 1

cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

#LIBS="-lxcb -lXau -lXdmcp -landroid_support" \

env CFLAGS="-isystem$BUILDDIR \
-include strings.h" \
LDFLAGS="-L$BUILDDIR" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=$TARGET_HOST \
--prefix=$TARGET_DIR/usr \
|| exit 1

cp -f `which libtool` ./

$BUILDDIR/setCrossEnvironment.sh \
sh -c 'ln -sf $CC gcc'

env PATH=`pwd`:$PATH \
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 2>&1 || exit 1

cd $BUILDDIR
ln -sf $PKGDIR/src/.libs/libSM.a ./
ln -sf ../$PKGDIR/include/X11/SM X11/
} || exit 1

# =========== libXt.a ==========

[ -e libXt.a ] || {
PKGURL=http://cgit.freedesktop.org/xorg/lib/libXt/snapshot/libXt-1.1.4.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
curl $PKGURL | tar xvz || exit 1
cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR \
-include strings.h" \
LDFLAGS="-L$BUILDDIR" \
LIBS="-lxcb -lXau -lXdmcp -landroid_support" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=$TARGET_HOST \
--prefix=$TARGET_DIR/usr \
|| exit 1

cp -f `which libtool` ./

$BUILDDIR/setCrossEnvironment.sh \
sh -c 'ln -sf $CC gcc'

echo "all: makestrs" > util/Makefile
echo "makestrs:" >> util/Makefile
echo "	/usr/bin/gcc makestrs.c -o makestrs -I /usr/include" >> util/Makefile

env PATH=`pwd`:$PATH \
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 2>&1 || exit 1

cd $BUILDDIR
ln -sf $PKGDIR/src/.libs/libXt.a ./
for F in $PKGDIR/include/X11/*.h ; do
ln -sf ../$F X11/
done
} || exit 1

# =========== libXmu.a ==========

[ -e libXmu.a ] || {
PKGURL=http://cgit.freedesktop.org/xorg/lib/libXmu/snapshot/libXmu-1.1.2.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
curl $PKGURL | tar xvz || exit 1
cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR \
-include strings.h" \
LDFLAGS="-L$BUILDDIR" \
LIBS="-lxcb -lXau -lXdmcp -landroid_support -lSM -lICE" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=$TARGET_HOST \
--prefix=$TARGET_DIR/usr \
|| exit 1

cp -f `which libtool` ./

$BUILDDIR/setCrossEnvironment.sh \
sh -c 'ln -sf $CC gcc'

env PATH=`pwd`:$PATH \
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 2>&1 || exit 1

cd $BUILDDIR
ln -sf $PKGDIR/src/.libs/libXmuu.a ./
ln -sf $PKGDIR/src/.libs/libXmu.a ./
ln -sf ../$PKGDIR/include/X11/Xmu X11/
} || exit 1

# =========== xhost binary ==========

[ -e xhost ] || {
PKGURL=http://cgit.freedesktop.org/xorg/app/xhost/snapshot/xhost-1.0.6.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
curl $PKGURL | tar xvz || exit 1
cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR \
-include strings.h \
-Dsethostent=abs -Dendhostent=sync" \
LDFLAGS="-L$BUILDDIR" \
LIBS="-lxcb -lXau -lXdmcp -landroid_support -lX11" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=$TARGET_HOST \
--prefix=$TARGET_DIR/usr \
|| exit 1

cp -f `which libtool` ./

$BUILDDIR/setCrossEnvironment.sh \
sh -c 'ln -sf $CC gcc'

env PATH=`pwd`:$PATH \
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 2>&1 || exit 1

cd $BUILDDIR
cp -f $PKGDIR/xhost ./
$BUILDDIR/setCrossEnvironment.sh \
sh -c '$STRIP xhost'
} || exit 1

# =========== xli binary ==========

[ -e xli ] || {
PKGURL=http://web.aanet.com.au/gwg/xli-1.16.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
curl $PKGURL | tar xvz || exit 1
cd $PKGDIR

echo "SRCS = bright.c clip.c cmuwmrast.c compress.c dither.c faces.c fbm.c \
      fill.c  g3.c gif.c halftone.c imagetypes.c img.c mac.c mcidas.c \
      mc_tables.c merge.c misc.c new.c options.c path.c pbm.c pcx.c \
      reduce.c jpeg.c jpeglib.c rle.c rlelib.c root.c rotate.c send.c smooth.c \
      sunraster.c value.c window.c xbitmap.c xli.c \
      xpixmap.c xwd.c zio.c zoom.c ddxli.c doslib.c tga.c bmp.c pcd.c " > Makefile
echo 'xli: $(SRCS)' >> Makefile
echo '	$(CC) $(CFLAGS) $(SRCS) -o xli $(LDFLAGS) -lX11 -lxcb -lXau -lXdmcp -landroid_support -lm' >> Makefile

echo '#include <stdarg.h>' > varargs.h

sed -i 's/extern int errno;//' *.c

env CFLAGS="-isystem$BUILDDIR \
-include strings.h \
-include sys/select.h \
-include math.h \
-include stdlib.h \
-include string.h \
-Drindex=strchr \
-isystem ." \
LDFLAGS="-L$BUILDDIR" \
$BUILDDIR/setCrossEnvironment.sh \
make V=1 2>&1 || exit 1

cd $BUILDDIR
cp -f $PKGDIR/xli ./
$BUILDDIR/setCrossEnvironment.sh \
sh -c '$STRIP xli'
} || exit 1

# =========== xsel binary ==========

[ -e xsel ] || {
PKGURL=https://github.com/kfish/xsel/archive/master.tar.gz
PKGDIR=xsel-master
echo $PKGDIR: $PKGURL
curl -L $PKGURL | tar xvz || exit 1
cd $PKGDIR

env CFLAGS="-isystem$BUILDDIR -Drpl_malloc=malloc" \
LDFLAGS="-L$BUILDDIR" \
LIBS="-lX11 -lxcb -lXau -lXdmcp -landroid_support" \
$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1

$BUILDDIR/setCrossEnvironment.sh \
make V=1 2>&1 || exit 1

cd $BUILDDIR
cp -f $PKGDIR/xsel ./ || exit 1
$BUILDDIR/setCrossEnvironment.sh \
sh -c '$STRIP xsel'
} || exit 1

# =========== xsdl ==========

ln -sf $BUILDDIR/../../../../../../libs/$TARGET_ARCH/libsdl-1.2.so $BUILDDIR/libSDL.so
ln -sf $NDK/sources/android/libportable/libs/$TARGET_ARCH/libportable.a $BUILDDIR/libpthread.a # dummy
ln -sf $NDK/sources/android/libportable/libs/$TARGET_ARCH/libportable.a $BUILDDIR/libts.a # dummy

[ -z "$PACKAGE_NAME" ] && PACKAGE_NAME=X.org.server

[ -e Makefile ] && grep "`pwd`" Makefile > /dev/null || \
env CFLAGS=" -DDEBUG \
	-isystem$BUILDDIR \
	-isystem$BUILDDIR/../android-shmem \
	-include strings.h\
	-include linux/time.h \
	-DFNONBLOCK=O_NONBLOCK \
	-DFNDELAY=O_NDELAY \
	-D_LINUX_IPC_H \
	-Dipc_perm=debian_ipc_perm \
	-I$BUILDDIR/pixman-0.30.2/pixman \
	-I$BUILDDIR/../../../../../../jni/sdl-1.2/include" \
LDFLAGS="-L$BUILDDIR" \
./setCrossEnvironment.sh \
LIBS="-lfontenc -lfreetype -llog -lSDL -lGLESv1_CM -landroid-shmem" \
../../configure \
--host=$TARGET_HOST \
--prefix=$TARGET_DIR/usr \
--with-xkb-output=$TARGET_DIR/tmp \
--disable-xorg --disable-dmx --disable-xvfb --disable-xnest --disable-xquartz --disable-xwin \
--disable-xephyr --disable-xfake --disable-xfbdev --disable-unit-tests --disable-tslib \
--disable-dri --disable-dri2 --disable-glx --disable-xf86vidmode \
--enable-xsdl --enable-kdrive --enable-kdrive-kbd --enable-kdrive-mouse --enable-kdrive-evdev \
--enable-shm --enable-mitshm \
|| exit 1

./setCrossEnvironment.sh make -j$NCPU V=1 2>&1 || exit 1

exit 0
