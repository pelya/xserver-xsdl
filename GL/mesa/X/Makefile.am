noinst_LTLIBRARIES = libX.la

INCLUDES = -I@MESA_SOURCE@/include \
           -I../X \
           -I../array_cache \
           -I../glapi \
           -I../main \
           -I../math \
           -I../shader \
           -I../swrast \
           -I../swrast_setup \
           -I../tnl \
           -I.. \
           -I../../glx \
           -I$(top_srcdir)/GL/glx \
           -I$(top_srcdir)/GL/include \
           -I$(top_srcdir)/hw/xfree86/os-support

# terribly gross hack.  i need a shower now.
AM_CFLAGS = $(DIX_CFLAGS) -DXFree86Server @MODULE_DEFINES@ @LOADER_DEFINES@

libX_la_SOURCES = xf86glx.c \
                  xf86glx_util.c \
                  xf86glx_util.h \
                  xf86glxint.h

nodist_libX_la_SOURCES = \
                  xm_api.c \
		  xm_buffer.c \
                  xm_dd.c \
                  xm_line.c \
                  xm_span.c \
                  xm_tri.c \
		  drivers/common/driverfuncs.c
