sdk_HEADERS = edid.h vdif.h xf86DDC.h

module_LTLIBRARIES = libddc.la

libddc_la_LDFLAGS = -avoid-version
libddc_la_SOURCES = xf86DDC.c edid.c interpret_edid.c print_edid.c \
                   interpret_vdif.c print_vdif.c ddcProperty.c

INCLUDES = $(XORG_INCS) -I$(srcdir)/../i2c

AM_CFLAGS = $(XORG_CFLAGS)

EXTRA_DIST = ddcPriv.h DDC.HOWTO
