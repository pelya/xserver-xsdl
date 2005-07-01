noinst_LTLIBRARIES = libdixmods.la

module_LTLIBRARIES = libbitmap.la libfb.la

# yes, this is a cheesy hack.
AM_CFLAGS = -include xorg-config.h $(XORG_CFLAGS)
INCLUDES = $(XORG_INCS) -I$(top_srcdir)/afb -I$(top_srcdir)/mfb \
           -I$(top_srcdir)/cfb -I$(top_srcdir)/dbe -I$(srcdir)/../loader \
           -I$(top_srcdir)/miext/layer -I$(top_srcdir)/miext/shadow

libbitmap_la_SOURCES = bitmapmod.c
libbitmap_la_LDFLAGS = -lXfont
libfb_la_SOURCES = fbmodule.c

libdixmods_la_SOURCES = \
	afbmodule.c \
	cfbmodule.c \
	cfb16module.c \
	cfb24module.c \
	cfb32module.c \
	mfbmodule.c \
	miinitext.c \
	laymodule.c \
	shmodule.c \
	$(top_srcdir)/Xext/xf86misc.c \
	$(top_srcdir)/Xext/xf86vmode.c
