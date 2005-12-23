if XGLSERVER
MODULE_SUBDIRS = module
endif

SUBDIRS =		  \
	.		  \
	$(MODULE_SUBDIRS)

INCLUDES =		    \
	@XEGL_INCS@	    \
	@XSERVER_CFLAGS@    \
	@XGLSERVER_CFLAGS@  \
	@XEGLSERVER_CFLAGS@

noinst_LTLIBRARIES = libxegl.la

libxegl_la_SOURCES = \
	xegl.h	     \
	xegl.c	     \
	xeglinput.c  \
	kkeymap.h    \
	kinput.c     \
	evdev.c

if XEGLSERVER
Xegl_DEPENDENCIES = @XEGL_LIBS@
Xegl_LDFLAGS	  = -export-dynamic
Xegl_SOURCES	  = xeglinit.c
Xegl_LDADD	  =	  \
	@XEGL_LIBS@	  \
	@XSERVER_LIBS@    \
	@XGLSERVER_LIBS@  \
	@XEGLSERVER_LIBS@
Xegl_programs	  = Xegl
endif

bin_PROGRAMS = $(Xegl_programs)
