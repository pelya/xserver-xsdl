glamor_la_LTLIBRARIES = glamor.la
glamor_la_CFLAGS =					\
	-DHAVE_XORG_CONFIG_H				\
	@DIX_CFLAGS@ @XORG_CFLAGS@			\
	-I$(top_srcdir)/hw/xfree86/common		\
	-I$(top_srcdir)/hw/xfree86/os-support/bus	\
	-I$(top_srcdir)/hw/xfree86/parser		\
	-I$(top_srcdir)/hw/xfree86/modes		\
	-I$(top_srcdir)/hw/xfree86/ddc			\
	-I$(top_srcdir)/hw/xfree86/ramdac		\
	-I$(top_srcdir)/hw/xfree86/i2c			\
	-I$(top_srcdir)/glamor				\
	-I/usr/include/drm

glamor_la_LDFLAGS =							\
	-module -avoid-version -L$(libdir) -lEGL $(top_builddir)/glamor/libglamor.la -lGLEW
glamor_ladir = $(moduledir)/drivers
glamor_la_SOURCES =				\
	glamor.c				\
	glamor_crtc.c				\
	glamor_ddx.h

glamor_la_DEPENDENCIES = 			\
 $(top_builddir)/glamor/libglamor.la
