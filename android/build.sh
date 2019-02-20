#!/bin/sh

set -x

export BUILDDIR=`pwd`

[ -e $BUILDDIR/setCrossEnvironment.sh ] || {
	echo "Launch build.sh from arch-specific directory:"
	echo "cd armeabi-v7a ; ./build.sh"
	exit 1
}

NCPU=4
uname -s | grep -i "linux" && NCPU=`cat /proc/cpuinfo | grep -c -i processor`

NDK=`which ndk-build`
NDK=`dirname $NDK`
NDK=`readlink -f $NDK`

[ -z "$TARGET_ARCH" ] && TARGET_ARCH=armeabi-v7a
[ -z "$TARGET_HOST" ] && TARGET_HOST=arm-linux-androideabi
[ -z "$TARGET_DIR" ] && TARGET_DIR=/proc/self/cwd

AR=`$BUILDDIR/setCrossEnvironment.sh sh -c 'echo $AR'`
echo AR=$AR

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


# =========== xorgproto ===========

[ -e X11/Xfuncproto.h ] || {
PKGURL=https://cgit.freedesktop.org/xorg/proto/xorgproto/snapshot/xorgproto-2018.4.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1

cd $PKGDIR
patch -p0 < ../../xproto.diff || exit 1
$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST --prefix=$BUILDDIR/usr \
|| exit 1
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 install 2>&1 || exit 1
cd $BUILDDIR
ln -sf $BUILDDIR/usr/include/X11 ./
} || exit 1

if false; then # Old proto libraries are now all merged into xorgproto
# =========== xproto ===========

[ -e X11/Xfuncproto.h ] || {
PKGURL=https://cgit.freedesktop.org/xorg/proto/x11proto/snapshot/xproto-7.0.31.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1

#ln -sf $PKGDIR X11
#cd X11
cd $PKGDIR
patch -p0 < ../../xproto.diff || exit 1
$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST --prefix=$BUILDDIR/usr \
|| exit 1
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 install 2>&1 || exit 1
cd $BUILDDIR
ln -sf $BUILDDIR/usr/include/X11 ./
} || exit 1

# =========== fontsproto ===========

[ -e X11/fonts/fontstruct.h ] || {
PKGURL=https://cgit.freedesktop.org/xorg/proto/fontsproto/snapshot/fontsproto-2.1.3.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1

#ln -sf ../$PKGDIR X11/fonts
#cd X11/fonts
cd $PKGDIR
$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST --prefix=$BUILDDIR/usr \
|| exit 1
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 install 2>&1 || exit 1
cd $BUILDDIR
} || exit 1

# =========== xextproto ===========

[ -e X11/extensions/geproto.h ] || {
PKGURL=https://cgit.freedesktop.org/xorg/proto/xextproto/snapshot/xextproto-7.3.0.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1

#ln -sf ../$PKGDIR X11/extensions
#cd X11/extensions
cd $PKGDIR
$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST --prefix=$BUILDDIR/usr \
|| exit 1
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 install 2>&1 || exit 1
cd $BUILDDIR
} || exit 1

# =========== inputproto ===========

[ -e X11/extensions/XI.h ] || {
PKGURL=https://cgit.freedesktop.org/xorg/proto/inputproto/snapshot/inputproto-2.3.2.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST --prefix=$BUILDDIR/usr \
|| exit 1
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 install 2>&1 || exit 1
cd $BUILDDIR
#for F in $PKGDIR/*.h ; do
#ln -sf ../$F X11/extensions/
#done
} || exit 1

# =========== kbproto ===========

[ -e X11/extensions/XKBproto.h ] || {
PKGURL=https://cgit.freedesktop.org/xorg/proto/kbproto/snapshot/kbproto-1.0.7.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST --prefix=$BUILDDIR/usr \
|| exit 1
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 install 2>&1 || exit 1
cd $BUILDDIR
#for F in $PKGDIR/*.h ; do
#ln -sf ../$F X11/extensions/
#done
} || exit 1

# =========== xineramaproto ===========

[ -e X11/extensions/panoramiXproto.h ] || {
PKGURL=https://cgit.freedesktop.org/xorg/proto/xineramaproto/snapshot/xineramaproto-1.2.1.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST --prefix=$BUILDDIR/usr \
|| exit 1
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 install 2>&1 || exit 1
cd $BUILDDIR
#for F in $PKGDIR/*.h ; do
#ln -sf ../$F X11/extensions/
#done
} || exit 1

# =========== renderproto ===========

[ -e X11/extensions/renderproto.h ] || {
PKGURL=https://cgit.freedesktop.org/xorg/proto/renderproto/snapshot/renderproto-0.11.1.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST --prefix=$BUILDDIR/usr \
|| exit 1
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 install 2>&1 || exit 1
cd $BUILDDIR
#for F in $PKGDIR/*.h ; do
#ln -sf ../$F X11/extensions/
#done
} || exit 1

# =========== xfixesproto ===========

[ -e X11/extensions/xfixesproto.h ] || {
PKGURL=https://cgit.freedesktop.org/xorg/proto/fixesproto/snapshot/fixesproto-5.0.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST --prefix=$BUILDDIR/usr \
|| exit 1
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 install 2>&1 || exit 1
cd $BUILDDIR
#for F in $PKGDIR/*.h ; do
#ln -sf ../$F X11/extensions/
#done
} || exit 1

