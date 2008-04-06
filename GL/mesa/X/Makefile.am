noinst_LTLIBRARIES = libX.la

INCLUDES = -I@MESA_SOURCE@/include \
           -I. \
           -I@MESA_SOURCE@/src/mesa/glapi \
           -I@MESA_SOURCE@/src/mesa/main \
           -I@MESA_SOURCE@/src/mesa

# -DXFree86Server is required because the X11 driver in Mesa thinks that
# symbol means "being built in the server"
AM_CFLAGS = \
	$(DIX_CFLAGS) \
	-DXFree86Server \
	@GLX_DEFINES@

XM_SOURCES = \
	xm_api.c \
	xm_buffer.c \
	xm_dd.c \
	xm_image.c \
	xm_line.c \
	xm_span.c \
	xm_tri.c

XM_SOURCES += drivers/common/driverfuncs.c

nodist_libX_la_SOURCES = $(XM_SOURCES)
