noinst_LTLIBRARIES = librandr.la

AM_CFLAGS = $(DIX_CFLAGS) @SERVER_DEFINES@ @LOADER_DEFINES@

if XORG
sdk_HEADERS = randrstr.h
endif

librandr_la_SOURCES =	\
	mirandr.c	\
	randr.c		\
	randrstr.h
