if GLX
GLX_SUBDIRS = glxext
endif

if XGLX
XGLX_SUBDIRS = glx
endif

if XEGL
XEGL_SUBDIRS = egl
endif

SUBDIRS = 		\
	.		\
	$(GLX_SUBDIRS)  \
	$(XGLX_SUBDIRS) \
	$(XEGL_SUBDIRS)

INCLUDES =		   \
	@XGL_INCS@	   \
	@XSERVER_CFLAGS@   \
	@XGLSERVER_CFLAGS@ \
	-DMODULEPATH=\"@MODULEPATH@\"

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

if XGLSERVER
Xgl_DEPENDENCIES = @XGL_LIBS@
Xgl_LDFLAGS = -export-dynamic
Xgl_SOURCES = xglinit.c
Xgl_LDADD =	\
	@XGL_LIBS@	\
	@XSERVER_LIBS@	\
	@XGLSERVER_LIBS@
Xgl_programs = Xgl
endif

bin_PROGRAMS = $(Xgl_programs)
