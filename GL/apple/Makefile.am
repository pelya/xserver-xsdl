AM_CFLAGS = $(DIX_CFLAGS)
AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/GL/glx \
	-I$(top_srcdir)/GL/include \
	-I$(top_srcdir)/GL/mesa/glapi \
	-I$(top_srcdir)/hw/xquartz \
	-I$(top_srcdir)/hw/xquartz/xpr \
	-I$(top_srcdir)/miext/damage

noinst_LIBRARIES = libCGLcore.a 
libCGLcore_a_SOURCES = \
	indirect.c \
	$(top_srcdir)/hw/dmx/glxProxy/compsize.c
