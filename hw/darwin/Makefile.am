AM_CFLAGS = $(XORG_CFLAGS) $(DIX_CFLAGS)
AM_CPPFLAGS = $(XORG_INCS) \
	-DINXQUARTZ \
	-DUSE_NEW_CLUT \
	-DXFree86Server \
	-I$(top_srcdir)/miext/rootless

SUBDIRS = quartz utils

bin_PROGRAMS = Xquartz
man1_MANS = Xquartz.man

Xquartz_SOURCES = \
	darwin.c \
	darwinEvents.c \
	darwinKeyboard.c \
	darwinXinput.c \
	$(top_srcdir)/fb/fbcmap_mi.c \
	$(top_srcdir)/mi/miinitext.c

#		We should probably add these once they're working, or are these obsolete and to be removed?
#		./quartz/cr/libcr.a
#		./quartz/fullscreen/libfullscreen.a

Xquartz_LDADD = \
	./quartz/libXquartz.a \
	./quartz/xpr/libxpr.a \
	$(top_builddir)/dix/dixfonts.lo \
	$(top_builddir)/config/libconfig.a \
	$(top_builddir)/dix/libdix.la \
	$(top_builddir)/os/libos.la \
	$(top_builddir)/dix/libxpstubs.la \
	$(top_builddir)/miext/shadow/libshadow.la \
	$(top_builddir)/fb/libfb.la \
	$(top_builddir)/mi/libmi.la \
	$(top_builddir)/composite/libcomposite.la \
	$(top_builddir)/damageext/libdamageext.la \
	$(top_builddir)/miext/damage/libdamage.la \
	$(top_builddir)/xfixes/libxfixes.la \
	$(top_builddir)/miext/cw/libcw.la \
	$(top_builddir)/Xext/libXext.la \
	$(top_builddir)/xkb/libxkb.la \
	$(top_builddir)/xkb/libxkbstubs.la \
	$(top_builddir)/Xi/libXi.la \
	$(top_builddir)/dbe/libdbe.la \
	$(top_builddir)/record/librecord.la \
	$(top_builddir)/XTrap/libxtrap.la \
	$(top_builddir)/miext/rootless/librootless.la \
	$(top_builddir)/miext/rootless/safeAlpha/libsafeAlpha.la \
	$(top_builddir)/miext/rootless/accel/librlAccel.la \
	$(DARWIN_LIBS) $(XSERVER_LIBS) -lXplugin

Xquartz_LDFLAGS =  \
	-XCClinker -Objc \
	-Wl,-u,_miDCInitialize \
	-Wl,-framework,Carbon \
	-L/System/Library/Frameworks/OpenGL.framework/Libraries -lGL \
	-Wl,-framework,OpenGL \
	-Wl,-framework,Cocoa \
	-Wl,-framework,CoreAudio \
	-Wl,-framework,IOKit

if X11APP
bin_SCRIPTS = x11app x11launcher

x11app:
	cd apple && xcodebuild CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" ARCHS="$(X11APP_ARCHS)"

x11launcher:
	cd launcher && xcodebuild CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" ARCHS="$(X11APP_ARCHS)"

x11app-install:
	cd apple && xcodebuild install DSTROOT=$(DESTDIR) INSTALL_PATH=$(prefix) DEPLOYMENT_LOCATION=YES SKIP_INSTALL=NO ARCHS="$(X11APP_ARCHS)"

x11launcher-install:
	cd launcher && xcodebuild install DSTROOT=$(DESTDIR) INSTALL_PATH=$(APPLE_APPLICATIONS_DIR) DEPLOYMENT_LOCATION=YES SKIP_INSTALL=NO ARCHS="$(X11APP_ARCHS)"

x11app-clean:
	rm -rf apple/build

x11launcher-clean:
	rm -rf launcher/build

install-data-hook: x11app-install x11launcher-install
clean-local: x11app-clean x11launcher-clean

endif

EXTRA_DIST = \
	Xquartz.man \
	darwinClut8.h \
	darwin.h \
	darwinKeyboard.h \
	apple/Info.plist \
	apple/X11.icns \
	apple/bundle-main.c \
	apple/English.lproj/InfoPlist.strings \
	apple/English.lproj/Localizable.strings \
	apple/English.lproj/main.nib/classes.nib \
	apple/English.lproj/main.nib/info.nib \
	apple/English.lproj/main.nib/keyedobjects.nib \
	apple/X11.xcodeproj/project.pbxproj \
	launcher/bundle-main.c \
	launcher/Info.plist \
	launcher/X11.icns \
	launcher/X11.xcodeproj/project.pbxproj
