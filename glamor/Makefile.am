noinst_LTLIBRARIES = libglamor.la

# Override these since glamor doesn't need them and the needed files aren't
# built (in hw/xfree86/os-support/solaris) until after glamor is built
SOLARIS_ASM_CFLAGS=""

if XORG
sdk_HEADERS = glamor.h
endif

INCLUDES = \
	$(XORG_INCS)

AM_CFLAGS = $(XORG_CFLAGS) $(DIX_CFLAGS)

libglamor_la_SOURCES = \
	glamor.c \
	glamor.h
