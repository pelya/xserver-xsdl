noinst_LTLIBRARIES = libcfb16.la

include $(top_srcdir)/cfb/Makefile.am.inc

libcfb16_la_SOURCES = $(libcfb_common_sources) $(libcfb_gen_sources)

INCLUDES = $(CFB_INCLUDES)

AM_CFLAGS = -DPSZ=16 $(DIX_CFLAGS)
