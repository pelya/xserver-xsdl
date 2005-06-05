INCLUDES =		    \
	@XGL_INCS@	    \
	@XSERVER_CFLAGS@

bin_PROGRAMS = Xegl

Xegl_SOURCES = xegl.c xeglinput.c kinput.c evdev.c

Xegl_LDADD =	          \
	@XGL_LIBS@        \
	@XEGL_LIBS@       \
	@XEGLSERVER_LIBS@ \
	@XSERVER_LIBS@

Xegl_DEPENDENCIES = @XGL_LIBS@

