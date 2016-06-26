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
PKGURL=https://cgit.freedesktop.org/xorg/proto/x11proto/snapshot/xproto-7.0.24.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || curl $PKGURL -o ../$PKGDIR.tar.gz || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1

ln -sf $PKGDIR X11
cd X11
patch -p0 < ../../xproto.diff || exit 1
$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1
cd $BUILDDIR
} || exit 1

# =========== fontsproto ===========

[ -e X11/fonts/fontstruct.h ] || {
PKGURL=https://cgit.freedesktop.org/xorg/proto/fontsproto/snapshot/fontsproto-2.1.2.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || curl $PKGURL -o ../$PKGDIR.tar.gz || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1

ln -sf ../$PKGDIR X11/fonts
cd X11/fonts
$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1
cd $BUILDDIR
} || exit 1

# =========== xtrans ===========

[ -e xtrans-1.2.7 ] || {
PKGURL=https://cgit.freedesktop.org/xorg/lib/libxtrans/snapshot/xtrans-1.2.7.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || curl $PKGURL -o ../$PKGDIR.tar.gz || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

patch -p0 < ../../Xtrans.c.diff || exit 1

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR -include strings.h -DSO_REUSEADDR=1" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=$TARGET_HOST \
|| exit 1

cd $BUILDDIR
ln -sf ../$PKGDIR X11/Xtrans
} || exit 1

# =========== xextproto ===========

[ -e X11/extensions/geproto.h ] || {
PKGURL=https://cgit.freedesktop.org/xorg/proto/xextproto/snapshot/xextproto-7.2.1.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || curl $PKGURL -o ../$PKGDIR.tar.gz || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1

ln -sf ../$PKGDIR X11/extensions
cd X11/extensions
$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1
cd $BUILDDIR
} || exit 1

# =========== inputproto ===========

