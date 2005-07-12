noinst_LIBRARIES = libdmxconfig.a

LIBSRCS = parser.y scanner.l dmxparse.c dmxprint.c dmxcompat.c dmxconfig.c

parser.h: parser.c
scanner.c: scanner.l parser.h

CLEANFILES = parser.c parser.h scanner.c

libdmxconfig_a_SOURCES = $(LIBSRCS)

AM_CFLAGS = -I.. -DHAVE_DMX_CONFIG_H @DMXMODULES_CFLAGS@
AM_YFLAGS = -d
