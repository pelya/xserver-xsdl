#!/bin/sh

make -j8 || exit 1

export SECURE_STORAGE_DIR=`pwd`

ln -sf /usr .
mkdir -p tmp

if [ -z "$1" ]; then
	hw/kdrive/ephyr/Xephyr :1 -audit 3 -listen inet -listen inet6 -nolisten unix -nopn -exec "x-window-manager & xlogo & xev & xclock -update 1"
else
	gdb --args hw/kdrive/ephyr/Xephyr :1 -exec xlogo
fi
