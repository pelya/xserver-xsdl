noinst_LIBRARIES = libdarwinShared.a
libdarwin_XINPUT_SRCS = darwinXinput.c

AM_CFLAGS = @XORG_CFLAGS@ @DIX_CFLAGS@
AM_CPPFLAGS = @XORG_CFLAGS@ @DIX_CFLAGS@
INCLUDES = @XORG_INCS@ -I../../miext/rootless

DEFS = @DEFS@ -DUSE_NEW_CLUT

if XQUARTZ
XQUARTZ_SUBDIRS = bundle quartz
endif

SUBDIRS = \
	  iokit \
	  $(XQUARTZ_SUBDIRS) \
	  utils \
	  .


darwinappdir = @APPLE_APPLICATIONS_DIR@/XDarwin.app

libdarwinShared_a_SOURCES = darwin.c \
			  darwinEvents.c \
			  darwinKeyboard.c \
			  $(darwin_XINPUT_SRCS) 

bin_PROGRAMS = XDarwin Xquartz
XDarwin_SOURCES = \
                  $(top_srcdir)/fb/fbcmap.c \
                  $(top_srcdir)/mi/miinitext.c \
                  $(top_srcdir)/Xi/stubs.c

Xquartz_SOURCES = \
                  $(top_srcdir)/fb/fbcmap.c \
                  $(top_srcdir)/mi/miinitext.c \
                  $(top_srcdir)/Xi/stubs.c \
                  apple/X11Application.m \
                  apple/X11Controller.m \
                  quartz/XServer.m \
                  quartz/Preferences.m \
                  quartz/applewm.c \
                  quartz/keysym2ucs.c \
                  quartz/pseudoramiX.c \
                  quartz/quartz.c \
                  quartz/quartzAudio.c \
                  quartz/quartzCocoa.m \
                  quartz/quartzKeyboard.c \
                  quartz/quartzPasteboard.c \
                  quartz/quartzStartup.c \
                  quartz/xpr/appledri.c \
                  quartz/xpr/dri.c \
                  quartz/xpr/xprAppleWM.c \
                  quartz/xpr/xprCursor.c \
                  quartz/xpr/xprFrame.c \
                  quartz/xpr/xprScreen.c \
                  quartz/xpr/x-hash.c \
                  quartz/xpr/x-hook.c \
                  quartz/xpr/x-list.c

XDarwin_LDADD = \
		$(top_builddir)/dix/dixfonts.lo \
		$(top_builddir)/dix/libdix.la \
		$(top_builddir)/config/libconfig.a \
		$(top_builddir)/os/libos.la \
		./libdarwinShared.a \
		./iokit/libiokit.a \
		$(top_builddir)/dix/libxpstubs.la \
		$(top_builddir)/miext/shadow/libshadow.la \
		$(top_builddir)/fb/libfb.la \
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
		$(XGLX_LIBS) \
		$(top_builddir)/miext/rootless/librootless.la \
		$(top_builddir)/miext/rootless/safeAlpha/libsafeAlpha.la \
		$(top_builddir)/miext/rootless/accel/librlAccel.la \
		@XORG_LIBS@ \
		-lXau -lXdmcp -lXfont -lfreetype

Xquartz_LDADD = \
		$(top_builddir)/dix/dixfonts.lo \
		$(top_builddir)/dix/libdix.la \
		$(top_builddir)/os/libos.la \
		./libdarwinShared.a \
		$(top_builddir)/dix/libxpstubs.la \
		$(top_builddir)/miext/shadow/libshadow.la \
		$(top_builddir)/fb/libfb.la \
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
		@XORG_LIBS@ \
		-lXau -lXdmcp -lXfont -lfreetype -lXplugin

XDarwin_LDFLAGS =  \
                 -XCClinker -Objc \
                 -Wl,-u,_miDCInitialize \
                 -Wl,-framework,IOKit

Xquartz_LDFLAGS =  \
                 -XCClinker -Objc \
                 -Wl,-u,_miDCInitialize \
                 -Wl,-framework,Carbon \
                 -Wl,-framework,ApplicationServices \
                 -Wl,-framework,Cocoa \
                 -Wl,-framework,CoreAudio \
                 -Wl,-framework,IOKit

