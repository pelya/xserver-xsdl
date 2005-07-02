noinst_LTLIBRARIES = libcfb.la

include Makefile.am.inc

DISTCLEANFILES += cfbglrop8.c

libcfb_la_SOURCES = cfb8bit.c cfbcmap.c cfbteblt8.c cfbglrop8.c cfbpush8.c \
                    cfbrctstp8.c $(libcfb_gen_sources) $(libcfb_common_sources)

libcfb_la_LIBADD = ../mfb/libmfb.la

AM_CFLAGS = -DPSZ=8 $(DIX_CFLAGS)

INCLUDES = $(CFB_INCLUDES)

EXTRA_DIST = cfbline.c cfbfillarc.c cfbzerarc.c cfbblt.c cfbsolid.c \
             cfbtileodd.c cfbtile32.c cfb8line.c cfbply1rct.c cfbglblt8.c \
	     cfb16.h cfb24.h cfb32.h cfb8bit.h cfbmap.h cfbmskbits.h cfbrrop.h \
	     cfbtab.h cfbunmap.h

sdk_HEADERS = cfb.h
