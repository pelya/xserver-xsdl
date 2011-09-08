noinst_LTLIBRARIES = libglamor.la

# Override these since glamor doesn't need them and the needed files aren't
# built (in hw/xfree86/os-support/solaris) until after glamor is built
SOLARIS_ASM_CFLAGS=""

if XORG
sdk_HEADERS = glamor.h
endif

if GLAMOR_GLES2
libglamor_la_LIBADD = $(GLESV2_LIBS)
else
libglamor_la_LIBADD = $(GL_LIBS)
endif

if XORG
sdk_HEADERS = glamor.h
endif

INCLUDES = \
	$(XORG_INCS)


AM_CFLAGS = $(XORG_CFLAGS) $(DIX_CFLAGS) $(LIBDRM_CFLAGS)

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
        glamor_triangles.c\
        glamor_pixmap.c\
        glamor_picture.c\
	glamor_window.c\
        glamor_gl_dispatch.c\
	glamor.h
