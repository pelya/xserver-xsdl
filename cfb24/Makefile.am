noinst_LTLIBRARIES = libcfb24.la

include $(top_srcdir)/cfb/Makefile.am.inc

libcfb24_la_SOURCES = $(libcfb_common_sources) $(libcfb_gen_sources)

EXTRA_DIST = cfbrrop24.h
	
AM_CFLAGS = -DPSZ=24

INCLUDES = $(CFB_INCLUDES)
