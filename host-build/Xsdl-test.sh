#!/bin/sh

make -j8 || exit 1

if [ -z "$1" ]; then
	hw/kdrive/sdl/Xsdl :1 -exec xlogo
else
	gdb --args hw/kdrive/sdl/Xsdl :1 -exec xlogo
fi
