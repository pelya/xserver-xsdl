noinst_LTLIBRARIES = librecord.la librecordmodule.la

AM_CFLAGS = $(DIX_CFLAGS) @SERVER_DEFINES@ @LOADER_DEFINES@

INCLUDES = -I$(top_srcdir)/hw/xfree86/os-support

librecord_la_SOURCES = record.c set.c

librecordmodule_la_CFLAGS = $(AM_CFLAGS) @MODULE_DEFINES@
librecordmodule_la_SOURCES = record.c set.c

EXTRA_DIST = set.h
