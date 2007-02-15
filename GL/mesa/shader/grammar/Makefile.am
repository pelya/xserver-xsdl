noinst_LTLIBRARIES = libgrammar.la

AM_CFLAGS = \
	$(DIX_CFLAGS) \
	-DXFree86Server \
	@GLX_DEFINES@

INCLUDES = -I@MESA_SOURCE@/include \
           -I../../X \
           -I../../glapi \
           -I../../main \
           -I../../math \
           -I../../shader \
           -I../../swrast \
           -I../../swrast_setup \
           -I../../tnl \
           -I../.. \
           -I$(top_srcdir)/hw/xfree86/os-support

nodist_libgrammar_la_SOURCES = grammar_mesa.c
