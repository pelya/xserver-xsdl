#!/bin/sh

make || exit 1

export SECURE_STORAGE_DIR="`pwd`/data"
cd data
../hw/kdrive/sdl/Xsdl :1111 \
  -mouse mouse -keybd keyboard \
  -nolock -noreset -nopn -listen inet -listen inet6 -nolisten unix \
  -fp /usr/share/fonts/X11/misc,/usr/share/fonts/X11/Type1,/usr/share/fonts/X11/100dpi,/usr/share/fonts/X11/75dpi,/usr/share/fonts \
  -screen 640/300x480/225x32
