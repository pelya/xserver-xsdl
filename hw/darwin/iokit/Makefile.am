noinst_LIBRARIES = libiokit.a

AM_CFLAGS = @XORG_CFLAGS@ @DIX_CFLAGS@
INCLUDES = -I. -I.. -I$(srcdir) -I$(srcdir)/.. @XORG_INCS@
AM_DEFS =
if XQUARTZ
AM_DEFS += -DDARWIN_WITH_QUARTZ -DXFree86Server
XQUARTZ_SUBDIRS = bundle quartz
endif
DEFS = @DEFS@ $(AM_DEFS)

libiokit_a_SOURCES = xfIOKit.c \
	           xfIOKitCursor.c \
	           xfIOKitStartup.c

EXTRA_DIST = \
	xfIOKit.h
