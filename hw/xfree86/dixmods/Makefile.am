noinst_LTLIBRARIES = libdixmods.la libxorgxkb.la

SUBDIRS = extmod

if GLX
GLXMODS = libglx.la libGLcore.la
endif

if XTRAP
XTRAPMOD = libxtrap.la
endif

# cfb24 is only used by xf8_32wid, which is only used by sunffb on SPARC
if BUILD_XF8_32WID
CFB24MOD = libcfb24.la
endif

module_LTLIBRARIES = libafb.la \
                     libcfb.la \
                     libcfb16.la \
                     $(CFB24MOD) \
                     libcfb32.la \
                     libfb.la \
                     liblayer.la \
                     libmfb.la \
                     libshadow.la

extsmoduledir = $(moduledir)/extensions
extsmodule_LTLIBRARIES = libdbe.la \
                         librecord.la \
                         $(GLXMODS) \
                         $(XTRAPMOD)

fontsmoduledir = $(moduledir)/fonts
fontsmodule_LTLIBRARIES = libbitmap.la \
                          libfreetype.la \
                          libtype1.la

AM_CFLAGS = @XORG_CFLAGS@ @DIX_CFLAGS@ @SERVER_DEFINES@ @LOADER_DEFINES@
INCLUDES = @XORG_INCS@ \
           -I$(top_srcdir)/afb \
           -I$(top_srcdir)/cfb \
           -I$(top_srcdir)/mfb \
           -I$(top_srcdir)/dbe \
           -I$(top_srcdir)/hw/xfree86/loader \
           -I$(top_srcdir)/miext/layer \
           -I$(top_srcdir)/miext/shadow

libGLcore_la_LDFLAGS = -avoid-version
libGLcore_la_LIBADD = $(top_builddir)/GL/mesa/libGLcore.la
libGLcore_la_SOURCES = GLcoremodule.c

libafb_la_LDFLAGS = -avoid-version
libafb_la_LIBADD = $(top_builddir)/afb/libafb.la
libafb_la_SOURCES = afbmodule.c

# libbitmap is just a wrapper around the bitmap font code in Xfont.
# it doesn't even make sense to have it loadable, but, welcome to X.
libbitmap_la_SOURCES = bitmapmod.c
libbitmap_la_LDFLAGS = -avoid-version # `pkg-config --libs xfont`

libcfb_la_LDFLAGS = -avoid-version
libcfb_la_LIBADD = $(top_builddir)/cfb/libcfb.la
libcfb_la_SOURCES = cfbmodule.c

libcfb16_la_LDFLAGS = -avoid-version
libcfb16_la_LIBADD = $(top_builddir)/cfb16/libcfb16.la
libcfb16_la_SOURCES = cfb16module.c

libcfb24_la_LDFLAGS = -avoid-version
libcfb24_la_LIBADD = $(top_builddir)/cfb24/libcfb24.la
libcfb24_la_SOURCES = cfb24module.c

libcfb32_la_LDFLAGS = -avoid-version
libcfb32_la_LIBADD = $(top_builddir)/cfb32/libcfb32.la
libcfb32_la_SOURCES = cfb32module.c

libdbe_la_LDFLAGS = -avoid-version
libdbe_la_LIBADD = $(top_builddir)/dbe/libdbemodule.la
libdbe_la_SOURCES = dbemodule.c

libfb_la_LDFLAGS = -avoid-version
libfb_la_LIBADD = $(top_builddir)/fb/libfb.la
libfb_la_SOURCES = $(top_builddir)/fb/fbcmap.c fbmodule.c

libglx_la_LDFLAGS = -avoid-version
libglx_la_LIBADD = $(top_builddir)/GL/glx/libglx.la
libglx_la_SOURCES = glxmodule.c

liblayer_la_LDFLAGS = -avoid-version
liblayer_la_LIBADD = $(top_builddir)/miext/layer/liblayer.la
liblayer_la_SOURCES = laymodule.c

libmfb_la_LDFLAGS = -avoid-version
libmfb_la_LIBADD = $(top_builddir)/mfb/libmfb.la
libmfb_la_SOURCES = mfbmodule.c

librecord_la_LDFLAGS = -avoid-version
librecord_la_LIBADD = $(top_builddir)/record/librecordmodule.la
librecord_la_SOURCES = recordmod.c

libshadow_la_LDFLAGS = -avoid-version
libshadow_la_LIBADD = $(top_builddir)/miext/shadow/libshadow.la
libshadow_la_SOURCES = shmodule.c

libfreetype_la_LDFLAGS = -avoid-version
libfreetype_la_SOURCES = ftmodule.c

libtype1_la_LDFLAGS = -avoid-version
libtype1_la_SOURCES = type1mod.c

libdixmods_la_SOURCES = $(top_srcdir)/mi/miinitext.c

libxorgxkb_la_SOURCES = xkbVT.c xkbPrivate.c xkbKillSrv.c

libxtrap_la_LDFLAGS = -avoid-version
libxtrap_la_LIBADD = $(top_builddir)/XTrap/libxtrapmodule.la
libxtrap_la_SOURCES = xf86XTrapModule.c

xkb%.c:
	echo "#define XF86DDXACTIONS" > $@
	echo "#include \"$(top_srcdir)/xkb/ddx$*.c\"" >> $@
