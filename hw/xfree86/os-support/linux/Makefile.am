if DRI
DRI_SUBDIRS=drm
DRI_LIBS=drm/libxorglinuxdrm.a
endif

noinst_LIBRARIES = liblinux.a
liblinux_a_SOURCES = lnx_init.c lnx_video.c lnx_io.c lnx_kbd.c lnx_mouse.c \
                     lnx_pci.c lnx_apm.c lnx_agp.c lnx_kmod.c lnx_KbdMap.c \
                     ../shared/bios_mmap.c ../shared/VTsw_usl.c \
                     ../shared/std_kbdEv.c ../shared/posix_tty.c \
                     ../shared/vidmem.c ../shared/sigio.c \
                     ../shared/stdResource.c ../shared/libc_wrapper.c \
                     ../shared/at_scancode.c
#libxorglinux_a_LIBADD = int10/libxorglinuxint10.a \
#                         $(DRI_LIBS)

sdk_HEADERS = agpgart.h

AM_CFLAGS = -DUSESTDRES -DHAVE_SYSV_IPC

SUBDIRS = $(DRI_SUBDIRS)

INCLUDES = $(XORG_INCS) -I/usr/include/drm # FIXME this last part is crack

EXTRA_DIST = \
	lnx.h \
	lnx_kbd.h
