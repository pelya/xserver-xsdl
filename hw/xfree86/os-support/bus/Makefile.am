noinst_LTLIBRARIES = libbus.la
sdk_HEADERS = xf86Pci.h

PCI_SOURCES = @XORG_OS_PCI@Pci.c 

if XORG_BUS_IX86PCI
PCI_SOURCES += ix86Pci.c
endif

if LINUX_IA64
PLATFORM_PCI_SOURCES = 460gxPCI.c altixPCI.c e8870PCI.c zx1PCI.c
endif

if XORG_BUS_SPARC
PLATFORM_PCI_SOURCES = sparcPci.c
PLATFORM_SOURCES = Sbus.c
endif

libbus_la_SOURCES = Pci.c $(PCI_SOURCES) $(PLATFORM_PCI_SOURCES) \
                    $(PLATFORM_SOURCES)

INCLUDES = $(XORG_INCS)

AM_CFLAGS = $(XORG_CFLAGS)

# hack to keep all the OS PCI support files in the distball even though
# there aren't rules to build them all yet.  also the AC_SUBST pattern
# above will defeat automake's EXTRA_DIST logic, woo.
PCIDISTSOURCES = \
	axpPci.c \
	freebsdPci.c \
	linuxPci.c \
	netbsdPci.c \
	ppcPci.c \
	sparcPci.c

EXTRA_DIST = \
	460gxPCI.h \
	Pci.h \
	altixPCI.h \
	e8870PCI.h \
	zx1PCI.h \
	xf86Sbus.h \
        $(PCIDISTSOURCES)
