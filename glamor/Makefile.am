noinst_LTLIBRARIES = libglamor.la

# Override these since glamor doesn't need them and the needed files aren't
# built (in hw/xfree86/os-support/solaris) until after glamor is built
SOLARIS_ASM_CFLAGS=""

SUBDIRS = glu3

if XORG
sdk_HEADERS = glamor.h
endif

INCLUDES = \
	$(XORG_INCS)

AM_CFLAGS = $(XORG_CFLAGS) $(DIX_CFLAGS)

libglamor_la_SOURCES = \
	glamor.c \
	glamor_copyarea.c \
	glamor_copywindow.c \
	glamor_core.c \
	glamor_fill.c \
	glamor_fillspans.c \
	glamor_getspans.c \
	glamor_glyphs.c \
	glamor_polyfillrect.c \
	glamor_polylines.c \
	glamor_putimage.c \
	glamor_setspans.c \
	glamor_render.c \
	glamor_tile.c \
	glamor.h
libglamor_la_LIBADD = \
	glu3/libglu3.la
