bin_PROGRAMS = Xquartz

# TODO: This man page needs sed magic and cleanup
man1_MANS = Xquartz.man

AM_CFLAGS =  $(XSERVER_CFLAGS) $(DIX_CFLAGS)
AM_CPPFLAGS = \
	-I$(srcdir) -I$(srcdir)/.. \
	-I$(top_srcdir)/miext \
	-I$(top_srcdir)/miext/rootless \
	-I$(top_srcdir)/miext/rootless/safeAlpha

Xquartz_SOURCES = \
	appledri.c \
	dri.c \
	xprAppleWM.c \
	xprCursor.c \
	xprFrame.c \
	xprScreen.c \
	x-hash.c \
	x-hook.c \
	x-list.c

Xquartz_LDADD = \
	$(top_builddir)/hw/xquartz/libXquartz.la \
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
	dri.h \
	dristruct.h \
	appledri.h \
	appledristr.h \
	x-hash.h \
	x-hook.h \
	x-list.h \
	Xplugin.h \
	xpr.h
