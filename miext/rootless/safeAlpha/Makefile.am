AM_CFLAGS = \
	    $(DIX_CFLAGS) \
            $(XORG_CFLAGS)

INCLUDES = -I$(srcdir)/.. -I$(top_srcdir)/hw/xfree86/os-support


noinst_LTLIBRARIES = libsafeAlpha.la
libsafeAlpha_la_SOURCES = safeAlphaPicture.c \
		       safeAlphaWindow.c

EXTRA_DIST = safeAlpha.h