XDarwin_CFLAGS = -DINXDARWIN
Xquartz_CFLAGS = -DINXQUARTZ -DHAS_CG_MACH_PORT -DHAS_KL_API  -DHAVE_XORG_CONFIG_H

if XQUARTZ
macosdir = $(darwinappdir)/Contents/MacOS

DEFS += -DDARWIN_WITH_QUARTZ -DXFree86Server

macos_PROGRAMS = XDarwinApp
macos_SCRIPTS = x11app

x11app:
	cd apple && xcodebuild

XDarwinApp_SOURCES = \
                $(top_srcdir)/fb/fbcmap.c \
                $(top_srcdir)/mi/miinitext.c \
		$(top_srcdir)/Xi/stubs.c 

XDarwinApp_LDADD = \
		$(top_builddir)/dix/dixfonts.lo \
		$(top_builddir)/dix/libdix.la \
		$(top_builddir)/config/libconfig.a \
		$(top_builddir)/os/libos.la \
		./quartz/XApplication.o \
		./libdarwinShared.a \
		./quartz/libXQuartz.a \
		$(top_builddir)/dix/libxpstubs.la \
		 $(top_builddir)/miext/shadow/libshadow.la \
		 $(top_builddir)/fb/libfb.la \
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
		 $(XGLX_LIBS) \
		 $(top_builddir)/miext/rootless/librootless.la \
		 $(top_builddir)/miext/rootless/safeAlpha/libsafeAlpha.la \
		 $(top_builddir)/miext/rootless/accel/librlAccel.la \
		 @XORG_LIBS@ \
		 -lXau -lXdmcp -lXfont -lfreetype \
		 $(top_builddir)/fb/libfb.la 

XDarwinApp_LDFLAGS =  \
		 -XCClinker -Objc \
		 -Wl,-u,_miDCInitialize \
		 -Wl,-framework,Carbon \
		 -Wl,-framework,ApplicationServices \
		 -Wl,-framework,Cocoa \
		 -Wl,-framework,CoreAudio \
		 -Wl,-framework,IOKit

XDarwinApp_CFLAGS = -DINXDARWINAPP
HOOK_TARGETS = xquartz-install-hook


crplugindir = $(darwinappdir)/Contents/Resources/cr.bundle/Contents/MacOS
crplugin_LTLIBRARIES = cr.la
cr_la_SOURCES = \
		quartz/cr/crAppleWM.m \
		quartz/cr/crFrame.m \
		quartz/cr/crScreen.m \
		quartz/fullscreen/quartzCursor.c \
		quartz/cr/XView.m

cr_la_LIBADD = \
		$(top_builddir)/miext/rootless/librootless.la \
		$(top_builddir)/miext/rootless/safeAlpha/libsafeAlpha.la \
		$(top_builddir)/miext/rootless/accel/librlAccel.la

cr_la_LDFLAGS = -shrext '' -Wl,-framework,ApplicationServices \
		-Wl,-framework,Cocoa \
		-Wl,-framework,Carbon \
		-XCClinker -ObjC \
		-XCClinker -bundle_loader -XCClinker XDarwinApp \
		-module -avoid-version -no-undefined
cr_la_DEPENDENCIES = XDarwinApp	

fullscreenplugindir = $(darwinappdir)/Contents/Resources/fullscreen.bundle/Contents/MacOS
fullscreenplugin_LTLIBRARIES = fullscreen.la
fullscreen_la_SOURCES = \
		quartz/fullscreen/fullscreen.c \
		quartz/fullscreen/quartzCursor.c

fullscreen_la_LIBADD = \
		$(top_builddir)/miext/shadow/libshadow.la

fullscreen_la_LDFLAGS = -shrext '' -Wl,-framework,ApplicationServices \
		-XCClinker -bundle_loader -XCClinker XDarwinApp \
		-module -avoid-version -no-undefined
fullscreen_la_DEPENDENCIES = XDarwinApp	

