if XGLXSERVER
XGLX_SUBDIRS = glx
endif

if XEGLSERVER
XEGL_SUBDIRS = egl
endif

SUBDIRS = . $(XGLX_SUBDIRS) $(XEGL_SUBDIRS)

INCLUDES =		   \
	@XGL_INCS@	   \
	@XSERVER_CFLAGS@   \
	@XGLSERVER_CFLAGS@

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
	xglglx.c
