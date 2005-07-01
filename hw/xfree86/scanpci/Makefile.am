module_LTLIBRARIES = libscanpci.la libpcidata.la

libpcidata_la_SOURCES = xf86PciData.c
libscanpci_la_SOURCES = xf86ScanPci.c

INCLUDES = $(XORG_INCS)

EXTRA_DIST = \
	xf86PciData.h \
	xf86PciIds.h \
	xf86PciStdIds.h \
	xf86PciStr.h \
	xf86ScanPci.h

xf86PciData.c:
	echo "#define PCIDATA" > $@
	echo "#include \"$(srcdir)/xf86ScanPci.c\"" >> $@

DISTCLEANFILES = xf86PciData.c
