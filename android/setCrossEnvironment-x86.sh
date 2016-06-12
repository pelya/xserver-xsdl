#!/bin/sh

IFS='
'

MYARCH=linux-x86_64
if uname -s | grep -i "linux" > /dev/null ; then
	MYARCH=linux-x86_64
fi
if uname -s | grep -i "darwin" > /dev/null ; then
	MYARCH=darwin-x86_64
fi
if uname -s | grep -i "windows" > /dev/null ; then
	MYARCH=windows-x86_64
fi

NDK=`which ndk-build`
NDK=`dirname $NDK`
NDK=`readlink -f $NDK`

[ -z "$NDK" ] && { echo "You need Andorid NDK r8 or newer installed to run this script" ; exit 1 ; }
GCCPREFIX=i686-linux-android
GCCVER=${GCCVER:-4.9}
PLATFORMVER=${PLATFORMVER:-android-14}
LOCAL_PATH=`dirname $0`
if which realpath > /dev/null ; then
	LOCAL_PATH=`realpath $LOCAL_PATH`
else
	LOCAL_PATH=`cd $LOCAL_PATH && pwd`
fi
ARCH=x86

CFLAGS="\
-fpic -ffunction-sections -funwind-tables -no-canonical-prefixes \
-fstack-protector -O2 -g -DNDEBUG \
-fomit-frame-pointer -fstrict-aliasing -funswitch-loops \
-finline-limit=300 \
-DANDROID -Wall -Wno-unused -Wa,--noexecstack -Wformat -Werror=format-security \
-isystem$NDK/platforms/$PLATFORMVER/arch-x86/usr/include \
-isystem$NDK/sources/cxx-stl/gnu-libstdc++/$GCCVER/include \
-isystem$NDK/sources/cxx-stl/gnu-libstdc++/$GCCVER/libs/$ARCH/include \
$CFLAGS"

UNRESOLVED="-Wl,--no-undefined"
SHARED="-Wl,--gc-sections -Wl,-z,nocopyreloc"
if [ -n "$BUILD_LIBRARY" ]; then
	[ -z "$SHARED_LIBRARY_NAME" ] && SHARED_LIBRARY_NAME=libapplication.so
	SHARED="-shared -Wl,-soname,$SHARED_LIBRARY_NAME"
fi
if [ -n "$ALLOW_UNRESOLVED_SYMBOLS" ]; then
	UNRESOLVED=
fi

LDFLAGS="\
$SHARED \
--sysroot=$NDK/platforms/$PLATFORMVER/arch-x86 \
-L$NDK/platforms/$PLATFORMVER/arch-x86/usr/lib \
-lc -lm -ldl -lz \
-L$NDK/sources/cxx-stl/gnu-libstdc++/$GCCVER/libs/$ARCH \
-lgnustl_static \
-no-canonical-prefixes $UNRESOLVED -Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now \
-lsupc++ \
$LDFLAGS"

env PATH=$NDK/toolchains/$ARCH-$GCCVER/prebuilt/$MYARCH/bin:$LOCAL_PATH:$PATH \
CFLAGS="$CFLAGS" \
CXXFLAGS="$CXXFLAGS $CFLAGS" \
LDFLAGS="$LDFLAGS" \
CC="$NDK/toolchains/$ARCH-$GCCVER/prebuilt/$MYARCH/bin/$GCCPREFIX-gcc" \
CXX="$NDK/toolchains/$ARCH-$GCCVER/prebuilt/$MYARCH/bin/$GCCPREFIX-g++" \
RANLIB="$NDK/toolchains/$ARCH-$GCCVER/prebuilt/$MYARCH/bin/$GCCPREFIX-ranlib" \
LD="$NDK/toolchains/$ARCH-$GCCVER/prebuilt/$MYARCH/bin/$GCCPREFIX-ld" \
AR="$NDK/toolchains/$ARCH-$GCCVER/prebuilt/$MYARCH/bin/$GCCPREFIX-ar" \
CPP="$NDK/toolchains/$ARCH-$GCCVER/prebuilt/$MYARCH/bin/$GCCPREFIX-cpp $CFLAGS" \
CXXCPP="$NDK/toolchains/$ARCH-$GCCVER/prebuilt/$MYARCH/bin/$GCCPREFIX-cpp -x c++ $CFLAGS" \
NM="$NDK/toolchains/$ARCH-$GCCVER/prebuilt/$MYARCH/bin/$GCCPREFIX-nm" \
AS="$NDK/toolchains/$ARCH-$GCCVER/prebuilt/$MYARCH/bin/$GCCPREFIX-as" \
STRIP="$NDK/toolchains/$ARCH-$GCCVER/prebuilt/$MYARCH/bin/$GCCPREFIX-strip" \
"$@"