[ -e X11/extensions/XI.h ] || {
PKGURL=https://cgit.freedesktop.org/xorg/proto/inputproto/snapshot/inputproto-2.3.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || curl $PKGURL -o ../$PKGDIR.tar.gz || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1
cd $BUILDDIR
for F in $PKGDIR/*.h ; do
ln -sf ../$F X11/extensions/
done
} || exit 1

# =========== kbproto ===========

[ -e X11/extensions/XKBproto.h ] || {
PKGURL=https://cgit.freedesktop.org/xorg/proto/kbproto/snapshot/kbproto-1.0.6.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || curl $PKGURL -o ../$PKGDIR.tar.gz || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1
cd $BUILDDIR
for F in $PKGDIR/*.h ; do
ln -sf ../$F X11/extensions/
done
} || exit 1

# =========== xineramaproto ===========

[ -e X11/extensions/panoramiXproto.h ] || {
PKGURL=https://cgit.freedesktop.org/xorg/proto/xineramaproto/snapshot/xineramaproto-1.2.1.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || curl $PKGURL -o ../$PKGDIR.tar.gz || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1
cd $BUILDDIR
for F in $PKGDIR/*.h ; do
ln -sf ../$F X11/extensions/
done
} || exit 1

# =========== renderproto ===========

[ -e X11/extensions/renderproto.h ] || {
PKGURL=https://cgit.freedesktop.org/xorg/proto/renderproto/snapshot/renderproto-0.11.1.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || curl $PKGURL -o ../$PKGDIR.tar.gz || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1
cd $BUILDDIR
for F in $PKGDIR/*.h ; do
ln -sf ../$F X11/extensions/
done
} || exit 1

# =========== xfixesproto ===========

[ -e X11/extensions/xfixesproto.h ] || {
PKGURL=https://cgit.freedesktop.org/xorg/proto/fixesproto/snapshot/fixesproto-5.0.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || curl $PKGURL -o ../$PKGDIR.tar.gz || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1
cd $BUILDDIR
for F in $PKGDIR/*.h ; do
ln -sf ../$F X11/extensions/
done
} || exit 1

# =========== damageproto ===========

[ -e X11/extensions/damageproto.h ] || {
PKGURL=https://cgit.freedesktop.org/xorg/proto/damageproto/snapshot/damageproto-1.2.1.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || curl $PKGURL -o ../$PKGDIR.tar.gz || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1
cd $BUILDDIR
for F in $PKGDIR/*.h ; do
ln -sf ../$F X11/extensions/
done
} || exit 1

# =========== compositeproto ===========

[ -e X11/extensions/compositeproto.h ] || {
PKGURL=https://cgit.freedesktop.org/xorg/proto/compositeproto/snapshot/compositeproto-0.4.2.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || curl $PKGURL -o ../$PKGDIR.tar.gz || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1
cd $BUILDDIR
for F in $PKGDIR/*.h ; do
ln -sf ../$F X11/extensions/
done
} || exit 1

# =========== bigreqsproto ===========

[ -e X11/extensions/bigreqsproto.h ] || {
PKGURL=https://cgit.freedesktop.org/xorg/proto/bigreqsproto/snapshot/bigreqsproto-1.1.2.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || curl $PKGURL -o ../$PKGDIR.tar.gz || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1
cd $BUILDDIR
for F in $PKGDIR/*.h ; do
ln -sf ../$F X11/extensions/
done
} || exit 1

# =========== scrnsaverproto ===========

[ -e X11/extensions/saver.h ] || {
PKGURL=https://cgit.freedesktop.org/xorg/proto/scrnsaverproto/snapshot/scrnsaverproto-1.2.2.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || curl $PKGURL -o ../$PKGDIR.tar.gz || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1
cd $BUILDDIR
for F in $PKGDIR/*.h ; do
ln -sf ../$F X11/extensions/
done
} || exit 1

# =========== videoproto ===========

[ -e X11/extensions/Xv.h ] || {
PKGURL=https://cgit.freedesktop.org/xorg/proto/videoproto/snapshot/videoproto-2.3.2.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || curl $PKGURL -o ../$PKGDIR.tar.gz || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1
cd $BUILDDIR
for F in $PKGDIR/*.h ; do
ln -sf ../$F X11/extensions/
done
} || exit 1

# =========== resourceproto ===========

[ -e X11/extensions/XResproto.h ] || {
PKGURL=https://cgit.freedesktop.org/xorg/proto/resourceproto/snapshot/resourceproto-1.2.0.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || curl $PKGURL -o ../$PKGDIR.tar.gz || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1
cd $BUILDDIR
for F in $PKGDIR/*.h ; do
ln -sf ../$F X11/extensions/
done
} || exit 1

# =========== xcmiscproto ===========

[ -e X11/extensions/xcmiscproto.h ] || {
PKGURL=https://cgit.freedesktop.org/xorg/proto/xcmiscproto/snapshot/xcmiscproto-1.2.2.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || curl $PKGURL -o ../$PKGDIR.tar.gz || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1
cd $BUILDDIR
for F in $PKGDIR/*.h ; do
ln -sf ../$F X11/extensions/
done
} || exit 1

# =========== randrproto ===========

[ -e X11/extensions/randr.h ] || {
PKGURL=https://cgit.freedesktop.org/xorg/proto/randrproto/snapshot/randrproto-1.4.0.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || curl $PKGURL -o ../$PKGDIR.tar.gz || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1
cd $BUILDDIR
for F in $PKGDIR/*.h ; do
ln -sf ../$F X11/extensions/
done
} || exit 1

# =========== xf86bigfontproto ===========

[ -e X11/extensions/xf86bigfproto.h ] || {
PKGURL=https://cgit.freedesktop.org/xorg/proto/xf86bigfontproto/snapshot/xf86bigfontproto-1.2.0.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || curl $PKGURL -o ../$PKGDIR.tar.gz || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1
cd $BUILDDIR
for F in $PKGDIR/*.h ; do
ln -sf ../$F X11/extensions/
done
} || exit 1

# =========== recordproto ===========

[ -e X11/extensions/recordproto.h ] || {
PKGURL=https://cgit.freedesktop.org/xorg/proto/recordproto/snapshot/recordproto-1.14.2.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || curl $PKGURL -o ../$PKGDIR.tar.gz || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1
cd $BUILDDIR
for F in $PKGDIR/*.h ; do
ln -sf ../$F X11/extensions/
done
} || exit 1

# =========== libportable.a ===========
[ -e libportable.a ] || {
	rm -rf libportable
	mkdir -p libportable
	$BUILDDIR/setCrossEnvironment.sh \
	sh -c 'cd libportable && \
	$CC $CFLAGS -c '"$NDK/sources/android/cpufeatures/*.c"' && \
	ar rcs ../libportable.a *.o' || exit 1
} || exit 1

# =========== libpixman-1.a ===========

[ -e libpixman-1.a ] || {
PKGURL=http://cairographics.org/releases/pixman-0.30.2.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || curl $PKGURL -o ../$PKGDIR.tar.gz || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

env CFLAGS="-I$NDK/sources/android/cpufeatures -DSO_REUSEADDR=1" \
LDFLAGS="-L$BUILDDIR -lportable" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=$TARGET_HOST \
--disable-arm-iwmmxt

sed -i "s/TOOLCHAIN_SUPPORTS_ATTRIBUTE_CONSTRUCTOR/DISABLE_TOOLCHAIN_SUPPORTS_ATTRIBUTE_CONSTRUCTOR/g" config.h

cd pixman
touch *.S

$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 2>&1 || exit 1

cd $BUILDDIR
ln -sf $BUILDDIR/$PKGDIR/pixman/.libs/libpixman-1.a $BUILDDIR/libpixman-1.a
} || exit 1

# =========== libfontenc.a ===========

[ -e libfontenc.a ] || {
PKGURL=https://cgit.freedesktop.org/xorg/lib/libfontenc/snapshot/libfontenc-1.1.2.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || curl $PKGURL -o ../$PKGDIR.tar.gz || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR -include strings.h -DSO_REUSEADDR=1" \
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
ln -sf ../$PKGDIR/include/X11/fonts/fontenc.h X11/fonts/
ln -sf $PKGDIR/src/.libs/libfontenc.a ./
} || exit 1

# =========== libXfont.a ===========

ln -sf $BUILDDIR/../../../../../../obj/local/$TARGET_ARCH/libfreetype.a $BUILDDIR/

[ -e libXfont.a ] || {
PKGURL=https://cgit.freedesktop.org/xorg/lib/libXfont/snapshot/libXfont-1.4.6.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || curl $PKGURL -o ../$PKGDIR.tar.gz || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR \
-include strings.h \
-I$BUILDDIR/../../../../../../jni/freetype/include \
-DNO_LOCALE -DSO_REUSEADDR=1" \
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
ln -sf $PKGDIR/src/.libs/libXfont.a ./
for F in $PKGDIR/include/X11/fonts/* ; do
ln -sf ../$F X11/fonts/
done
} || exit 1

# =========== libXau.a ==========

[ -e libXau.a ] || {
PKGURL=https://cgit.freedesktop.org/xorg/lib/libXau/snapshot/libXau-1.0.8.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || curl $PKGURL -o ../$PKGDIR.tar.gz || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR \
-include strings.h -DSO_REUSEADDR=1" \
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
ln -sf $PKGDIR/.libs/libXau.a ./
ln -sf ../$PKGDIR/include/X11/Xauth.h X11/
} || exit 1

# =========== libXdmcp.a ==========

[ -e libXdmcp.a ] || {
PKGURL=https://cgit.freedesktop.org/xorg/lib/libXdmcp/snapshot/libXdmcp-1.1.1.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || curl $PKGURL -o ../$PKGDIR.tar.gz || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR \
-include strings.h -DSO_REUSEADDR=1" \
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
ln -sf $PKGDIR/.libs/libXdmcp.a ./
ln -sf ../$PKGDIR/include/X11/Xdmcp.h X11/
} || exit 1

# =========== xcbproto ===========
[ -e proto-1.8 ] || {
PKGURL=https://cgit.freedesktop.org/xcb/proto/snapshot/proto-1.8.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || curl $PKGURL -o ../$PKGDIR.tar.gz || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 2>&1 || exit 1
cd $BUILDDIR
} || exit 1

# =========== libxcb.a ==========

[ -e libxcb.a ] || {
PKGURL=https://cgit.freedesktop.org/xcb/libxcb/snapshot/libxcb-1.10.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || curl $PKGURL -o ../$PKGDIR.tar.gz || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR \
-include strings.h -DSO_REUSEADDR=1" \
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
ln -sf $PKGDIR/src/.libs/libxcb.a ./
mkdir -p xcb
ln -sf ../$PKGDIR/src/xcb.h xcb/
ln -sf ../$PKGDIR/src/xproto.h xcb/
ln -sf ../$PKGDIR/src/xcbext.h xcb/
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
PKGURL=https://cgit.freedesktop.org/xorg/lib/libX11/snapshot/libX11-1.6.2.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || curl $PKGURL -o ../$PKGDIR.tar.gz || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR \
			-isystem$BUILDDIR/../android-shmem \
			-include strings.h -DSO_REUSEADDR=1" \
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
ln -sf $PKGDIR/src/.libs/libX11.a ./
for F in $PKGDIR/include/X11/*.h ; do
ln -sf ../$F X11
done
} || exit 1

# =========== libXext.a ==========

[ -e libXext.a ] || {
PKGURL=https://cgit.freedesktop.org/xorg/lib/libXext/snapshot/libXext-1.3.2.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || curl $PKGURL -o ../$PKGDIR.tar.gz || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR \
-include strings.h -DSO_REUSEADDR=1" \
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
PKGURL=https://cgit.freedesktop.org/xorg/lib/libXrender/snapshot/libXrender-0.9.8.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || curl $PKGURL -o ../$PKGDIR.tar.gz || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR \
-include strings.h -DSO_REUSEADDR=1" \
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
PKGURL=https://cgit.freedesktop.org/xorg/lib/libXrandr/snapshot/libXrandr-1.4.2.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || curl $PKGURL -o ../$PKGDIR.tar.gz || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR \
-include strings.h -DSO_REUSEADDR=1" \
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
PKGURL=https://cgit.freedesktop.org/xorg/lib/libxkbfile/snapshot/libxkbfile-1.0.8.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || curl $PKGURL -o ../$PKGDIR.tar.gz || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR \
-include strings.h -DSO_REUSEADDR=1" \
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
ln -sf $PKGDIR/src/.libs/libxkbfile.a ./
for F in $PKGDIR/include/X11/extensions/*.h ; do
ln -sf ../$F X11/extensions/
done
} || exit 1

# =========== xkbcomp binary ==========

[ -e xkbcomp -a -e pie/xkbcomp ] || {
PKGURL=https://cgit.freedesktop.org/xorg/app/xkbcomp/snapshot/xkbcomp-1.2.4.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || curl $PKGURL -o ../$PKGDIR.tar.gz || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR \
-include strings.h -Os -DSO_REUSEADDR=1" \
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
cp -f $PKGDIR/xkbcomp ./
$BUILDDIR/setCrossEnvironment.sh \
sh -c '$STRIP xkbcomp'
} || exit 1

mkdir -p pie

[ -e pie/xkbcomp ] || {
cd pie
tar xvzf ../../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR \
-include strings.h -Os -DSO_REUSEADDR=1" \
LDFLAGS="-pie -L$BUILDDIR" \
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

cd $BUILDDIR/pie
cp -f $PKGDIR/xkbcomp ./
$BUILDDIR/setCrossEnvironment.sh \
sh -c '$STRIP xkbcomp'
cd $BUILDDIR
} || exit 1

# =========== libICE.a ==========

[ -e libICE.a ] || {
PKGURL=https://cgit.freedesktop.org/xorg/lib/libICE/snapshot/libICE-1.0.8.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || curl $PKGURL -o ../$PKGDIR.tar.gz || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

#LIBS="-lxcb -lXau -lXdmcp -landroid_support" \

env CFLAGS="-isystem$BUILDDIR \
-include strings.h -DSO_REUSEADDR=1" \
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
PKGURL=https://cgit.freedesktop.org/xorg/lib/libSM/snapshot/libSM-1.2.2.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || curl $PKGURL -o ../$PKGDIR.tar.gz || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

#LIBS="-lxcb -lXau -lXdmcp -landroid_support" \

env CFLAGS="-isystem$BUILDDIR \
-include strings.h -DSO_REUSEADDR=1" \
LDFLAGS="-L$BUILDDIR" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=$TARGET_HOST \
--prefix=$TARGET_DIR/usr \
--without-libuuid \
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
PKGURL=https://cgit.freedesktop.org/xorg/lib/libXt/snapshot/libXt-1.1.4.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || curl $PKGURL -o ../$PKGDIR.tar.gz || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR \
-include strings.h -DSO_REUSEADDR=1" \
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
PKGURL=https://cgit.freedesktop.org/xorg/lib/libXmu/snapshot/libXmu-1.1.2.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || curl $PKGURL -o ../$PKGDIR.tar.gz || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR \
-include strings.h -DSO_REUSEADDR=1" \
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

[ -e xhost -a -e pie/xhost ] || {
PKGURL=https://cgit.freedesktop.org/xorg/app/xhost/snapshot/xhost-1.0.6.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || curl $PKGURL -o ../$PKGDIR.tar.gz || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR \
-include strings.h \
-Dsethostent=abs -Dendhostent=sync -Os -DSO_REUSEADDR=1" \
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

[ -e pie/xhost ] || {
cd pie
tar xvzf ../../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR \
-include strings.h \
-Dsethostent=abs -Dendhostent=sync -Os -DSO_REUSEADDR=1" \
LDFLAGS="-pie -L$BUILDDIR" \
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

cd $BUILDDIR/pie
cp -f $PKGDIR/xhost ./
$BUILDDIR/setCrossEnvironment.sh \
sh -c '$STRIP xhost'
cd $BUILDDIR
} || exit 1

# =========== xli binary ==========

[ -e xli -a -e pie/xli ] || {
PKGURL=http://web.aanet.com.au/gwg/xli-1.16.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || curl $PKGURL -o ../$PKGDIR.tar.gz || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

echo "SRCS := bright.c clip.c cmuwmrast.c compress.c dither.c faces.c fbm.c \
      fill.c  g3.c gif.c halftone.c imagetypes.c img.c mac.c mcidas.c \
      mc_tables.c merge.c misc.c new.c options.c path.c pbm.c pcx.c \
      reduce.c jpeg.c jpeglib.c rle.c rlelib.c root.c rotate.c send.c smooth.c \
      sunraster.c value.c window.c xbitmap.c xli.c \
      xpixmap.c xwd.c zio.c zoom.c ddxli.c doslib.c tga.c bmp.c pcd.c " > Makefile
echo 'OBJS := $(SRCS:%.c=%.o)' >> Makefile
echo '%.o: %.c' >> Makefile
echo '	$(CC) $(CFLAGS) $(PIE) $(if $(filter compress.c, $+), -O0) -c $+ -o $@' >> Makefile
echo 'xli: $(OBJS)' >> Makefile
echo '	$(CC) $(CFLAGS) $+ -o $@ $(LDFLAGS) $(PIE) -lX11 -lxcb -lXau -lXdmcp -landroid_support -lm' >> Makefile

echo '#include <stdarg.h>' > varargs.h

sed -i 's/extern int errno;//' *.c

env CFLAGS="-isystem$BUILDDIR \
-include strings.h \
-include sys/select.h \
-include math.h \
-include stdlib.h \
-include string.h \
-Dindex=strchr \
-Drindex=strrchr \
-isystem . \
-Os" \
LDFLAGS="-L$BUILDDIR" \
$BUILDDIR/setCrossEnvironment.sh \
make V=1 PIE= 2>&1 || exit 1

cd $BUILDDIR
cp -f $PKGDIR/xli ./
$BUILDDIR/setCrossEnvironment.sh \
sh -c '$STRIP xli'
} || exit 1

[ -e pie/xli ] || {
cd pie
tar xvzf ../../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

cp -f ../../$PKGDIR/Makefile ./
cp -f ../../$PKGDIR/varargs.h ./

sed -i 's/extern int errno;//' *.c

env CFLAGS="-isystem$BUILDDIR \
-include strings.h \
-include sys/select.h \
-include math.h \
-include stdlib.h \
-include string.h \
-Dindex=strchr \
-Drindex=strrchr \
-isystem . \
-Os" \
LDFLAGS="-L$BUILDDIR" \
$BUILDDIR/setCrossEnvironment.sh \
make V=1 PIE=-pie 2>&1 || exit 1

cd $BUILDDIR/pie
cp -f $PKGDIR/xli ./
$BUILDDIR/setCrossEnvironment.sh \
sh -c '$STRIP xli'
cd $BUILDDIR
} || exit 1

# =========== xsel binary ==========

[ -e xsel -a -e pie/xsel ] || {
PKGURL=https://github.com/kfish/xsel/archive/master.tar.gz
PKGDIR=xsel-master
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || curl -L $PKGURL -o ../$PKGDIR.tar.gz || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

env CFLAGS="-isystem$BUILDDIR -Drpl_malloc=malloc -Os" \
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

[ -e pie/xsel ] || {
cd pie
tar xvzf ../../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

env CFLAGS="-isystem$BUILDDIR -Drpl_malloc=malloc -Os" \
LDFLAGS="-pie -L$BUILDDIR" \
LIBS="-lX11 -lxcb -lXau -lXdmcp -landroid_support" \
$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1

$BUILDDIR/setCrossEnvironment.sh \
make V=1 2>&1 || exit 1

cd $BUILDDIR/pie
cp -f $PKGDIR/xsel ./ || exit 1
$BUILDDIR/setCrossEnvironment.sh \
sh -c '$STRIP xsel'
cd $BUILDDIR
} || exit 1

# =========== xsdl ==========

ln -sf $BUILDDIR/../../../../../../libs/$TARGET_ARCH/libsdl-1.2.so $BUILDDIR/libSDL.so
ln -sf $BUILDDIR/libportable.a $BUILDDIR/libpthread.a # dummy
ln -sf $BUILDDIR/libportable.a $BUILDDIR/libts.a # dummy

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
	-DSO_REUSEADDR=1 \
	-Dipc_perm=debian_ipc_perm \
	-I$BUILDDIR/pixman-0.30.2/pixman \
	-I$BUILDDIR/../../../../../../jni/sdl-1.2/include \
	-I$BUILDDIR/../../../../../../jni/crypto/include" \
LDFLAGS="-L$BUILDDIR -L$BUILDDIR/../../../../../../jni/crypto/lib-$TARGET_ARCH" \
./setCrossEnvironment.sh \
LIBS="-lfontenc -lfreetype -llog -lSDL -lGLESv1_CM -landroid-shmem -l:libcrypto.so.sdl.0.so" \
OPENSSL_LIBS=-l:libcrypto.so.sdl.0.so \
LIBSHA1_LIBS=-l:libcrypto.so.sdl.0.so \
../../configure \
--host=$TARGET_HOST \
--prefix=$TARGET_DIR/usr \
--with-xkb-output=$TARGET_DIR/tmp \
--disable-xorg --disable-dmx --disable-xvfb --disable-xnest --disable-xquartz --disable-xwin \
--disable-xephyr --disable-xfake --disable-xfbdev --disable-unit-tests --disable-tslib \
--disable-dri --disable-dri2 --disable-glx --disable-xf86vidmode \
--enable-xsdl --enable-kdrive --enable-kdrive-kbd --enable-kdrive-mouse --enable-kdrive-evdev \
--enable-shm --enable-mitshm --disable-config-udev \
|| exit 1

./setCrossEnvironment.sh make -j$NCPU V=1 2>&1 || exit 1

exit 0
