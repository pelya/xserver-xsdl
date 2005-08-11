noinst_LTLIBRARIES = libfb.la

AM_CFLAGS = $(DIX_CFLAGS)

sdk_HEADERS = fb.h fbrop.h fbpseudocolor.h fboverlay.h

if MMX_CAPABLE
AM_CFLAGS += -mmmx -msse -Winline --param inline-unit-growth=10000 \
        --param large-function-growth=10000 -DUSE_MMX
endif

libfb_la_SOURCES = 	\
	fb.h		\
	fb24_32.c	\
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
	fbedgeimp.h	\
	fbmmx.c		\
	fbmmx.h

EXTRA_DIST = fbcmap.c
