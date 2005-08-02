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

# TODO: Support AMD64 & SPARC, and generate inline assembly .il files
#  if defined(AMD64Architecture)
#PROWORKS_ASM_ARCH  =  amd64
#PROWORKS_ASM_FLAGS =  -xarch=amd64
#  elif defined(i386Architecture)
PROWORKS_ASM_ARCH  =  ia32
PROWORKS_ASM_FLAGS =
#  elif defined(SparcArchitecture) && !defined(Sparc64Architecture)
#PROWORKS_ASM_ARCH  =  sparcv8plus
#PROWORKS_ASM_FLAGS =  -xarch=v8plus
#  else
#   error "Unsupported Architecture for building with Sun Compilers"
#  endif
PROWORKS_INOUT_SRC = solaris-$(PROWORKS_ASM_ARCH).S

#all:: $(PROWORKS_ASM_ARCH).il
#includes:: $(PROWORKS_ASM_ARCH).il

#CppFileTarget($(PROWORKS_ASM_ARCH).il,solaris-$(PROWORKS_ASM_ARCH).S,-DINLINE_ASM,NullParameter)

$(PROWORKS_INOUT_OBJ): $(PROWORKS_INOUT_SRC)
	$(AS) -P -o $@ $(PROWORKS_ASM_FLAGS) $(PROWORKS_INOUT_SRC)

noinst_LTLIBRARIES = libsolaris.la
libsolaris_la_SOURCES = sun_bios.c sun_init.c $(IO_SRC) $(KBD_SRCS) \
	sun_mouse.c sun_vid.c $(AGP_SRC) \
	$(srcdir)/../shared/libc_wrapper.c \
	$(srcdir)/../shared/kmod_noop.c $(srcdir)/../shared/pm_noop.c \
	$(srcdir)/../shared/posix_tty.c $(srcdir)/../shared/sigiostubs.c \
	$(srcdir)/../shared/stdPci.c $(srcdir)/../shared/stdResource.c \
	$(VTSW_SRC) $(PROWORKS_INOUT_SRC)

sdk_HEADERS = agpgart.h

AM_CFLAGS = -DUSESTDRES -DHAVE_SYSV_IPC $(XORG_CFLAGS)

INCLUDES = $(XORG_INCS) 

EXTRA_DIST = sun_inout.s solaris-amd64.S solaris-ia32.S	solaris-sparcv8plus.S

