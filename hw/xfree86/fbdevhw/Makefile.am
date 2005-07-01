module_LTLIBRARIES = libfbdevhw.la

#if FBDEVHW
libfbdevhw_la_SOURCES = fbdevhw.c
#else
#libxorgfbdevhw_la_SOURCES = fbdevhwstub.c
#endif

INCLUDES = $(XORG_INCS) -I$(srcdir)/../i2c

sdk_HEADERS = fbdevhw.h

EXTRA_DIST = fbpriv.h
