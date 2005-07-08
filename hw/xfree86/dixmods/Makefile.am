noinst_LTLIBRARIES = libdixmods.la

module_LTLIBRARIES = libbitmap.la libfb.la

# yes, this is a cheesy hack.
AM_CFLAGS = $(XORG_CFLAGS) -include xorg-config.h
INCLUDES = $(XORG_INCS) -I$(top_srcdir)/afb -I$(top_srcdir)/mfb \
           -I$(top_srcdir)/cfb -I$(top_srcdir)/dbe -I$(srcdir)/../loader \
           -I$(top_srcdir)/miext/layer -I$(top_srcdir)/miext/shadow

libbitmap_la_SOURCES = \
	bitmapmod.c
libbitmap_la_LDFLAGS = `pkg-config --libs xfont`

libfb_la_SOURCES = \
	$(top_srcdir)/fb/fbcmap.c
	fbmodule.c

libdixmods_la_SOURCES = \
	afbmodule.c \
	cfbmodule.c \
	cfb16module.c \
	cfb24module.c \
	cfb32module.c \
	mfbmodule.c \
	laymodule.c \
	shmodule.c \
	xkbKillSrv.c \
	xkbPrivate.c \
	xkbVT.c \
	$(top_srcdir)/mi/miinitext.c \
	$(top_srcdir)/Xext/xf86misc.c \
	$(top_srcdir)/Xext/xf86vmode.c

xkb%.c:
	echo "#define XF86DDXACTIONS" > $@
	echo "#include \"$(top_srcdir)/xkb/ddx$*.c\"" >> $@

DISTCLEANFILES = xkbKillSrv.c xkbPrivate.c xkbVT.c
