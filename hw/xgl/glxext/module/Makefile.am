INCLUDES =		 \
	@XGLX_INCS@	 \
	@XSERVER_CFLAGS@

libglx_la_LDFLAGS = -avoid-version
libglx_la_SOURCES = glxmodule.c
libglx_la_LIBADD  = $(top_builddir)/GL/glx/libglx.la
libglx_modules    = libglx.la

libglcore_la_LDFLAGS = -avoid-version
libglcore_la_SOURCES = glcoremodule.c
libglcore_la_LIBADD  = $(top_builddir)/GL/mesa/libglcore.la
libglcore_modules    = libglcore.la

moduledir = @MODULEPATH@/xgl

module_LTLIBRARIES =	     \
	$(libglcore_modules) \
	$(libglx_modules)