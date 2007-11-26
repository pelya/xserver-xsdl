bin_SCRIPTS = x11app

.PHONY: x11app

x11app:
	xcodebuild CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" ARCHS="$(X11APP_ARCHS)"

install-data-hook:
	xcodebuild install DSTROOT=$(DESTDIR) INSTALL_PATH=$(prefix) DEPLOYMENT_LOCATION=YES SKIP_INSTALL=NO ARCHS="$(X11APP_ARCHS)"

clean-local:
	rm -rf build

EXTRA_DIST = \
	Info.plist \
	X11.icns \
	bundle-main.c \
	English.lproj/InfoPlist.strings \
	English.lproj/Localizable.strings \
	English.lproj/main.nib/classes.nib \
	English.lproj/main.nib/info.nib \
	English.lproj/main.nib/keyedobjects.nib \
	X11.xcodeproj/project.pbxproj
