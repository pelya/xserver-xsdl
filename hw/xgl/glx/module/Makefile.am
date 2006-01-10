AM_CFLAGS = \
	-I$(srcdir)/.. \
	-I$(srcdir)/../.. \
	$(XGLXMODULES_CFLAGS)

libxglx_la_LDFLAGS = -avoid-version
libxglx_la_SOURCES = xglxmodule.c
libxglx_la_LIBADD = \
	$(top_builddir)/hw/xgl/glx/libxglx.la \
	$(XGLXMODULES_LIBS)

moduledir = @xglmoduledir@

module_LTLIBRARIES = libxglx.la
