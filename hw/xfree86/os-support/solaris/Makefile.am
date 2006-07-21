if SOLARIS_USL_CONSOLE
IO_SRC   = $(srcdir)/../sysv/sysv_io.c
KBD_SRCS = $(srcdir)/../shared/sysv_kbd.c $(srcdir)/../shared/std_kbdEv.c
VTSW_SRC = $(srcdir)/../shared/VTsw_usl.c
else
IO_SRC   = sun_io.c sun_kbd.h
KBD_SRCS = sun_kbd.c sun_kbdEv.c sun_kbd.h
VTSW_SRC = $(srcdir)/../shared/VTsw_noop.c
endif

# TODO: Don't build agpgart on SPARC
#if defined(i386Architecture) || defined(AMD64Architecture)
AGP_SRC = sun_agp.c
#else
#AGP_SRC = $(srcdir)/../shared/agp_noop.c
#endif

SOLARIS_INOUT_SRC = solaris-$(SOLARIS_INOUT_ARCH).S
DISTCLEANFILES = solaris-$(SOLARIS_INOUT_ARCH).il

solaris-$(SOLARIS_INOUT_ARCH).il: solaris-${SOLARIS_INOUT_ARCH}.S
	$(CPP) -P -DINLINE_ASM solaris-${SOLARIS_INOUT_ARCH}.S > $@

noinst_LTLIBRARIES = libsolaris.la
libsolaris_la_SOURCES = sun_bios.c sun_init.c $(IO_SRC) $(KBD_SRCS) \
	sun_mouse.c sun_vid.c $(AGP_SRC) \
	$(srcdir)/../shared/libc_wrapper.c \
	$(srcdir)/../shared/kmod_noop.c $(srcdir)/../shared/pm_noop.c \
	$(srcdir)/../shared/posix_tty.c $(srcdir)/../shared/sigiostubs.c \
	$(srcdir)/../shared/stdPci.c $(srcdir)/../shared/stdResource.c \
	$(VTSW_SRC)
nodist_libsolaris_la_SOURCES = $(SOLARIS_INOUT_SRC)

sdk_HEADERS = agpgart.h
nodist_sdk_HEADERS = solaris-$(SOLARIS_INOUT_ARCH).il

AM_CFLAGS = -DUSESTDRES -DHAVE_SYSV_IPC $(XORG_CFLAGS)

INCLUDES = $(XORG_INCS) 

EXTRA_DIST = solaris-amd64.S solaris-ia32.S solaris-sparcv8plus.S \
	apSolaris.shar sun_inout.s
