bin_PROGRAMS = Xquartz

AM_CFLAGS =  $(XSERVER_CFLAGS) $(DIX_CFLAGS)
AM_CPPFLAGS = \
	-I$(srcdir) -I$(srcdir)/.. \
	-I$(top_srcdir)/miext \
	-I$(top_srcdir)/miext/rootless

Xquartz_SOURCES = \
	appledri.c \
	dri.c \
	xprAppleWM.c \
	xprCursor.c \
	xprEvent.c \
	xprFrame.c \
	xprScreen.c \
	x-hash.c \
	x-hook.c \
	x-list.c

Xquartz_LDADD = \
	$(top_builddir)/hw/xquartz/libXquartz.la \
	$(top_builddir)/dix/dixfonts.lo \
	$(top_builddir)/miext/rootless/librootless.la \
	$(DARWIN_LIBS) $(XSERVER_LIBS) $(XSERVER_SYS_LIBS) -lXplugin

Xquartz_LDFLAGS =  \
	-XCClinker -Objc \
	-Wl,-u,_miDCInitialize \
	-Wl,-framework,Carbon \
	-L/System/Library/Frameworks/OpenGL.framework/Libraries -lGL \
	-Wl,-framework,OpenGL \
	-Wl,-framework,Cocoa \
	-Wl,-framework,CoreAudio \
	-Wl,-framework,IOKit

appmandir = $(APP_MAN_DIR)
appman_PRE = Xquartz.man.pre
appman_PROCESSED = $(appman_PRE:man.pre=man)
appman_DATA = $(appman_PRE:man.pre=@APP_MAN_SUFFIX@)

CLEANFILES = $(appman_PROCESSED) $(appman_DATA)

include $(top_srcdir)/cpprules.in

.man.$(APP_MAN_SUFFIX):
	cp $< $@

EXTRA_DIST = \
	Xquartz.man.pre \
	dri.h \
	dristruct.h \
	appledri.h \
	appledristr.h \
	x-hash.h \
	x-hook.h \
	x-list.h \
	xpr.h \
	xprEvent.h
