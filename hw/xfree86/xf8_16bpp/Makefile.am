module_LTLIBRARIES = libxf8_16bpp.la

sdk_HEADERS = cfb8_16.h

INCLUDES = $(XORG_INCS) -I$(top_srcdir)/mfb -I$(top_srcdir)/cfb

AM_CFLAGS = $(XORG_CFLAGS)

libxf8_16bpp_la_SOURCES = \
	cfb8_16module.c \
	cfbscrinit.c \
	cfbwindow.c

libxf8_16bpp_la_LIBADD = $(top_builddir)/cfb16/libcfb16.la
