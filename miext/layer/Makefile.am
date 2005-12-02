noinst_LTLIBRARIES = liblayer.la

AM_CFLAGS = $(DIX_CFLAGS) @SERVER_DEFINES@ @MODULE_DEFINES@ @LOADER_DEFINES@

INCLUDES = -I$(top_srcdir)/hw/xfree86/os-support

liblayer_la_SOURCES =	\
	layer.h		\
	layergc.c	\
	layerinit.c	\
	layerpict.c	\
	layerstr.h	\
	layerwin.c
