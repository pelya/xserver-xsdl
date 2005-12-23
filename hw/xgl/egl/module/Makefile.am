INCLUDES =		    \
	@XEGL_INCS@	    \
	@XSERVER_CFLAGS@    \
	@XEGLSERVER_CFLAGS@

libxegl_la_LDFLAGS = -avoid-version
libxegl_la_SOURCES = xeglmodule.c
libxegl_la_LIBADD   =	  \
	@XEGL_MOD_LIBS@	  \
	@XEGLSERVER_LIBS@

moduledir = @MODULEPATH@/xgl

module_LTLIBRARIES = libxegl.la