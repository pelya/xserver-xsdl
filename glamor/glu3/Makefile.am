noinst_LTLIBRARIES = libglu3.la

# Override these since glu3 doesn't need them and the needed files aren't
# built (in hw/xfree86/os-support/solaris) until after glu3 is built
SOLARIS_ASM_CFLAGS=""

INCLUDES = \
	$(XORG_INCS)

AM_CFLAGS = $(XORG_CFLAGS) $(DIX_CFLAGS)

libglu3_la_SOURCES = \
	matrix.c \
	glu3.h \
	glu3_scalar.h