# =========== damageproto ===========

[ -e X11/extensions/damageproto.h ] || {
PKGURL=https://cgit.freedesktop.org/xorg/proto/damageproto/snapshot/damageproto-1.2.1.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST --prefix=$BUILDDIR/usr \
|| exit 1
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 install 2>&1 || exit 1
cd $BUILDDIR
#for F in $PKGDIR/*.h ; do
#ln -sf ../$F X11/extensions/
#done
} || exit 1

# =========== compositeproto ===========

[ -e X11/extensions/compositeproto.h ] || {
PKGURL=https://cgit.freedesktop.org/xorg/proto/compositeproto/snapshot/compositeproto-0.4.2.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST --prefix=$BUILDDIR/usr \
|| exit 1
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 install 2>&1 || exit 1
cd $BUILDDIR
#for F in $PKGDIR/*.h ; do
#ln -sf ../$F X11/extensions/
#done
} || exit 1

# =========== bigreqsproto ===========

[ -e X11/extensions/bigreqsproto.h ] || {
PKGURL=https://cgit.freedesktop.org/xorg/proto/bigreqsproto/snapshot/bigreqsproto-1.1.2.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST --prefix=$BUILDDIR/usr \
|| exit 1
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 install 2>&1 || exit 1
cd $BUILDDIR
#for F in $PKGDIR/*.h ; do
#ln -sf ../$F X11/extensions/
#done
} || exit 1

# =========== scrnsaverproto ===========

[ -e X11/extensions/saver.h ] || {
PKGURL=https://cgit.freedesktop.org/xorg/proto/scrnsaverproto/snapshot/scrnsaverproto-1.2.2.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST --prefix=$BUILDDIR/usr \
|| exit 1
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 install 2>&1 || exit 1
cd $BUILDDIR
#for F in $PKGDIR/*.h ; do
#ln -sf ../$F X11/extensions/
#done
} || exit 1

# =========== videoproto ===========

[ -e X11/extensions/Xv.h ] || {
PKGURL=https://cgit.freedesktop.org/xorg/proto/videoproto/snapshot/videoproto-2.3.3.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST --prefix=$BUILDDIR/usr \
|| exit 1
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 install 2>&1 || exit 1
cd $BUILDDIR
#for F in $PKGDIR/*.h ; do
#ln -sf ../$F X11/extensions/
#done
} || exit 1

# =========== resourceproto ===========

[ -e X11/extensions/XResproto.h ] || {
PKGURL=https://cgit.freedesktop.org/xorg/proto/resourceproto/snapshot/resourceproto-1.2.0.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST --prefix=$BUILDDIR/usr \
|| exit 1
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 install 2>&1 || exit 1
cd $BUILDDIR
#for F in $PKGDIR/*.h ; do
#ln -sf ../$F X11/extensions/
#done
} || exit 1

# =========== xcmiscproto ===========

[ -e X11/extensions/xcmiscproto.h ] || {
PKGURL=https://cgit.freedesktop.org/xorg/proto/xcmiscproto/snapshot/xcmiscproto-1.2.2.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST --prefix=$BUILDDIR/usr \
|| exit 1
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 install 2>&1 || exit 1
cd $BUILDDIR
#for F in $PKGDIR/*.h ; do
#ln -sf ../$F X11/extensions/
#done
} || exit 1

# =========== randrproto ===========

[ -e X11/extensions/randr.h ] || {
PKGURL=https://cgit.freedesktop.org/xorg/proto/randrproto/snapshot/randrproto-1.5.0.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST --prefix=$BUILDDIR/usr \
|| exit 1
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 install 2>&1 || exit 1
cd $BUILDDIR
#for F in $PKGDIR/*.h ; do
#ln -sf ../$F X11/extensions/
#done
} || exit 1

# =========== xf86bigfontproto ===========

[ -e X11/extensions/xf86bigfproto.h ] || {
PKGURL=https://cgit.freedesktop.org/xorg/proto/xf86bigfontproto/snapshot/xf86bigfontproto-1.2.0.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST --prefix=$BUILDDIR/usr \
|| exit 1
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 install 2>&1 || exit 1
cd $BUILDDIR
#for F in $PKGDIR/*.h ; do
#ln -sf ../$F X11/extensions/
#done
} || exit 1

# =========== recordproto ===========

[ -e X11/extensions/recordproto.h ] || {
PKGURL=https://cgit.freedesktop.org/xorg/proto/recordproto/snapshot/recordproto-1.14.2.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST --prefix=$BUILDDIR/usr \
|| exit 1
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 install 2>&1 || exit 1
cd $BUILDDIR
#for F in $PKGDIR/*.h ; do
#ln -sf ../$F X11/extensions/
#done
} || exit 1

fi # Old proto libraries are now all merged into xorgproto

# =========== xtrans ===========

