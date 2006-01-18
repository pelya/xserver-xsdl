noinst_LTLIBRARIES = libdbe.la libdbemodule.la

AM_CFLAGS = $(DIX_CFLAGS) @SERVER_DEFINES@ @LOADER_DEFINES@

INCLUDES = -I$(top_srcdir)/hw/xfree86/os-support

if XORG
sdk_HEADERS = dbestruct.h
endif

libdbe_la_SOURCES = \
        dbe.c \
        midbe.c \
        midbe.h \
        midbestr.h

libdbemodule_la_CFLAGS = $(AM_CFLAGS) @MODULE_DEFINES@
libdbemodule_la_SOURCES = \
        dbe.c \
        midbe.c \
        midbe.h \
        midbestr.h
