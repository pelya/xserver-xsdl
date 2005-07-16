noinst_LTLIBRARIES = libX.la

INCLUDES = -I@mesadir@/include \
           -I$(srcdir)/../X \
           -I$(srcdir)/../array_cache \
           -I$(srcdir)/../glapi \
           -I$(srcdir)/../main \
           -I$(srcdir)/../math \
           -I$(srcdir)/../shader \
           -I$(srcdir)/../swrast \
           -I$(srcdir)/../swrast_setup \
           -I$(srcdir)/../tnl \
           -I$(srcdir)/.. \
           -I$(top_srcdir)/GL/glx \
           -I$(top_srcdir)/GL/include

# terribly gross hack.  i need a shower now.
AM_CFLAGS = $(DIX_CFLAGS) -DXFree86Server

libX_la_SOURCES = xf86glx.c \
                  xf86glx_util.c \
                  xm_api.c \
                  xm_dd.c \
                  xm_line.c \
                  xm_span.c \
                  xm_tri.c
