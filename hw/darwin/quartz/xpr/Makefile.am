noinst_LIBRARIES = libxpr.a
AM_CFLAGS =  $(XORG_CFLAGS) $(DIX_CFLAGS)
AM_CPPFLAGS = $(XORG_INCS) \
	   -DHAVE_XORG_CONFIG_H \
	   -I$(srcdir) -I$(srcdir)/.. -I$(srcdir)/../.. \
	   -I$(top_srcdir)/miext \
	   -I$(top_srcdir)/miext/rootless \
	   -I$(top_srcdir)/miext/rootless/safeAlpha

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

EXTRA_DIST = \
	dri.h \
	dristruct.h \
	appledri.h \
	appledristr.h \
	x-hash.h \
	x-hook.h \
	x-list.h \
	Xplugin.h \
	xpr.h
