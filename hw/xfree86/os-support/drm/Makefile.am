libdrm_la_LTLIBRARIES = libdrm.la
libdrm_la_CFLAGS = -I$(top_srcdir)/hw/xfree86/common \
                   -I$(top_srcdir)/hw/xfree86/dri \
                   -I$(top_srcdir)/hw/xfree86/os-support \
                   -I$(top_srcdir)/hw/xfree86/os-support/bus \
                   -I$(top_srcdir)/hw/xfree86/os-suport/shared/drm/kernel \
                   -I$(top_srcdir)/GL/glx \
                   -I$(top_srcdir)/GL/include \
                   -DHAVE_XORG_CONFIG_H \
                   @LIBDRM_CFLAGS@

libdrm_la_LDFLAGS = -module -avoid-version
libdrm_ladir = $(moduledir)/@XORG_OS@
libdrm_la_SOURCES = \
	xf86drm.c \
	xf86drmHash.c \
	xf86drmRandom.c \
	xf86drmSL.c \
	drmmodule.c
