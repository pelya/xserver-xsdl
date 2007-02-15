BINDIR = ${bindir}
include $(top_srcdir)/cpprules.in
XINITDIR = $(libdir)/X11/xinit
XDEFS = \
        -DX_VERSION="$(PLIST_VERSION_STRING)" \
        -DX_PRE_RELEASE="$(PRE)" \
        -DX_REL_DATE="$(XORG_DATE)" \
        -DX_VENDOR_NAME="$(VENDOR_STRING)" \
        -DX_VENDOR_LINK="$(PLIST_VENDOR_WEB)"


resourcesdir = @APPLE_APPLICATIONS_DIR@/XDarwin.app/Contents/Resources

Japaneselprojdir = $(resourcesdir)/Japanese.lproj

Japaneselproj_DATA = \
		    XDarwinHelp.html \
		    InfoPlist.strings \
		    Credits.rtf Localizable.strings

Japaneselprojnibdir = $(Japaneselprojdir)/MainMenu.nib
Japaneselprojnib_DATA = MainMenu.nib/classes.nib MainMenu.nib/objects.nib

InfoPlist.strings: $(srcdir)/../English.lproj/InfoPlist.strings.cpp
	 $(RAWCPP) $(RAWCPPFLAGS) $(XDEFS) $(CPP_FILES_FLAGS) $< | $(CPP_SED_MAGIC) | $(SED) -e's/__quote__/"/g' > $@

XDarwinHelp.html: XDarwinHelp.html.cpp
	$(RAWCPP) $(RAWCPPFLAGS) $(XDEFS) $(CPP_FILES_FLAGS) $< | $(CPP_SED_MAGIC) > $@

CLEANFILES = XDarwinHelp.html InfoPlist.strings 

EXTRA_DIST = \
	Credits.rtf Localizable.strings \
	Localizable.strings \
	MainMenu.nib/classes.nib \
	MainMenu.nib/objects.nib \
	XDarwinHelp.html.cpp
