INCLUDES = -I$(srcdir)/../../shared/drm/kernel

noinst_LIBRARIES = libxorglinuxdrm.a
libxorglinuxdrm_a_SOURCES = xf86drm.c xf86drmHash.c xf86drmRandom.c \
                              xf86drmSL.c drmmodule.c

sdk_LIBDIR = $(libdir)/xorg
sdk_LIBS = libxorglinuxdrm.a
