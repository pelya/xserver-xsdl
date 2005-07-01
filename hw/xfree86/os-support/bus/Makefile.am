noinst_LIBRARIES = libbus.a
sdk_HEADERS = xf86Pci.h

PCI_SOURCES = @XORG_OS@Pci.c ix86Pci.c

libbus_a_SOURCES = Pci.c $(PCI_SOURCES)

if XORG_BUS_SBUS
	libbus_a_SOURCES += Sbus.c
endif

INCLUDES = $(XORG_INCS)

EXTRA_DIST = \
	460gxPCI.h \
	Pci.h \
	altixPCI.h \
	e8870PCI.h \
	zx1PCI.h
