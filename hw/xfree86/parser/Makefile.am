if INSTALL_LIBXF86CONFIG
lib_LTLIBRARIES = libxf86config.la
LIBHEADERS = \
	xf86Optrec.h \
	xf86Parser.h
else
noinst_LTLIBRARIES = libxf86config.la
endif

libxf86config_la_SOURCES = \
	Device.c \
	Files.c \
	Flags.c \
	Input.c \
	Layout.c \
	Module.c \
	Video.c \
	Monitor.c \
	Pointer.c \
	Screen.c \
	Vendor.c \
	read.c \
	scan.c \
	write.c \
	DRI.c \
	Extensions.c

AM_CFLAGS = $(DIX_CFLAGS) $(XORG_CFLAGS)

EXTRA_DIST = \
	Configint.h \
	configProcs.h \
	xf86Optrec.h \
	xf86Parser.h \
	xf86tokens.h

sdk_HEADERS = \
	xf86Parser.h \
	xf86Optrec.h
