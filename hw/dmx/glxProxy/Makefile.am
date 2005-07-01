noinst_LIBRARIES = libglxproxy.a
libglxproxy_a_SOURCES =  g_disptab.c glxcmds.c glxext.c glxutil.c g_renderswap.c \
                render2swap.c renderpixswap.c global.c glxcmdsswap.c \
		compsize.c glxscreens.c glxvisuals.c \
                glxsingle.c glxvendor.c glxfbconfig.c glxswap.c

AM_CFLAGS = -I.. -I$(top_srcdir)/include @GL_CFLAGS@ -UXFree86LOADER 