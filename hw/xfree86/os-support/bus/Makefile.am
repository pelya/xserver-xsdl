noinst_LTLIBRARIES = libbus.la
sdk_HEADERS = xf86Pci.h

PCI_SOURCES = @XORG_OS_PCI@Pci.c ix86Pci.c

if LINUX_IA64
PLATFORM_PCI_SOURCES = 460gxPCI.c altixPCI.c e8870PCI.c zx1PCI.c
endif

libbus_la_SOURCES = Pci.c $(PCI_SOURCES) $(PLATFORM_PCI_SOURCES)

if XORG_BUS_SBUS
	libbus_la_SOURCES += Sbus.c
endif

INCLUDES = $(XORG_INCS)

AM_CFLAGS = $(XORG_CFLAGS)

EXTRA_DIST = \
	460gxPCI.h \
	Pci.h \
	altixPCI.h \
	e8870PCI.h \
	zx1PCI.h

