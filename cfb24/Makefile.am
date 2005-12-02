noinst_LTLIBRARIES = libcfb24.la

include $(top_srcdir)/cfb/Makefile.am.inc

libcfb24_la_SOURCES = $(libcfb_common_sources) $(libcfb_gen_sources)

EXTRA_DIST = cfbrrop24.h

AM_CFLAGS = -DPSZ=24 $(DIX_CFLAGS) @SERVER_DEFINES@ @MODULE_DEFINES@ @LOADER_DEFINES@

INCLUDES = $(CFB_INCLUDES) -I$(top_srcdir)/hw/xfree86/os-support
