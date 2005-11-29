noinst_LTLIBRARIES = libdamage.la

AM_CFLAGS = $(DIX_CFLAGS) @MODULE_DEFINES@ @LOADER_DEFINES@

INCLUDES = -I$(srcdir)/../cw -I$(top_srcdir)/hw/xfree86/os-support

libdamage_la_SOURCES =	\
	damage.c	\
	damage.h	\
	damagestr.h
