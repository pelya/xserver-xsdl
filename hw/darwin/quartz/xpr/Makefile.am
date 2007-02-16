noinst_LIBRARIES = libxpr.a
AM_CFLAGS =  @XORG_CFLAGS@ @DIX_CFLAGS@
INCLUDES = @XORG_INCS@ \
	   -I$(srcdir) -I$(srcdir)/.. -I$(srcdir)/../.. \
	   -I$(top_srcdir)/miext/rootless/safeAlpha \
	   -I$(top_srcdir)/miext/rootless \
	   -I$(top_srcdir)/miext

libxpr_a_SOURCES = \
	appledri.c \
        dri.c \
        xprAppleWM.c \
        xprCursor.c \
        xprFrame.c \
        xprScreen.c \
        x-hash.c \
        x-hook.c \
        x-list.c

xprbundledir = @APPLE_APPLICATIONS_DIR@/Resources/xpr.bundle/Contents/MacOS

EXTRA_DIST = \
	dri.h \
	dristruct.h \
	x-hash.h \
	x-hook.h \
	x-list.h \
	Xplugin.h \
	xpr.h
