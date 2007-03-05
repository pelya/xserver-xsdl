BINDIR = ${bindir}
include $(top_srcdir)/cpprules.in
XINITDIR = $(libdir)/X11/xinit
XDEFS = \
        -DX_VERSION="$(PLIST_VERSION_STRING)" \
        -DX_PRE_RELEASE="$(PRE)" \
        -DX_REL_DATE="$(XORG_DATE)" \
        -DX_VENDOR_NAME="$(VENDOR_STRING)" \
        -DX_VENDOR_LINK="$(PLIST_VENDOR_WEB)"

SUBDIRS =  English.lproj Dutch.lproj French.lproj German.lproj Japanese.lproj \
	ko.lproj Portuguese.lproj Spanish.lproj Swedish.lproj

bin_SCRIPTS = startXClients

startXClients: $(srcdir)/startXClients.cpp
	$(RAWCPP) $(RAWCPPFLAGS) $(XDEFS) $(CPP_FILES_FLAGS) -DXINITDIR=$(XINITDIR) -DXBINDIR=$(BINDIR)  $< | $(CPP_SED_MAGIC) > $@
	-chmod 755 startXClients

contentsdir = @APPLE_APPLICATIONS_DIR@/XDarwin.app/Contents
resourcesdir = $(contentsdir)/Resources

contents_DATA = Info.plist
resources_DATA = XDarwin.icns startXClients

install-data-hook:
	chmod 755 $(DESTDIR)$(resourcesdir)/startXClients
	echo "APPL????" > $(DESTDIR)$(contentsdir)/PkgInfo
	touch $(DESTDIR)@APPLE_APPLICATIONS_DIR@/XDarwin.app

uninstall-hook:
	rm -rf $(DESTDIR)$(contentsdir)/PkgInfo

CLEANFILES = startXClients

EXTRA_DIST = \
	XDarwin.icns \
	Info.plist
