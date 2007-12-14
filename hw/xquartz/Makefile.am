noinst_LTLIBRARIES = libXquartz.la
AM_CFLAGS = $(XSERVER_CFLAGS) $(DIX_CFLAGS)
AM_CPPFLAGS = \
	-DBUILD_DATE=\"$(BUILD_DATE)\" \
	-DINXQUARTZ \
	-DUSE_NEW_CLUT \
	-DXFree86Server \
	-I$(top_srcdir)/miext/rootless

if X11APP
X11APP_SUBDIRS = bundle

if LAUNCHD
libexec_PROGRAMS = x11-exec
x11_exec_LDFLAGS = -framework ApplicationServices
endif

endif

SUBDIRS = . xpr $(X11APP_SUBDIRS)
DIST_SUBDIRS = xpr bundle

libXquartz_la_SOURCES = \
	$(top_srcdir)/fb/fbcmap.c \
	$(top_srcdir)/mi/miinitext.c \
	X11Application.m \
	X11Controller.m \
	applewm.c \
	darwin.c \
	darwinEvents.c \
	darwinKeyboard.c \
	darwinXinput.c \
	keysym2ucs.c \
	pseudoramiX.c \
	quartz.c \
	quartzAudio.c \
	quartzCocoa.m \
	quartzForeground.c \
	quartzKeyboard.c \
	quartzPasteboard.c \
	quartzStartup.c

EXTRA_DIST = \
	X11Application.h \
	X11Controller.h \
	applewmExt.h \
	darwinClut8.h \
	darwin.h \
	darwinEvents.h \
	darwinKeyboard.h \
	keysym2ucs.h \
	pseudoramiX.h \
	quartz.h \
	quartzAudio.h \
	quartzCommon.h \
	quartzCursor.c \
	quartzCursor.h \
	quartzForeground.h \
	quartzKeyboard.h \
	quartzPasteboard.h
