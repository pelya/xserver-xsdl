if DRI
DRI_SUBDIRS=drm
DRI_LIBS=drm/libxorglinuxdrm.a
endif

noinst_LIBRARIES = liblinux.a
liblinux_a_SOURCES = lnx_init.c lnx_video.c lnx_io.c lnx_kbd.c lnx_mouse.c \
                     lnx_pci.c lnx_agp.c lnx_kmod.c lnx_KbdMap.c \
                     $(srcdir)/../shared/bios_mmap.c \
		     $(srcdir)/../shared/VTsw_usl.c \
                     $(srcdir)/../shared/std_kbdEv.c \
		     $(srcdir)/../shared/posix_tty.c \
                     $(srcdir)/../shared/vidmem.c \
		     $(srcdir)/../shared/sigio.c \
                     $(srcdir)/../shared/stdResource.c \
		     $(srcdir)/../shared/libc_wrapper.c \
                     $(srcdir)/../shared/at_scancode.c

sdk_HEADERS = agpgart.h

AM_CFLAGS = -DUSESTDRES -DHAVE_SYSV_IPC $(XORG_CFLAGS)

SUBDIRS = $(DRI_SUBDIRS)

INCLUDES = $(XORG_INCS) -I/usr/include/drm # FIXME this last part is crack

EXTRA_DIST = \
	lnx.h \
	lnx_kbd.h