[ -e xtrans-1.3.5 ] || {
PKGURL=https://cgit.freedesktop.org/xorg/lib/libxtrans/snapshot/xtrans-1.3.5.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

patch -p0 < ../../xtrans.diff || exit 1

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR/usr/include -include strings.h" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=$TARGET_HOST --prefix=$BUILDDIR/usr \
|| exit 1

$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 install 2>&1 || exit 1

cd $BUILDDIR
#ln -sf ../$PKGDIR X11/Xtrans
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
PKGURL=https://cairographics.org/releases/pixman-0.38.0.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

aclocal
automake --add-missing
autoreconf -f

env CFLAGS="-I$NDK/sources/android/cpufeatures" \
LDFLAGS="-L$BUILDDIR -lportable" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=$TARGET_HOST \
--disable-arm-iwmmxt \
--enable-static \
--prefix=$BUILDDIR/usr \
|| exit 1

sed -i "s/TOOLCHAIN_SUPPORTS_ATTRIBUTE_CONSTRUCTOR/DISABLE_TOOLCHAIN_SUPPORTS_ATTRIBUTE_CONSTRUCTOR/g" config.h

cd pixman
touch *.S

$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 2>&1 || exit 1
touch $BUILDDIR/$PKGDIR/pixman/.libs/libpixman-1.so
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 install install-am 2>&1 || exit 1

cd $BUILDDIR
#ln -sf $BUILDDIR/$PKGDIR/pixman/.libs/libpixman-1.a $BUILDDIR/libpixman-1.a
$AR rcs $BUILDDIR/libpixman-1.a $BUILDDIR/$PKGDIR/pixman/.libs/*.o || exit 1

} || exit 1

# =========== libfontenc.a ===========

[ -e libfontenc.a ] || {
PKGURL=https://cgit.freedesktop.org/xorg/lib/libfontenc/snapshot/libfontenc-1.1.3.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR/usr/include -include strings.h" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=$TARGET_HOST \
--prefix=$BUILDDIR/usr \
--enable-static \
|| exit 1

#cp -f `which libtool` ./

$BUILDDIR/setCrossEnvironment.sh \
sh -c 'ln -sf $CC gcc'

env PATH=`pwd`:$PATH \
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 2>&1 || exit 1
touch src/.libs/libfontenc.so
env PATH=`pwd`:$PATH \
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 install 2>&1 || exit 1

cd $BUILDDIR
#ln -sf ../$PKGDIR/include/X11/fonts/fontenc.h X11/fonts/
#ln -sf $PKGDIR/src/.libs/libfontenc.a ./
$AR rcs libfontenc.a $PKGDIR/src/.libs/*.o || exit 1
} || exit 1

# =========== libXfont.a ===========

ln -sf $BUILDDIR/../../../../../../obj/local/$TARGET_ARCH/libfreetype.a $BUILDDIR/

# =========== libXfont2.a ===========

[ -e libXfont2.a ] || {
PKGURL=https://cgit.freedesktop.org/xorg/lib/libXfont/snapshot/libXfont2-2.0.3.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR/usr/include \
-include strings.h \
-I$BUILDDIR/../../../../../../jni/freetype/include \
-DNO_LOCALE -DOPEN_MAX=256" \
LDFLAGS="-L$BUILDDIR" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=$TARGET_HOST \
--prefix=$BUILDDIR/usr \
--enable-static \
|| exit 1

$BUILDDIR/setCrossEnvironment.sh \
sh -c 'ln -sf $CC gcc'

env PATH=`pwd`:$PATH \
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 2>&1 || exit 1
touch .libs/libXfont2.so
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 install 2>&1 || exit 1

cd $BUILDDIR
$AR rcs libXfont2.a $PKGDIR/src/*/.libs/*.o
} || exit 1

# =========== libXau.a ==========

[ -e libXau.a ] || {
PKGURL=https://cgit.freedesktop.org/xorg/lib/libXau/snapshot/libXau-1.0.9.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR/usr/include \
-include strings.h" \
LDFLAGS="-L$BUILDDIR" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=$TARGET_HOST \
--prefix=$BUILDDIR/usr \
|| exit 1

#cp -f `which libtool` ./

$BUILDDIR/setCrossEnvironment.sh \
sh -c 'ln -sf $CC gcc'

env PATH=`pwd`:$PATH \
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 2>&1 || exit 1
touch .libs/libXau.so
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 install 2>&1 || exit 1

cd $BUILDDIR
#ln -sf $PKGDIR/.libs/libXau.a ./
$AR rcs libXau.a $PKGDIR/.libs/*.o

#ln -sf ../$PKGDIR/include/X11/Xauth.h X11/
} || exit 1

# =========== libXdmcp.a ==========

[ -e libXdmcp.a ] || {
PKGURL=https://cgit.freedesktop.org/xorg/lib/libXdmcp/snapshot/libXdmcp-1.1.2.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR/usr/include \
-include strings.h" \
LDFLAGS="-L$BUILDDIR" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=$TARGET_HOST \
--prefix=$BUILDDIR/usr \
|| exit 1

#cp -f `which libtool` ./

$BUILDDIR/setCrossEnvironment.sh \
sh -c 'ln -sf $CC gcc'

env PATH=`pwd`:$PATH \
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 2>&1 || exit 1
touch .libs/libXdmcp.so
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 install 2>&1 || exit 1

cd $BUILDDIR
#ln -sf $PKGDIR/.libs/libXdmcp.a ./
$AR rcs libXdmcp.a $PKGDIR/.libs/*.o
#ln -sf ../$PKGDIR/include/X11/Xdmcp.h X11/
} || exit 1

# =========== xcbproto ===========
[ -e usr/lib/pkgconfig/xcb-proto.pc ] || {
PKGURL=https://xcb.freedesktop.org/dist/xcb-proto-1.13.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST --prefix=$BUILDDIR/usr \
|| exit 1
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 install 2>&1 || exit 1
cd $BUILDDIR
} || exit 1

# =========== libxcb.a ==========

[ -e libxcb.a ] || {
PKGURL=https://xcb.freedesktop.org/dist/libxcb-1.13.1.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR/usr/include \
-include strings.h" \
LDFLAGS="-L$BUILDDIR" \
PKG_CONFIG_PATH=$BUILDDIR/usr/lib/pkgconfig \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=$TARGET_HOST \
--prefix=$BUILDDIR/usr \
|| exit 1

#cp -f `which libtool` ./

$BUILDDIR/setCrossEnvironment.sh \
sh -c 'ln -sf $CC gcc'

sed -i 's/[$]MV [$]realname [$][{]realname[}]U/$CP $realname ${realname}U/g' ./libtool

env PATH=`pwd`:$PATH \
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 2>&1 || exit 1

for f in src/.libs/*.la; do
touch "`echo $f | sed 's/[.]la$/.so/'`"
done

$BUILDDIR/setCrossEnvironment.sh \
make -j1 V=1 install 2>&1 || exit 1

cd $BUILDDIR
#ln -sf $PKGDIR/src/.libs/libxcb.a ./
$AR rcs libxcb.a $PKGDIR/src/.libs/*.o
} || exit 1

# =========== libandroid_support.a ==========

[ -e libandroid_support.a ] || {
if echo $TARGET_ARCH | grep '64'; then
$AR rcs libandroid_support.a
else
ln -s $NDK/sources/cxx-stl/llvm-libc++/libs/TARGET_ARCH/libandroid_support.a ./ || exit 1
fi
cd $BUILDDIR
} || exit 1

# =========== libX11.a ==========

[ -e libX11.a ] || {
PKGURL=https://cgit.freedesktop.org/xorg/lib/libX11/snapshot/libX11-1.6.7.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

patch -p0 < ../../libX11.diff || exit 1

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR/usr/include \
			-isystem$BUILDDIR/../android-shmem \
			-include strings.h" \
LDFLAGS="-L$BUILDDIR" \
$BUILDDIR/setCrossEnvironment.sh \
LIBS="-lXau -lXdmcp -landroid_support -landroid-shmem" \
./configure \
--host=$TARGET_HOST \
--prefix=$TARGET_DIR/usr \
|| exit 1

$BUILDDIR/setCrossEnvironment.sh \
sh -c 'ln -sf $CC gcc'

echo "all install: makekeys" > src/util/Makefile
echo "makekeys:" >> src/util/Makefile
echo "	/usr/bin/gcc makekeys.c -o makekeys -I /usr/include" >> src/util/Makefile

env PATH=`pwd`:$PATH \
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 2>&1 || exit 1

ln -sf ../usr ./
ln -sf ../usr ./src/
ln -sf ../usr ./src/util/
ln -sf ../usr ./src/xcms/
ln -sf ../usr ./src/xkb/
ln -sf ../usr ./include/
ln -sf ../usr ./nls/
ln -sf ../usr ./man/
ln -sf ../usr ./man/xkb/
ln -sf ../usr ./specs/
ln -sf ../usr ./specs/XIM/
ln -sf ../usr ./specs/XKB/
ln -sf ../usr ./specs/libX11/
ln -sf ../usr ./specs/i18n/
ln -sf ../usr ./specs/i18n/compose/
ln -sf ../usr ./specs/i18n/framework/
ln -sf ../usr ./specs/i18n/localedb/
ln -sf ../usr ./specs/i18n/trans/

mkdir -p ./usr/share/X11
mkdir -p ./usr/share/X11/locale
mkdir -p ./usr/share/man/man3
mkdir -p ./usr/share/man/man5
mkdir -p ./usr/share/doc/libX11/XIM
mkdir -p ./usr/share/doc/libX11/XKB
mkdir -p ./usr/share/doc/libX11/libX11
mkdir -p ./usr/share/doc/libX11/i18n/compose
mkdir -p ./usr/share/doc/libX11/i18n/framework
mkdir -p ./usr/share/doc/libX11/i18n/localedb
mkdir -p ./usr/share/doc/libX11/i18n/trans

cd nls
for f in *; do
[ -d $f ] && mkdir -p ./usr/share/X11/locale/$f
done
cd ..

touch src/.libs/libX11.so
touch src/.libs/libX11-xcb.so
touch src/.libs/libX11-i18n.so
touch src/.libs/libX11-xcms.so
touch src/.libs/libX11-xkb.so

$BUILDDIR/setCrossEnvironment.sh \
make -j1 V=1 install install-am "MKDIR_P=test -d" 2>&1 || exit 1

cd $BUILDDIR
#for F in $PKGDIR/include/X11/*.h ; do
#ln -sf ../$F X11
#done

#ln -sf $PKGDIR/src/.libs/libX11.a ./
rm -f $PKGDIR/src/.libs/x11_xcb.o
$AR rcs libX11-core.a $PKGDIR/src/.libs/*.o
ln -sf $PKGDIR/src/xlibi18n/.libs/libi18n.a ./libX11-i18n.a
ln -sf $PKGDIR/src/xcms/.libs/libxcms.a ./libX11-xcms.a
ln -sf $PKGDIR/src/xkb/.libs/libxkb.a ./libX11-xkb.a

$AR -M <<EOF
CREATE libX11.a
ADDLIB libX11-core.a
ADDLIB libX11-i18n.a
ADDLIB libX11-xcms.a
ADDLIB libX11-xkb.a
SAVE
END
EOF

$AR s libX11.a
} || exit 1

# =========== libXext.a ==========

[ -e libXext.a ] || {
PKGURL=https://cgit.freedesktop.org/xorg/lib/libXext/snapshot/libXext-1.3.3.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR/usr/include \
-include strings.h" \
LDFLAGS="-L$BUILDDIR" \
LIBS="-lxcb -lXau -lXdmcp -landroid_support" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=$TARGET_HOST \
--prefix=$BUILDDIR/usr \
|| exit 1

#cp -f `which libtool` ./

$BUILDDIR/setCrossEnvironment.sh \
sh -c 'ln -sf $CC gcc'

env PATH=`pwd`:$PATH \
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 2>&1 || exit 1
touch src/.libs/libXext.so
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 install 2>&1 || exit 1

cd $BUILDDIR
#ln -sf $PKGDIR/src/.libs/libXext.a ./
$AR rcs libXext.a $PKGDIR/src/.libs/*.o
#for F in $PKGDIR/include/X11/extensions/*.h ; do
#ln -sf ../$F X11/extensions/
#done
} || exit 1

# =========== libXrender.a ==========

[ -e libXrender.a ] || {
PKGURL=https://cgit.freedesktop.org/xorg/lib/libXrender/snapshot/libXrender-0.9.10.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR/usr/include \
-include strings.h" \
LDFLAGS="-L$BUILDDIR" \
LIBS="-lxcb -lXau -lXdmcp -landroid_support" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=$TARGET_HOST \
--prefix=$BUILDDIR/usr \
|| exit 1

#cp -f `which libtool` ./

$BUILDDIR/setCrossEnvironment.sh \
sh -c 'ln -sf $CC gcc'

env PATH=`pwd`:$PATH \
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 2>&1 || exit 1
touch src/.libs/libXrender.so
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 install 2>&1 || exit 1

cd $BUILDDIR
#ln -sf $PKGDIR/src/.libs/libXrender.a ./
$AR rcs libXrender.a $PKGDIR/src/.libs/*.o
#for F in $PKGDIR/include/X11/extensions/*.h ; do
#ln -sf ../$F X11/extensions/
#done
} || exit 1

# =========== libXrandr.a ==========

[ -e libXrandr.a ] || {
PKGURL=https://cgit.freedesktop.org/xorg/lib/libXrandr/snapshot/libXrandr-1.5.1.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR/usr/include \
-include strings.h" \
LDFLAGS="-L$BUILDDIR" \
LIBS="-lxcb -lXau -lXdmcp -landroid_support" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=$TARGET_HOST \
--prefix=$BUILDDIR/usr \
|| exit 1

#cp -f `which libtool` ./

$BUILDDIR/setCrossEnvironment.sh \
sh -c 'ln -sf $CC gcc'

env PATH=`pwd`:$PATH \
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 2>&1 || exit 1
touch src/.libs/libXrandr.so
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 install 2>&1 || exit 1

cd $BUILDDIR
#ln -sf $PKGDIR/src/.libs/libXrandr.a ./
$AR rcs libXrandr.a $PKGDIR/src/.libs/*.o
#for F in $PKGDIR/include/X11/extensions/*.h ; do
#ln -sf ../$F X11/extensions/
#done
} || exit 1

# =========== libxkbfile.a ==========

[ -e libxkbfile.a ] || {
PKGURL=https://cgit.freedesktop.org/xorg/lib/libxkbfile/snapshot/libxkbfile-1.0.9.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR/usr/include \
-include strings.h" \
LDFLAGS="-L$BUILDDIR" \
LIBS="-lxcb -lXau -lXdmcp -landroid_support" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=$TARGET_HOST \
--prefix=$BUILDDIR/usr \
|| exit 1

#cp -f `which libtool` ./

$BUILDDIR/setCrossEnvironment.sh \
sh -c 'ln -sf $CC gcc'

env PATH=`pwd`:$PATH \
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 2>&1 || exit 1
touch src/.libs/libxkbfile.so
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 install 2>&1 || exit 1

cd $BUILDDIR
#ln -sf $PKGDIR/src/.libs/libxkbfile.a ./
$AR rcs libxkbfile.a $PKGDIR/src/.libs/*.o
#for F in $PKGDIR/include/X11/extensions/*.h ; do
#ln -sf ../$F X11/extensions/
#done
} || exit 1

# =========== xkbcomp binary ==========

[ -e xkbcomp ] || {
PKGURL=https://cgit.freedesktop.org/xorg/app/xkbcomp/snapshot/xkbcomp-1.4.2.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR/usr/include \
-include strings.h -Os" \
LDFLAGS="-pie -L$BUILDDIR" \
LIBS="-lxcb -lXau -lXdmcp -landroid_support -lX11 -landroid-shmem" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=$TARGET_HOST \
--prefix=$TARGET_DIR/usr \
|| exit 1

#cp -f `which libtool` ./

$BUILDDIR/setCrossEnvironment.sh \
sh -c 'ln -sf $CC gcc'

env PATH=`pwd`:$PATH \
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 2>&1 || exit 1

cd $BUILDDIR
cp -f $PKGDIR/xkbcomp ./
$BUILDDIR/setCrossEnvironment.sh \
sh -c '$STRIP xkbcomp'

cd $BUILDDIR
} || exit 1

# =========== libICE.a ==========

[ -e libICE.a ] || {
PKGURL=https://cgit.freedesktop.org/xorg/lib/libICE/snapshot/libICE-1.0.9.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

#LIBS="-lxcb -lXau -lXdmcp -landroid_support" \

env CFLAGS="-isystem$BUILDDIR/usr/include \
-include strings.h" \
LDFLAGS="-L$BUILDDIR" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=$TARGET_HOST \
--prefix=$BUILDDIR/usr \
|| exit 1

#cp -f `which libtool` ./

$BUILDDIR/setCrossEnvironment.sh \
sh -c 'ln -sf $CC gcc'

env PATH=`pwd`:$PATH \
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 2>&1 || exit 1
touch src/.libs/libICE.so
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 install 2>&1 || exit 1

cd $BUILDDIR
#ln -sf $PKGDIR/src/.libs/libICE.a ./
$AR rcs libICE.a $PKGDIR/src/.libs/*.o

#ln -sf ../$PKGDIR/include/X11/ICE X11/
} || exit 1

# =========== libSM.a ==========

[ -e libSM.a ] || {
PKGURL=https://cgit.freedesktop.org/xorg/lib/libSM/snapshot/libSM-1.2.3.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

#LIBS="-lxcb -lXau -lXdmcp -landroid_support" \

env CFLAGS="-isystem$BUILDDIR/usr/include \
-include strings.h" \
LDFLAGS="-L$BUILDDIR" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=$TARGET_HOST \
--prefix=$BUILDDIR/usr \
--without-libuuid \
|| exit 1

#cp -f `which libtool` ./

$BUILDDIR/setCrossEnvironment.sh \
sh -c 'ln -sf $CC gcc'

env PATH=`pwd`:$PATH \
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 2>&1 || exit 1
touch src/.libs/libSM.so
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 install 2>&1 || exit 1

cd $BUILDDIR
#ln -sf $PKGDIR/src/.libs/libSM.a ./
$AR rcs libSM.a $PKGDIR/src/.libs/*.o

#ln -sf ../$PKGDIR/include/X11/SM X11/
} || exit 1

# =========== libXt.a ==========

[ -e libXt.a ] || {
PKGURL=https://cgit.freedesktop.org/xorg/lib/libXt/snapshot/libXt-1.1.5.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR/usr/include \
-include strings.h" \
LDFLAGS="-L$BUILDDIR" \
LIBS="-lxcb -lXau -lXdmcp -landroid_support" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=$TARGET_HOST \
--prefix=$TARGET_DIR/usr \
|| exit 1

$BUILDDIR/setCrossEnvironment.sh \
sh -c 'ln -sf $CC gcc'

echo "all install: makestrs" > util/Makefile
echo "makestrs:" >> util/Makefile
echo "	/usr/bin/gcc makestrs.c -o makestrs -I /usr/include" >> util/Makefile

env PATH=`pwd`:$PATH \
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 2>&1 || exit 1

ln -sf ../usr ./
ln -sf ../usr ./src/
ln -sf ../usr ./include/
ln -sf ../usr ./man/
ln -sf ../usr ./specs/
mkdir -p ./usr/share/doc/libXt

touch src/.libs/libXt.so

$BUILDDIR/setCrossEnvironment.sh \
make -j1 V=1 install "MKDIR_P=test -d" 2>&1 || exit 1

cd $BUILDDIR
#ln -sf $PKGDIR/src/.libs/libXt.a ./
$AR rcs libXt.a $PKGDIR/src/.libs/*.o

#for F in $PKGDIR/include/X11/*.h ; do
#ln -sf ../$F X11/
#done
} || exit 1

# =========== libXmuu.a ==========

[ -e libXmuu.a ] || {
PKGURL=https://cgit.freedesktop.org/xorg/lib/libXmu/snapshot/libXmu-1.1.2.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR/usr/include \
-include strings.h" \
LDFLAGS="-L$BUILDDIR" \
LIBS="-lxcb -lXau -lXdmcp -landroid_support -lSM -lICE" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=$TARGET_HOST \
--prefix=$TARGET_DIR/usr \
|| exit 1

$BUILDDIR/setCrossEnvironment.sh \
sh -c 'ln -sf $CC gcc'

env PATH=`pwd`:$PATH \
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 2>&1 || exit 1

ln -sf ../usr ./
ln -sf ../usr ./src/
ln -sf ../usr ./include/
ln -sf ../usr ./doc/
mkdir -p ./usr/include/X11/Xmu
mkdir -p ./usr/share/doc/libXmu

touch src/.libs/libXmuu.so
touch src/.libs/libXmu.so

$BUILDDIR/setCrossEnvironment.sh \
make -j1 V=1 install "MKDIR_P=test -d" 2>&1 || exit 1

cd $BUILDDIR
#ln -sf $PKGDIR/src/.libs/libXmuu.a ./
#ln -sf $PKGDIR/src/.libs/libXmu.a ./
$AR rcs libXmuu.a $PKGDIR/src/.libs/*.o
$AR rcs libXmu.a

#ln -sf ../$PKGDIR/include/X11/Xmu X11/
} || exit 1

# =========== libxshmfence.a ==========

[ -e libxshmfence.a ] || {
PKGURL=https://cgit.freedesktop.org/xorg/lib/libxshmfence/snapshot/libxshmfence-1.3.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

patch -p0 < ../../xshmfence.diff || exit 1

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

mkdir tmp

env CFLAGS="-isystem$BUILDDIR/usr/include \
-include limits.h \
-DMAXINT=INT_MAX" \
LDFLAGS="-L$BUILDDIR" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=$TARGET_HOST \
--prefix=$BUILDDIR/usr \
--with-shared-memory-dir=/proc/self/cwd/tmp \
|| exit 1

env PATH=`pwd`:$PATH \
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 2>&1 || exit 1
touch src/.libs/libxshmfence.so
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 install 2>&1 || exit 1

cd $BUILDDIR
$AR rcs libxshmfence.a $PKGDIR/src/.libs/*.o
} || exit 1

# =========== xhost binary ==========

[ -e xhost ] || {
PKGURL=https://cgit.freedesktop.org/xorg/app/xhost/snapshot/xhost-1.0.7.tar.gz
PKGDIR=`basename --suffix=.tar.gz $PKGURL`
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

[ -e configure ] || \
autoreconf -v --install \
|| exit 1

env CFLAGS="-isystem$BUILDDIR/usr/include \
-include strings.h \
-Dsethostent=abs -Dendhostent=sync -Os" \
LDFLAGS="-pie -L$BUILDDIR" \
LIBS="-lxcb -lXau -lXdmcp -landroid_support -lX11 -landroid-shmem" \
$BUILDDIR/setCrossEnvironment.sh \
./configure \
--host=$TARGET_HOST \
--prefix=$TARGET_DIR/usr \
|| exit 1

#cp -f `which libtool` ./

$BUILDDIR/setCrossEnvironment.sh \
sh -c 'ln -sf $CC gcc'

env PATH=`pwd`:$PATH \
$BUILDDIR/setCrossEnvironment.sh \
make -j$NCPU V=1 2>&1 || exit 1

cd $BUILDDIR
cp -f $PKGDIR/xhost ./
$BUILDDIR/setCrossEnvironment.sh \
sh -c '$STRIP xhost'

cd $BUILDDIR
} || exit 1

# =========== xli binary ==========

[ -e xli ] || {
PKGURL=https://deb.debian.org/debian/pool/main/x/xli/xli_1.17.0+20061110.orig.tar.gz
#PKGDIR=`basename --suffix=.tar.gz $PKGURL`
PKGDIR=xli-2006-11-10
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

echo "SRCS := bright.c clip.c cmuwmrast.c compress.c dither.c faces.c fbm.c fill.c  g3.c gif.c halftone.c \
				imagetypes.c img.c mac.c mcidas.c mc_tables.c merge.c misc.c new.c options.c path.c pbm.c \
				pcx.c reduce.c jpeg.c rle.c rlelib.c root.c rotate.c send.c smooth.c sunraster.c \
				value.c window.c xbitmap.c xli.c xpixmap.c xwd.c zio.c zoom.c ddxli.c tga.c bmp.c pcd.c png.c" > Makefile
echo 'OBJS := $(SRCS:%.c=%.o)' >> Makefile
echo '%.o: %.c' >> Makefile
echo '	$(CC) $(CFLAGS) $(PIE) $(if $(filter compress.c, $+), -O0) -c $+ -o $@' >> Makefile
echo 'xli: $(OBJS)' >> Makefile
echo '	$(CC) $(CFLAGS) $+ -o $@ $(LDFLAGS) $(PIE) -lX11 -lxcb -lXau -lXdmcp -lXext -ljpeg -lpng -landroid_support -landroid-shmem -lm -lz' >> Makefile

echo '#include <stdarg.h>' > varargs.h

sed -i 's/extern int errno;//' *.c

env CFLAGS="-isystem$BUILDDIR/usr/include \
-include strings.h \
-include sys/select.h \
-include math.h \
-include stdlib.h \
-include string.h \
-Dindex=strchr \
-Drindex=strrchr \
-isystem . \
-isystem $BUILDDIR/../../../../../../jni/jpeg/include \
-isystem $BUILDDIR/../../../../../../jni/png/include \
-Os" \
LDFLAGS="-L$BUILDDIR \
-L$BUILDDIR/../../../../../../obj/local/$TARGET_ARCH" \
$BUILDDIR/setCrossEnvironment.sh \
make V=1 PIE=-pie 2>&1 || exit 1

cd $BUILDDIR
cp -f $PKGDIR/xli ./
$BUILDDIR/setCrossEnvironment.sh \
sh -c '$STRIP xli'

cd $BUILDDIR
} || exit 1

# =========== xsel binary ==========

[ -e xsel ] || {
PKGURL=https://github.com/kfish/xsel/archive/master.tar.gz
PKGDIR=xsel-master
echo $PKGDIR: $PKGURL
[ -e ../$PKGDIR.tar.gz ] || { curl -L $PKGURL -o $PKGDIR.tar.gz && mv $PKGDIR.tar.gz ../ ; } || rm ../$PKGDIR.tar.gz
tar xvzf ../$PKGDIR.tar.gz || exit 1
cd $PKGDIR

env CFLAGS="-isystem$BUILDDIR/usr/include -Drpl_malloc=malloc -Os" \
LDFLAGS="-pie -L$BUILDDIR" \
LIBS="-lX11 -lxcb -lXau -lXdmcp -landroid_support -landroid-shmem" \
$BUILDDIR/setCrossEnvironment.sh \
./autogen.sh --host=$TARGET_HOST \
|| exit 1

$BUILDDIR/setCrossEnvironment.sh \
make V=1 2>&1 || exit 1

cd $BUILDDIR
cp -f $PKGDIR/xsel ./ || exit 1
$BUILDDIR/setCrossEnvironment.sh \
sh -c '$STRIP xsel'

cd $BUILDDIR
} || exit 1

# =========== xsdl ==========

#ln -sf $BUILDDIR/../../../../../../libs/$TARGET_ARCH/libsdl-1.2.so $BUILDDIR/libSDL.so
ln -sf $BUILDDIR/../../../../../../jni/application/sdl-config $BUILDDIR/
ln -sf $BUILDDIR/libportable.a $BUILDDIR/libpthread.a # dummy
ln -sf $BUILDDIR/libportable.a $BUILDDIR/libts.a # dummy

[ -z "$PACKAGE_NAME" ] && PACKAGE_NAME=X.org.server

# Hack for NDK r19
SYSTEM_LIBDIR=$NDK/platforms
case $TARGET_ARCH in
	arm64-v8a)   SYSTEM_LIBDIR=$NDK/platforms/android-21/arch-arm64/usr/lib;;
	armeabi-v7a) SYSTEM_LIBDIR=$NDK/platforms/android-16/arch-arm/usr/lib;;
	x86)         SYSTEM_LIBDIR=$NDK/platforms/android-16/arch-x86/usr/lib;;
	x86_64)      SYSTEM_LIBDIR=$NDK/platforms/android-21/arch-x86_64/usr/lib64;;
esac

[ -e Makefile ] && grep "`pwd`" Makefile > /dev/null || \
env CFLAGS=" -DDEBUG \
	-isystem$BUILDDIR/usr/include \
	-isystem$BUILDDIR/../android-shmem \
	-include strings.h\
	-include linux/time.h \
	-DFNONBLOCK=O_NONBLOCK \
	-DFNDELAY=O_NDELAY \
	-D_LINUX_IPC_H \
	-Dipc_perm=debian_ipc_perm \
	-I$BUILDDIR/usr/include/pixman-1 \
	-I$BUILDDIR/../../../../../../jni/crypto/include \
	-I$BUILDDIR/../../../../../../jni/sdl-1.2/include" \
LDFLAGS="-L$BUILDDIR \
	-L$BUILDDIR/../../../../../../libs/$TARGET_ARCH \
	-L$SYSTEM_LIBDIR" \
PKG_CONFIG_PATH=$BUILDDIR/usr/lib/pkgconfig:$BUILDDIR/usr/share/pkgconfig \
./setCrossEnvironment.sh \
LIBS="-lfontenc -lfreetype -llog -lsdl-1.2 -lsdl_native_helpers -lGLESv1_CM -landroid-shmem -l:libcrypto.so.sdl.1.so -lz -lm -ldl" \
OPENSSL_LIBS=-l:libcrypto.so.sdl.1.so \
LIBSHA1_LIBS=-l:libcrypto.so.sdl.1.so \
PATH=$BUILDDIR:$PATH \
../../configure \
--host=$TARGET_HOST \
--prefix=$TARGET_DIR/usr \
--with-xkb-output=$TARGET_DIR/tmp \
--disable-xorg --disable-dmx --disable-xvfb --disable-xnest --disable-xquartz --disable-xwin \
--disable-xephyr --disable-unit-tests \
--disable-dri --disable-dri2 --disable-glx --disable-xf86vidmode \
--enable-xsdl --enable-kdrive \
--enable-mitshm --disable-config-udev --disable-libdrm \
|| exit 1

./setCrossEnvironment.sh make -j$NCPU V=1 2>&1 || exit 1

exit 0
