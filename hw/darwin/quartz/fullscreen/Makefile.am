noinst_LIBRARIES = libfullscreen.a
AM_CFLAGS = @XORG_CFLAGS@ @DIX_CFLAGS@
INCLUDES = @XORG_INCS@ -I$(srcdir) -I$(srcdir)/.. -I$(srcdir)/../.. 

libfullscreen_a_SOURCES = fullscreen.c \
			   quartzCursor.c

EXTRA_DIST = \
	quartzCursor.h
