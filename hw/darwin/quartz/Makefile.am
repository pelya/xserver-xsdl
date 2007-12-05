noinst_LTLIBRARIES = libXQuartz.la

AM_CFLAGS = $(XSERVER_CFLAGS) $(DIX_CFLAGS)
AM_OBJCFLAGS = $(XSERVER_CFLAGS) $(DIX_CFLAGS)

# TODO: This should not pull in rootless... rootless should all be in xpr
AM_CPPFLAGS = \
	-I$(srcdir) -I$(srcdir)/.. \
	-I$(top_srcdir)/miext/rootless

if X11APP
X11APP_SUBDIRS = apple
endif

SUBDIRS = . xpr $(X11APP_SUBDIRS)
DIST_SUBDIRS = xpr apple

libXQuartz_la_SOURCES = \
	X11Application.m \
	X11Controller.m \
	applewm.c \
	keysym2ucs.c \
	pseudoramiX.c \
	quartz.c \
	quartzAudio.c \
	quartzCocoa.m \
	quartzKeyboard.c \
	quartzPasteboard.c \
	quartzStartup.c

EXTRA_DIST = \
	X11Application.h \
	X11Controller.h \
	applewmExt.h \
	keysym2ucs.h \
	pseudoramiX.h \
	quartzAudio.h \
	quartzCommon.h \
	quartzCursor.c \
	quartzCursor.h \
	quartz.h \
	quartzPasteboard.h
