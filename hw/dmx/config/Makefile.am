noinst_LIBRARIES = libdmxconfig.a

LIBSRCS = parser.y \
          scanner.l \
          dmxparse.c \
          dmxparse.h \
          dmxprint.c \
          dmxprint.h \
          dmxcompat.c \
          dmxcompat.h \
          dmxconfig.c \
          dmxconfig.h

parser.h: parser.c
scanner.c: scanner.l parser.h

BUILT_SOURCES = parser.c parser.h scanner.c

CLEANFILES = $(BUILT_SOURCES)

libdmxconfig_a_SOURCES = $(LIBSRCS)

AM_CFLAGS = -I$(top_srcdir)/hw/dmx -DHAVE_DMX_CONFIG_H @DMXMODULES_CFLAGS@
AM_YFLAGS = -d
