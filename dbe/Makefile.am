noinst_LTLIBRARIES = libdbe.la

AM_CFLAGS = $(DIX_CFLAGS) @SERVER_DEFINES@ @LOADER_DEFINES@

if XORG
sdk_HEADERS = dbestruct.h
endif

libdbe_la_SOURCES = \
        dbe.c \
        midbe.c \
        midbe.h \
        midbestr.h
