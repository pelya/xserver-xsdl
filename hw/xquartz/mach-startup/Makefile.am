AM_CPPFLAGS = \
	-DBUILD_DATE=\"$(BUILD_DATE)\" \
	-DXSERVER_VERSION=\"$(VERSION)\"

x11appdir = $(APPLE_APPLICATIONS_DIR)/X11.app/Contents/MacOS
x11app_PROGRAMS = X11

X11_SOURCES = \
	bundle-main.c

X11_LDADD = \
	$(top_builddir)/hw/xquartz/libXquartz.la \
	$(top_builddir)/hw/xquartz/xpr/libXquartzXpr.la \
	$(top_builddir)/dix/dixfonts.lo \
	$(top_builddir)/miext/rootless/librootless.la \
	$(DARWIN_LIBS) $(XSERVER_LIBS) $(XSERVER_SYS_LIBS) -lXplugin -lX11

X11_LDFLAGS =  \
	-XCClinker -Objc \
	-Wl,-u,_miDCInitialize \
	-Wl,-framework,Carbon \
	-L/System/Library/Frameworks/OpenGL.framework/Libraries -lGL \
	-Wl,-framework,OpenGL \
	-Wl,-framework,Cocoa \
	-Wl,-framework,CoreAudio \
	-Wl,-framework,IOKit

bin_PROGRAMS = Xquartz

Xquartz_SOURCES = \
	stub.c

Xquartz_LDFLAGS =  \
	-Wl,-framework,CoreServices
