if DRI
DRI_SUBDIRS=drm
endif

SUBDIRS = bus @XORG_OS_SUBDIR@ misc $(DRI_SUBDIRS)
DIST_SUBDIRS = bus drm misc linux solaris

sdk_HEADERS = xf86_OSproc.h xf86_OSlib.h xf86_ansic.h xf86_libc.h xf86drm.h \
              xf86drmCompat.h assyntax.h xf86OSKbd.h xf86OSmouse.h

EXTRA_DIST = int10Defines.h xf86OSpriv.h
