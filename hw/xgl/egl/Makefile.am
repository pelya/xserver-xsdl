if XGL
XGL_MODULE_DIRS = module
endif

DIST_SUBDIRS = module

SUBDIRS =		  \
	.		  \
	$(XGL_MODULE_DIRS)

AM_CFLAGS = \
	$(DIX_CFLAGS) \
	-DHAVE_XGL_CONFIG_H \
	-DHAVE_DIX_CONFIG_H \
	$(XEGLMODULES_CFLAGS)

noinst_LTLIBRARIES = libxegl.la

libxegl_la_SOURCES = \
	xegl.h	     \
	xegl.c	     \
	xeglinput.c  \
	kkeymap.h    \
	kinput.c     \
	evdev.c

Xegl_DEPENDENCIES = @XEGL_LIBS@
Xegl_LDFLAGS	  = -export-dynamic
Xegl_SOURCES	  = \
	xeglinit.c \
	$(top_srcdir)/mi/miinitext.c \
	$(top_srcdir)/Xext/dpmsstubs.c \
	$(top_srcdir)/Xi/stubs.c \
	$(top_srcdir)/fb/fbcmap.c
Xegl_LDADD = \
	libxegl.la \
	../libxgl.a \
	$(XORG_CORE_LIBS) \
	$(XEGL_LIBS) \
	$(XSERVER_LIBS) \
	$(EXTENSION_LIBS) \
	$(XGLXMODULES_LIBS)
Xegl_programs	  = Xegl

bin_PROGRAMS = $(Xegl_programs)
