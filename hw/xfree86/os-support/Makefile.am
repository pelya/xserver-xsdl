if DRI
DRI_SUBDIRS=drm
endif

SUBDIRS = bus @XORG_OS_SUBDIR@ misc $(DRI_SUBDIRS)
DIST_SUBDIRS = bsd bus drm misc linux solaris

sdk_HEADERS = xf86_OSproc.h xf86_OSlib.h xf86_ansic.h xf86_libc.h xf86drm.h \
              xf86drmCompat.h assyntax.h xf86OSKbd.h xf86OSmouse.h

EXTRA_DIST = int10Defines.h xf86OSpriv.h README.OS-lib

# to get the grouping semantics right, you have to glom these three together
# as one library, otherwise libtool will actively defeat your attempts to
# list them multiple times on the link line.
noinst_LTLIBRARIES = libxorgos.la
libxorgos_la_SOURCES = xorgos.c
libxorgos_la_LIBADD = @XORG_OS_SUBDIR@/lib@XORG_OS_SUBDIR@.la \
                      bus/libbus.la \
                      misc/libmisc.la

xorgos.c:
	touch $@

DISTCLEANFILES = xorgos.c