if GLX
glxMesaplugindir = $(darwinappdir)/Contents/Resources/glxMesa.bundle/Contents/MacOS
glxMesaplugin_LTLIBRARIES = glxMesa.la
glxMesa_la_SOURCES =
glxMesa_la_LIBADD = \
		$(top_builddir)/GL/glx/libglx.la \
		$(top_builddir)/GL/mesa/libGLcore.la 
glxMesa_la_LDFLAGS = -shrext '' \
		-Wl,-framework,AGL \
		-Wl,-framework,OpenGL \
		-XCClinker -ObjC \
		-XCClinker -bundle_loader -XCClinker XDarwinApp \
		-module -avoid-version -no-undefined
glxMesa_la_DEPENDENCIES = XDarwinApp	
endif

endif
if HAVE_X_PLUGIN

xprplugindir = $(darwinappdir)/Contents/Resources/xpr.bundle/Contents/MacOS
xprplugin_LTLIBRARIES = xpr.la
xpr_la_SOURCES = \
		quartz/xpr/appledri.c \
		quartz/xpr/dri.c \
		quartz/xpr/xprAppleWM.c \
		quartz/xpr/xprCursor.c \
		quartz/xpr/xprFrame.c \
		quartz/xpr/xprScreen.c \
		quartz/xpr/x-hash.c \
		quartz/xpr/x-hook.c \
		quartz/xpr/x-list.c

xpr_la_LIBADD = \
		$(top_builddir)/miext/rootless/librootless.la \
		$(top_builddir)/miext/rootless/safeAlpha/libsafeAlpha.la \
		$(top_builddir)/miext/rootless/accel/librlAccel.la

xpr_la_LDFLAGS = -shrext '' -Wl,-framework,ApplicationServices \
		-lXplugin \
		-XCClinker -bundle_loader -XCClinker XDarwinApp \
		-module -avoid-version -no-undefined
xpr_la_DEPENDENCIES = XDarwinApp	

endif

if HAVE_AGL_FRAMEWORK
glxCGLplugindir = $(darwinappdir)/Contents/Resources/glxCGL.bundle/Contents/MacOS
glxCGLplugin_LTLIBRARIES = glxCGL.la
glxCGL_la_SOURCES =
glxCGL_la_LIBADD = \
		$(top_builddir)/GL/glx/glxext.o \
		$(top_builddir)/GL/glx/libglx.a \
		$(top_builddir)/GL/apple/libAGLcore.a
glxCGL_la_LDFLAGS = -shrext '' -Wl,-framework,ApplicationServices \
		-Wl,-framework,AGL \
		-Wl,-framework,OpenGL \
		-XCClinker -ObjC \
		-XCClinker -bundle_loader -XCClinker XDarwinApp \
		-module -avoid-version -no-undefined
glxCGL_la_DEPENDENCIES = XDarwinApp	


glxAGLplugindir = $(darwinappdir)/Contents/Resources/glxAGL.bundle/Contents/MacOS
glxAGLplugin_LTLIBRARIES = glxAGL.la
glxAGL_la_SOURCES =
glxAGL_la_LIBADD = \
		$(top_builddir)/GL/glx/glxext.o \
		$(top_builddir)/GL/glx/libglx.a \
		$(top_builddir)/GL/apple/libAGLcore.a
glxAGL_la_LDFLAGS = -shrext '' \
		-Wl,-framework,AGL \
		-Wl,-framework,OpenGL \
		-XCClinker -ObjC \
		-XCClinker -bundle_loader -XCClinker XDarwinApp \
		-module -avoid-version -no-undefined
glxAGL_la_DEPENDENCIES = XDarwinApp	



endif

man1_MANS = XDarwin.man

uninstall-hook:
	rm -rf $(DESTDIR)$(macosdir)/XDarwin

install-data-hook: $(HOOK_TARGETS)

xquartz-install-hook:
	mv $(DESTDIR)$(macosdir)/XDarwinApp $(DESTDIR)$(macosdir)/XDarwin
	cd apple && xcodebuild install

EXTRA_DIST = \
	darwin.c \
	darwinClut8.h \
	darwinEvents.c \
	darwin.h \
	darwinKeyboard.c \
	darwinKeyboard.h \
	darwinXinput.c \
	XDarwin.man
