noinst_LTLIBRARIES = libX.la

INCLUDES = -I@MESA_SOURCE@/include \
           -I../X \
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

# -DXFree86Server is required because the X11 driver in Mesa thinks that
# symbol means "being built in the server"
AM_CFLAGS = \
	$(DIX_CFLAGS) \
	-DXFree86Server \
	@GLX_DEFINES@

nodist_libX_la_SOURCES = \
                  xm_api.c \
		  xm_buffer.c \
                  xm_dd.c \
                  xm_image.c \
                  xm_line.c \
                  xm_span.c \
                  xm_tri.c \
		  drivers/common/driverfuncs.c
