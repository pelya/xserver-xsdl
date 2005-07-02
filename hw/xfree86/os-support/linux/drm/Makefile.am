INCLUDES = -I$(srcdir)/../../shared/drm/kernel

sdklibdir = $(libdir)/xorg
sdklib_LIBRARIES = libxorglinuxdrm.a
libxorglinuxdrm_a_SOURCES = xf86drm.c xf86drmHash.c xf86drmRandom.c \
                            xf86drmSL.c drmmodule.c

AM_CFLAGS = $(XORG_CFLAGS)
