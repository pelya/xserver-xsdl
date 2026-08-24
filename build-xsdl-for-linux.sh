#!/bin/sh
env SDL=yes CFLAGS="-O0 -g -DDEBUG" \
./configure \
--enable-debug \
--prefix=`pwd`/data/usr \
--with-xkb-path=`pwd`/data/xkb \
--disable-xorg --disable-dmx --disable-xvfb --disable-xnest --disable-xquartz --disable-xwin \
--disable-xwayland --disable-xephyr --disable-unit-tests \
--disable-dri --disable-dri2 --disable-glx --disable-xf86vidmode \
--disable-config-udev --disable-libdrm \
--disable-libunwind \
--with-systemd-daemon=no \
--enable-xsdl --enable-kdrive --enable-mitshm \
|| exit 1

make -j8 || exit 1

make install || exit 1

ln -sf /usr/bin/xkbcomp data/usr/bin/xkbcomp || exit 1

