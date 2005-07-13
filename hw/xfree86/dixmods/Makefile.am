noinst_LTLIBRARIES = libdixmods.la

if GLX
GLXMODS = libglx.la libGLcore.la
endif

module_LTLIBRARIES = \
        libafb.la libbitmap.la libcfb.la libcfb16.la libcfb24.la libcfb32.la \
        libdbe.la libfb.la liblayer.la libmfb.la libshadow.la $(GLXMODS)

# yes, this is a cheesy hack.
AM_CFLAGS = $(XORG_CFLAGS) -include xorg-config.h
INCLUDES = $(XORG_INCS) -I$(top_srcdir)/afb -I$(top_srcdir)/mfb \
           -I$(top_srcdir)/cfb -I$(top_srcdir)/dbe -I$(srcdir)/../loader \
           -I$(top_srcdir)/miext/layer -I$(top_srcdir)/miext/shadow

libGLcore_la_LDFLAGS = -avoid-version
libGLcore_la_LIBADD = $(top_srcdir)/GL/mesa/libGLcore.a
libGLcore_la_SOURCES = GLcoremodule.c

libafb_la_LDFLAGS = -avoid-version
libafb_la_LIBADD = $(top_srcdir)/afb/libafb.la
libafb_la_SOURCES = afbmodule.c

# libbitmap is just a wrapper around the bitmap font code in Xfont.
# it doesn't even make sense to have it loadable, but, welcome to X.
libbitmap_la_SOURCES = bitmapmod.c
libbitmap_la_LDFLAGS = -avoid-version # `pkg-config --libs xfont`

libcfb_la_LDFLAGS = -avoid-version
libcfb_la_LIBADD = $(top_srcdir)/cfb/libcfb.la
libcfb_la_SOURCES = cfbmodule.c

libcfb16_la_LDFLAGS = -avoid-version
libcfb16_la_LIBADD = $(top_srcdir)/cfb16/libcfb16.la
libcfb16_la_SOURCES = cfb16module.c

libcfb24_la_LDFLAGS = -avoid-version
libcfb24_la_LIBADD = $(top_srcdir)/cfb24/libcfb24.la
libcfb24_la_SOURCES = cfb24module.c

libcfb32_la_LDFLAGS = -avoid-version
libcfb32_la_LIBADD = $(top_srcdir)/cfb32/libcfb32.la
libcfb32_la_SOURCES = cfb32module.c

libdbe_la_LDFLAGS = -avoid-version
libdbe_la_LIBADD = $(top_srcdir)/dbe/libdbe.la
libdbe_la_SOURCES = dbemodule.c

libfb_la_LDFLAGS = -avoid-version
libfb_la_LIBADD = $(top_srcdir)/fb/libfb.la
libfb_la_SOURCES = $(top_srcdir)/fb/fbcmap.c fbmodule.c

libglx_la_LDFLAGS = -avoid-version
libglx_la_LIBADD = $(top_srcdir)/GL/glx/libglx.la
libglx_la_SOURCES = glxmodule.c

liblayer_la_LDFLAGS = -avoid-version
liblayer_la_LIBADD = $(top_srcdir)/miext/layer/liblayer.la
liblayer_la_SOURCES = laymodule.c

libmfb_la_LDFLAGS = -avoid-version
libmfb_la_LIBADD = $(top_srcdir)/mfb/libmfb.la
libmfb_la_SOURCES = mfbmodule.c

libshadow_la_LDFLAGS = -avoid-version
libshadow_la_LIBADD = $(top_srcdir)/miext/shadow/libshadow.la
libshadow_la_SOURCES = shmodule.c

libdixmods_la_SOURCES = \
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
