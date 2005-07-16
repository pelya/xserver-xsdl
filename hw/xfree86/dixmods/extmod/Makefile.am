sdk_HEADERS = dgaproc.h

module_LTLIBRARIES = libextmod.la

# DGA_SRCS = xf86dga.c xf86dga2.c
if XV
XV_SRCS = xvmod.c
endif

AM_CFLAGS = @XORG_CFLAGS@
INCLUDES = @XORG_INCS@ \
           -I$(top_srcdir)/afb \
           -I$(top_srcdir)/mfb \
           -I$(top_srcdir)/cfb \
           -I$(top_srcdir)/dbe \
           -I$(top_srcdir)/hw/xfree86/loader \
           -I$(top_srcdir)/miext/layer \
           -I$(top_srcdir)/miext/shadow

libextmod_la_LDFLAGS = -avoid-version
libextmod_la_LIBADD = $(top_srcdir)/Xext/libXext.la
libextmod_la_SOURCES = modinit.c \
                       $(DGA_SRCS) \
                       xf86misc.c \
                       xf86vmode.c \
                       $(XV_SRCS)
