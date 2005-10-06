noinst_LIBRARIES = libparser.a
libparser_a_SOURCES = Device.c Files.c Flags.c Input.c Keyboard.c Layout.c \
                      Module.c Video.c Monitor.c Pointer.c Screen.c \
                      Vendor.c read.c scan.c write.c DRI.c Extensions.c

AM_CFLAGS = $(XORG_CFLAGS)

EXTRA_DIST = \
	Configint.h \
	configProcs.h \
	xf86Optrec.h \
	xf86Parser.h \
	xf86tokens.h \
	cpconfig.c
