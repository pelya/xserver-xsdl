AM_CFLAGS = $(DIX_CFLAGS)
AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/GL/glx \
	-I$(top_srcdir)/GL/include \
	-I$(top_srcdir)/GL/mesa/glapi \
	-I$(top_srcdir)/hw/darwin/quartz \
	-I$(top_srcdir)/hw/darwin/quartz/cr \
	-I$(top_srcdir)/miext/damage

if HAVE_AGL_FRAMEWORK
noinst_LIBRARIES = libAGLcore.a 
libAGLcore_a_SOURCES = aglGlx.c \
		       $(top_srcdir)/hw/darwin/quartz/xpr/x-list.c \
		       $(top_srcdir)/hw/darwin/quartz/xpr/x-list.h \
		       $(top_srcdir)/hw/darwin/quartz/xpr/x-hash.c \
		       $(top_srcdir)/hw/darwin/quartz/xpr/x-hash.h \
		       $(top_srcdir)/hw/dmx/glxProxy/compsize.c
endif
