AM_CFLAGS = $(XSERVER_CFLAGS) $(DIX_CFLAGS)
AM_CPPFLAGS = \
	-DINXQUARTZ \
	-DUSE_NEW_CLUT \
	-DXFree86Server \
	-I$(top_srcdir)/miext/rootless

if X11APP
X11APP_SUBDIRS = apple
endif

SUBDIRS = quartz utils $(X11APP_SUBDIRS)
DIST_SUBDIRS = quartz utils apple launcher

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

EXTRA_DIST = \
	Xquartz.man \
	darwinClut8.h \
	darwin.h \
	darwinKeyboard.h
