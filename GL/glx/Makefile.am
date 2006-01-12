noinst_LTLIBRARIES = libglx.la

AM_CFLAGS = \
	@DIX_CFLAGS@ \
	@GL_CFLAGS@ \
	@XLIB_CFLAGS@ \
	-I@MESA_SOURCE@/include \
	-I@MESA_SOURCE@/src/mesa/glapi \
	-I@MESA_SOURCE@/src/mesa/main \
	@SERVER_DEFINES@ \
	@MODULE_DEFINES@ \
	@LOADER_DEFINES@

# none yet
#sdk_HEADERS =

INCLUDES = -I$(top_srcdir)/GL/include -I$(top_srcdir)/hw/xfree86/os-support

nodist_libglx_la_SOURCES = indirect_size.h

libglx_la_SOURCES = \
        g_disptab.c \
        g_disptab.h \
        g_disptab_EXT.c \
        g_disptab_EXT.h \
        glapi.c \
        glcontextmodes.c \
        glcontextmodes.h \
        global.c \
        glthread.c \
        glxbuf.c \
        glxbuf.h \
        glxcmds.c \
        glxcmdsswap.c \
        glxcontext.h \
        glxdrawable.h \
        glxerror.h \
        glxext.c \
        glxext.h \
        glxfb.c \
        glxfb.h \
        glximports.c \
        glximports.h \
        glxmem.c \
        glxmem.h \
        glxpix.c \
        glxpix.h \
        glxscreens.c \
        glxscreens.h \
        glxserver.h \
        glxutil.c \
        glxutil.h \
        indirect_dispatch.c \
        indirect_dispatch.h \
        indirect_dispatch_swap.c \
        indirect_reqsize.c \
        indirect_reqsize.h \
        indirect_size_get.c \
        indirect_size_get.h \
        indirect_util.c \
        indirect_util.h \
        render2.c \
        render2swap.c \
        renderpix.c \
        renderpixswap.c \
        rensize.c \
        rensizetab.c \
        single2.c \
        single2swap.c \
        singlepix.c \
        singlepixswap.c \
        singlesize.c \
        singlesize.h \
        unpack.h \
        xfont.c
