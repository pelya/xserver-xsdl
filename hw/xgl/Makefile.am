if GLX
GLX_SUBDIRS = glxext
endif

if XGLX
XGLX_SUBDIRS = glx
endif

if XEGL
XEGL_SUBDIRS = egl
endif

DIST_SUBDIRS = glxext glx egl

SUBDIRS = 		\
	.		\
	$(GLX_SUBDIRS)  \
	$(XGLX_SUBDIRS) \
	$(XEGL_SUBDIRS)

AM_CFLAGS = \
	@SERVER_DEFINES@ \
	-DHAVE_XGL_CONFIG_H \
	-DHAVE_DIX_CONFIG_H \
	$(XGLMODULES_CFLAGS)

noinst_LIBRARIES = libxgl.a

libxgl_a_SOURCES =     \
	xgl.h	       \
	xglinput.c     \
	xgloutput.c    \
	xglcmap.c      \
	xglparse.c     \
	xglscreen.c    \
	xglarea.c      \
	xglgeometry.c  \
	xglpixmap.c    \
	xglsync.c      \
	xglsolid.c     \
	xgltile.c      \
	xglcopy.c      \
	xglfill.c      \
	xglwindow.c    \
	xglbstore.c    \
	xglget.c       \
	xglgc.c	       \
	xglshm.c       \
	xglcomp.c      \
	xglpict.c      \
	xglglyph.c     \
	xgltrap.c      \
	xglhash.c      \
        xglloader.c    \
	xglglx.c

EXTRA_DIST = \
	xglmodule.h

Xgl_LDFLAGS = -export-dynamic
Xgl_SOURCES = \
	xglinit.c \
	$(top_srcdir)/mi/miinitext.c \
	$(top_srcdir)/Xext/dpmsstubs.c \
	$(top_srcdir)/Xi/stubs.c \
	$(top_srcdir)/fb/fbcmap.c
Xgl_LDADD = \
	libxgl.a \
	$(XORG_CORE_LIBS) \
	$(XGL_LIBS) \
	$(XSERVER_LIBS) \
	$(EXTENSION_LIBS) \
	$(XGLMODULES_LIBS)
Xgl_programs = Xgl

bin_PROGRAMS = $(Xgl_programs)
