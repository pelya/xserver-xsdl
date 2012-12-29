inst_LTLIBRARIES = libglamor.la

# Override these since glamor doesn't need them and the needed files aren't
# built (in hw/xfree86/os-support/solaris) until after glamor is built
SOLARIS_ASM_CFLAGS=""

if GLAMOR_GLES2
libglamor_la_LIBADD = $(GLESV2_LIBS)
else
libglamor_la_LIBADD = $(GL_LIBS)
endif

instdir = $(moduledir)

INCLUDES = \
	$(XORG_INCS)

AM_CFLAGS = $(CWARNFLAGS) $(XORG_CFLAGS) $(DIX_CFLAGS) $(LIBDRM_CFLAGS)

libglamor_la_LDFLAGS = -avoid-version

libglamor_la_SOURCES = \
	compat-api.h \
	glamor.c \
	glamor_copyarea.c \
	glamor_copywindow.c \
	glamor_core.c \
	glamor_debug.h \
	glamor_gl_dispatch.h \
	glamor_fill.c \
	glamor_fillspans.c \
	glamor_getspans.c \
	glamor_glyphs.c \
	glamor_polyfillrect.c \
	glamor_polylines.c \
	glamor_putimage.c \
	glamor_setspans.c \
	glamor_render.c \
	glamor_gradient.c \
	glamor_trapezoid.c \
	glamor_tile.c \
	glamor_triangles.c\
	glamor_addtraps.c\
	glamor_getimage.c\
	glamor_copyplane.c\
	glamor_glyphblt.c\
	glamor_polyops.c\
	glamor_priv.h\
	glamor_pixmap.c\
	glamor_largepixmap.c\
	glamor_picture.c\
	glamor_window.c\
	glamor_gl_dispatch.c\
	glamor_fbo.c\
	glamor_compositerects.c\
	glamor_utils.h\
	glamor.h\
	glapi.h

sdk_HEADERS = glamor.h

if EGL
LIBGLAMOREGL = libglamoregl.la
module_LTLIBRARIES = $(LIBGLAMOREGL)
libglamoregl_la_DEPENDENCIES = libglamor.la
libglamoregl_la_LDFLAGS = -avoid-version -module $(EGL_LIBS) -lglamor $(GLX_SYS_LIBS)
libglamoregl_la_SOURCES = glamor_eglmodule.c $(top_srcdir)/src/glamor_egl.c
libglamoregl_la_CFLAGS = $(AM_CFLAGS) $(GLX_DEFINES) -I$(top_srcdir)/src $(LIBDRM_CFLAGS) $(EGL_CFLAGS)
endif


