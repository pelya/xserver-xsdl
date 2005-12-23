INCLUDES =		    \
	@XGLX_INCS@	    \
	@XSERVER_CFLAGS@    \
	@XGLXSERVER_CFLAGS@

libxglx_la_LDFLAGS = -avoid-version
libxglx_la_SOURCES = xglxmodule.c
libxglx_la_LIBADD   =	  \
	@XGLX_MOD_LIBS@	  \
	@XGLXSERVER_LIBS@

moduledir = @MODULEPATH@/xgl

module_LTLIBRARIES = libxglx.la