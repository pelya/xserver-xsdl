# TODO: Fix VT support for pre-Solaris 8
#if defined(i386Architecture) && (OSMinorVersion < 8)
#IO_SRC   = $(srcdir)/../sysv/sysv_io.c
#KBD_SRCS = $(srcdir)/../sysv/sysv_kbd.c $(srcdir)/../shared/std_kbdEv.c
#VTSW_SRC = $(srcdir)/../shared/VTsw_usl.c
#else
IO_SRC   = sun_io.c
KBD_SRCS = sun_kbd.c sun_kbdEv.c
VTSW_SRC = $(srcdir)/../shared/VTsw_noop.c
#endif

# TODO: Don't build agpgart on SPARC
#if defined(i386Architecture) || defined(AMD64Architecture)
AGP_SRC = sun_agp.c
#else
#AGP_SRC = $(srcdir)/../shared/agp_noop.c
#endif

noinst_LIBRARIES = libsolaris.a
libsolaris_a_SOURCES = sun_bios.c sun_init.c $(IO_SRC) $(KBD_SRCS) \
	$(PROWORKS_INOUT_SRC) sun_mouse.c sun_vid.c $(AGP_SRC) \
	$(srcdir)/../shared/libc_wrapper.c \
	$(srcdir)/../shared/kmod_noop.c $(srcdir)/../shared/pm_noop.c \
	$(srcdir)/../shared/posix_tty.c $(srcdir)/../shared/sigiostubs.c \
	$(srcdir)/../shared/stdPci.c $(srcdir)/../shared/stdResource.c \
	$(VTSW_SRC)

sdk_HEADERS = agpgart.h

AM_CFLAGS = -DUSESTDRES -DHAVE_SYSV_IPC $(XORG_CFLAGS)

INCLUDES = $(XORG_INCS) 

EXTRA_DIST = sun_inout.s solaris-amd64.S solaris-ia32.S	solaris-sparcv8plus.S

