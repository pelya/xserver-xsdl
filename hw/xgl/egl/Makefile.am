INCLUDES =		    \
	@XGL_INCS@	    \
	@XSERVER_CFLAGS@    \
	@XGLXSERVER_CFLAGS@

bin_PROGRAMS = Xegl

Xegl_SOURCES =	    \
	xegl.h	    \
	xegl.c	    \
	xeglinput.c \
	kkeymap.h   \
	kinput.c    \
	evdev.c

Xegl_LDADD =		  \
	@XGL_LIBS@	  \
	@XSERVER_LIBS@	  \
	@XEGLSERVER_LIBS@

Xegl_DEPENDENCIES = @XGL_LIBS@

