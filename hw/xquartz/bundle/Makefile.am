bin_SCRIPTS = x11app

.PHONY: x11app

x11app:
	xcodebuild CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" ARCHS="$(X11APP_ARCHS)"

install-data-hook:
	xcodebuild install DSTROOT="/$(DESTDIR)" INSTALL_PATH="$(APPLE_APPLICATIONS_DIR)" DEPLOYMENT_LOCATION=YES SKIP_INSTALL=NO ARCHS="$(X11APP_ARCHS)"

if LAUNCHD
launchagents_PRE = org.x.X11.plist.pre
launchagents_DATA = $(launchagents_PRE:plist.pre=plist)

CPP_FILES_FLAGS = -D__libexecdir__="${libexecdir}"

CLEANFILES = $(launchagents_DATA)

include $(top_srcdir)/cpprules.in
endif

clean-local:
	rm -rf build

resourcedir=$(libdir)/X11/xserver
resource_DATA = Xquartz.plist

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
