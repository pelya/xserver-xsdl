AM_CPPFLAGS = \
	-DBUILD_DATE=\"$(BUILD_DATE)\" \
	-DXSERVER_VERSION=\"$(VERSION)\" \
	-DMACHO_STARTUP

x11appdir = $(APPLE_APPLICATIONS_DIR)/X11.app/Contents/MacOS
x11app_PROGRAMS = X11

dist_X11_SOURCES = \
	bundle-main.c 
#	launchd_fd.c

nodist_X11_SOURCES = \
	mach_startupServer.c \
	mach_startupUser.c

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

dist_Xquartz_SOURCES = \
	stub.c \
	launchd_fd.c

nodist_Xquartz_SOURCES = \
	mach_startupUser.c

Xquartz_LDFLAGS =  \
	-Wl,-framework,CoreServices

BUILT_SOURCES = \
	mach_startupServer.c \
	mach_startupUser.c \
	mach_startupServer.h \
	mach_startup.h

CLEANFILES = \
	$(BUILT_SOURCES)

$(BUILT_SOURCES): mach_startup.defs
	mig -sheader mach_startupServer.h mach_startup.defs

EXTRA_DIST = \
	launchd_fd.h \
	mach_startup.defs \
	mach_startup_types.h

