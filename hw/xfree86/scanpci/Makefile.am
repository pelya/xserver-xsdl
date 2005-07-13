module_LTLIBRARIES = libscanpci.la libpcidata.la

libpcidata_la_LDFLAGS = -avoid-version
libscanpci_la_LDFLAGS = -avoid-version

libpcidata_la_SOURCES = xf86PciData.c
libscanpci_la_SOURCES = xf86ScanPci.c

INCLUDES = $(XORG_INCS)

AM_CFLAGS = $(XORG_CFLAGS)

BUILT_SOURCES = xf86PciIds.h

EXTRA_DIST = \
	xf86PciData.h \
	xf86PciIds.h \
	xf86PciStdIds.h \
	xf86PciStr.h \
	xf86ScanPci.h

xf86PciData.c:
	echo "#define PCIDATA" > $@
	echo "#include \"$(srcdir)/xf86ScanPci.c\"" >> $@

xf86PciIds.h: $(srcdir)/../common/xf86PciInfo.h
	$(PERL) $(srcdir)/pciid2c.pl $(srcdir)/../common/xf86PciInfo.h < $(srcdir)/pci.ids > xf86PciIds.h

DISTCLEANFILES = xf86PciData.c xf86PciIds.h
