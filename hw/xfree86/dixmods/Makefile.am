noinst_LTLIBRARIES = libdixmods.la libxorgxkb.la

SUBDIRS = extmod

if GLX
GLXMODS = libglx.la libGLcore.la
endif

if XTRAP
XTRAPMOD = libxtrap.la
endif

if DBE
DBEMOD = libdbe.la
endif

if AFB
AFBMOD = libafb.la
endif

if CFB
CFBMOD = libcfb.la libcfb32.la
endif

if MFB
MFBMOD = libmfb.la
endif

module_LTLIBRARIES = $(AFBMOD) \
                     $(CFBMOD) \
                     libfb.la \
                     libwfb.la \
                     $(MFBMOD) \
                     libshadow.la

extsmoduledir = $(moduledir)/extensions
extsmodule_LTLIBRARIES = librecord.la \
			 $(DBEMOD) \
                         $(GLXMODS) \
                         $(XTRAPMOD)

fontsmoduledir = $(moduledir)/fonts
fontsmodule_LTLIBRARIES = libfreetype.la \
                          libtype1.la

AM_CFLAGS = @XORG_CFLAGS@ @DIX_CFLAGS@
INCLUDES = @XORG_INCS@ \
           -I$(top_srcdir)/dbe \
           -I$(top_srcdir)/hw/xfree86/loader \
           -I$(top_srcdir)/miext/shadow \
           -I$(top_srcdir)/GL/glx

libGLcore_la_LDFLAGS = -avoid-version
libGLcore_la_LIBADD = $(top_builddir)/GL/mesa/libGLcore.la
libGLcore_la_SOURCES = GLcoremodule.c

libafb_la_LDFLAGS = -avoid-version
libafb_la_LIBADD = $(top_builddir)/afb/libafb.la
libafb_la_SOURCES = afbmodule.c

libcfb_la_LDFLAGS = -avoid-version
libcfb_la_LIBADD = $(top_builddir)/cfb/libcfb.la
libcfb_la_SOURCES = cfbmodule.c

libcfb32_la_LDFLAGS = -avoid-version
libcfb32_la_LIBADD = $(top_builddir)/cfb32/libcfb32.la
libcfb32_la_SOURCES = cfb32module.c

libdbe_la_LDFLAGS = -avoid-version
libdbe_la_LIBADD = $(top_builddir)/dbe/libdbe.la
libdbe_la_SOURCES = dbemodule.c

libfb_la_LDFLAGS = -avoid-version
libfb_la_LIBADD = $(top_builddir)/fb/libfb.la
libfb_la_SOURCES = $(top_builddir)/fb/fbcmap_mi.c fbmodule.c
libfb_la_CFLAGS = $(AM_CFLAGS)

libwfb_la_LDFLAGS = -avoid-version
libwfb_la_LIBADD = $(top_builddir)/fb/libwfb.la
libwfb_la_SOURCES = $(top_builddir)/fb/fbcmap_mi.c fbmodule.c
libwfb_la_CFLAGS = $(AM_CFLAGS) -DFB_ACCESS_WRAPPER

libglx_la_LDFLAGS = -avoid-version
if AIGLX
GLXDRI_LIBRARY = $(top_builddir)/GL/glx/libglxdri.la
endif
libglx_la_LIBADD = \
	$(top_builddir)/GL/glx/libglx.la \
	$(GLXDRI_LIBRARY)
libglx_la_SOURCES = glxmodule.c

libmfb_la_LDFLAGS = -avoid-version
libmfb_la_LIBADD = $(top_builddir)/mfb/libmfb.la
libmfb_la_SOURCES = mfbmodule.c

librecord_la_LDFLAGS = -avoid-version
librecord_la_LIBADD = $(top_builddir)/record/librecord.la
librecord_la_SOURCES = recordmod.c

libshadow_la_LDFLAGS = -avoid-version
libshadow_la_LIBADD = $(top_builddir)/miext/shadow/libshadow.la
libshadow_la_SOURCES = shmodule.c

libfreetype_la_LDFLAGS = -avoid-version
libfreetype_la_SOURCES = ftmodule.c

libtype1_la_LDFLAGS = -avoid-version
libtype1_la_SOURCES = type1mod.c

libdixmods_la_SOURCES = $(top_srcdir)/mi/miinitext.c
libdixmods_la_CFLAGS = -DXFree86LOADER $(AM_CFLAGS)

libxorgxkb_la_SOURCES = xkbVT.c xkbPrivate.c xkbKillSrv.c
libxorgxkb_la_LIBADD = $(top_builddir)/dix/libdix.la

libxtrap_la_LDFLAGS = -avoid-version
libxtrap_la_LIBADD = $(top_builddir)/XTrap/libxtrap.la
libxtrap_la_SOURCES = xf86XTrapModule.c
