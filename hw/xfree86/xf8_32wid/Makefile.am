module_LTLIBRARIES = libxf8_32wid.la

sdk_HEADERS = cfb8_32wid.h

INCLUDES = $(XORG_INCS) -I$(top_srcdir)/cfb -I$(top_srcdir)/mfb

libxf8_32wid_la_SOURCES = \
	cfb8_32widmodule.c \
	cfbscrinit.c \
	cfbwindow.c \
	cfbwid.c
	
#libxf8_32wid_la_LIBADD = $(top_builddir)/cfb16/libcfb16.la \
#                         $(top_builddir)/cfb24/libcfb24.la \
#                         $(top_builddir)/cfb32/libcfb32.la
