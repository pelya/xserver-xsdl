bin_SCRIPTS = x11app

.PHONY: x11app

x11app:
	xcodebuild CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" ARCHS="$(X11APP_ARCHS)"

install-data-hook:
	xcodebuild install DSTROOT="/$(DESTDIR)" INSTALL_PATH="$(APPLE_APPLICATIONS_DIR)" DEPLOYMENT_LOCATION=YES SKIP_INSTALL=NO ARCHS="$(X11APP_ARCHS)"
	$(MKDIR_P) "$(DESTDIR)/System/Library/LaunchAgents/"
	$(INSTALL) org.x.X11.plist "$(DESTDIR)/System/Library/LaunchAgents/"

clean-local:
	rm -rf build

EXTRA_DIST = \
	org.x.X11.plist \
	Info.plist \
	X11.icns \
	bundle-main.c \
	launcher-main.c \
	server-main.c \
	English.lproj/InfoPlist.strings \
	English.lproj/Localizable.strings \
	English.lproj/main.nib/classes.nib \
	English.lproj/main.nib/info.nib \
	English.lproj/main.nib/keyedobjects.nib \
	X11.xcodeproj/project.pbxproj
