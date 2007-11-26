bin_SCRIPTS = x11launcher

.PHONY: x11launcher

x11launcher:
	xcodebuild CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" ARCHS="$(X11APP_ARCHS)"

install-data-hook:
	xcodebuild install DSTROOT=$(DESTDIR) INSTALL_PATH=$(APPLE_APPLICATIONS_DIR) DEPLOYMENT_LOCATION=YES SKIP_INSTALL=NO ARCHS="$(X11APP_ARCHS)"

clean-local:
	rm -rf build

EXTRA_DIST = \
	bundle-main.c \
	Info.plist \
	X11.icns \
	X11.xcodeproj/project.pbxproj
