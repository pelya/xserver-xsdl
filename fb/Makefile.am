noinst_LTLIBRARIES = libfb.la libfbmmx.la

INCLUDES = \
	-I$(top_srcdir)/hw/xfree86/os-support \
	-I$(top_srcdir)/hw/xfree86/os-support/bus \
	-I$(top_srcdir)/hw/xfree86/common
AM_CFLAGS = $(DIX_CFLAGS)

if XORG
sdk_HEADERS = fb.h fbrop.h fbpseudocolor.h fboverlay.h
endif

if MMX_CAPABLE
AM_CFLAGS += -DUSE_MMX

libfbmmx_la_CFLAGS =				\
	$(DIX_CFLAGS)				\
	-DUSE_MMX				\
	-mmmx					\
	-msse					\
	-Winline				\
	--param inline-unit-growth=10000	\
        --param large-function-growth=10000
endif

libfbmmx_la_SOURCES =	\
	fbmmx.c		\
	fbmmx.h

libfb_la_SOURCES = 	\
	fb.h		\
	fb24_32.c	\
	fb24_32.h	\
	fballpriv.c	\
	fbarc.c		\
	fbbits.c	\
	fbbits.h	\
	fbblt.c		\
	fbbltone.c	\
	fbbstore.c	\
	fbcompose.c	\
	fbcopy.c	\
	fbfill.c	\
	fbfillrect.c	\
	fbfillsp.c	\
	fbgc.c		\
	fbgetsp.c	\
	fbglyph.c	\
	fbimage.c	\
	fbline.c	\
	fboverlay.c	\
	fboverlay.h	\
	fbpict.c	\
	fbpict.h	\
	fbpixmap.c	\
	fbpoint.c	\
	fbpush.c	\
	fbrop.h		\
	fbscreen.c	\
	fbseg.c		\
	fbsetsp.c	\
	fbsolid.c	\
	fbstipple.c	\
	fbtile.c	\
	fbtrap.c	\
	fbutil.c	\
	fbwindow.c	\
	fbpseudocolor.c \
	fbpseudocolor.h \
	fbedge.c	\
	fbedgeimp.h

libfb_la_LIBADD = libfbmmx.la

EXTRA_DIST = fbcmap.c
