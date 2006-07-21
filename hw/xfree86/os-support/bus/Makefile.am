noinst_LTLIBRARIES = libbus.la
sdk_HEADERS = xf86Pci.h

PCI_SOURCES =

if XORG_BUS_LINUXPCI
PCI_SOURCES += linuxPci.c
endif

if XORG_BUS_FREEBSDPCI
PCI_SOURCES += freebsdPci.c
endif

if XORG_BUS_NETBSDPCI
PCI_SOURCES += netbsdPci.c
endif

if XORG_BUS_IX86PCI
PCI_SOURCES += ix86Pci.c
endif

if XORG_BUS_PPCPCI
PCI_SOURCES += ppcPci.c
endif

if XORG_BUS_SPARCPCI
PCI_SOURCES += sparcPci.c
endif

if LINUX_ALPHA
PCI_SOURCES += axpPci.c
endif

if LINUX_IA64
PLATFORM_PCI_SOURCES = \
	460gxPCI.c \
	460gxPCI.h \
	altixPCI.c \
	altixPCI.h \
	e8870PCI.c \
	e8870PCI.h \
	zx1PCI.c \
	zx1PCI.h
endif

if XORG_BUS_SPARC
PLATFORM_SOURCES = Sbus.c
sdk_HEADERS += xf86Sbus.h
endif

libbus_la_SOURCES = Pci.c Pci.h $(PCI_SOURCES) $(PLATFORM_PCI_SOURCES) \
                    $(PLATFORM_SOURCES)

INCLUDES = $(XORG_INCS)

AM_CFLAGS = $(XORG_CFLAGS)

EXTRA_DIST = $(sdk_HEADERS)
