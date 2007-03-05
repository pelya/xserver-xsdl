AM_CFLAGS =  \
	    $(DIX_CFLAGS) \
	    $(XORG_CFLAGS)

INCLUDES = -I$(top_srcdir)/hw/xfree86/os-support

SUBDIRS = safeAlpha accel

noinst_LTLIBRARIES = librootless.la
librootless_la_SOURCES = \
	rootlessCommon.c \
	rootlessCommon.h \
	rootlessConfig.h \
	rootlessGC.c \
	rootless.h \
	rootlessScreen.c \
	rootlessValTree.c \
	rootlessWindow.c \
	rootlessWindow.h 

EXTRA_DIST = \
	README.txt
