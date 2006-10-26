noinst_LTLIBRARIES = liblinux.la

if LINUX_IA64
PLATFORM_PCI_SUPPORT = $(srcdir)/lnx_ia64.c $(srcdir)/../shared/ia64Pci.c
PLATFORM_DEFINES = -DOS_PROBE_PCI_CHIPSET=lnxProbePciChipset
PLATFORM_INCLUDES = -I$(srcdir)/../shared
endif
if LINUX_ALPHA
PLATFORM_PCI_SUPPORT = lnx_ev56.c \
       $(srcdir)/lnx_axp.c \
       $(srcdir)/../shared/xf86Axp.c
endif

if LNXACPI
ACPI_SOURCES = lnx_acpi.c lnx_apm.c
XORG_CFLAGS += -DHAVE_ACPI
endif

if LNXAPM
APM_SOURCES = lnx_apm.c
XORG_CFLAGS += -DHAVE_APM
endif

liblinux_la_SOURCES = lnx_init.c lnx_video.c lnx_io.c lnx_kbd.c lnx_mouse.c \
                     lnx_pci.c lnx_agp.c lnx_kmod.c lnx_KbdMap.c \
                     $(srcdir)/../shared/bios_mmap.c \
		     $(srcdir)/../shared/VTsw_usl.c \
                     $(srcdir)/../shared/std_kbdEv.c \
		     $(srcdir)/../shared/posix_tty.c \
                     $(srcdir)/../shared/vidmem.c \
		     $(srcdir)/../shared/sigio.c \
                     $(srcdir)/../shared/stdResource.c \
		     $(srcdir)/../shared/libc_wrapper.c \
                     $(srcdir)/../shared/at_scancode.c \
                     $(ACPI_SOURCES) \
                     $(APM_SOURCES) \
                     $(PLATFORM_PCI_SUPPORT)

AM_CFLAGS = -DUSESTDRES -DHAVE_SYSV_IPC $(XORG_CFLAGS) $(PLATFORM_DEFINES)

INCLUDES = $(XORG_INCS) $(PLATFORM_INCLUDES) -I/usr/include/drm # FIXME this last part is crack

# FIXME: These need to be added to the build
LNX_EXTRA_SOURCES = \
	lnx_font.c \
	lnx_jstk.c \
	lnxResource.c

EXTRA_DIST = \
	$(LNX_EXTRA_SOURCES) \
	lnx.h \
	lnx_kbd.h \
	$(srcdir)/../shared/xf86Axp.h
