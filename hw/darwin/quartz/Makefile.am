noinst_LIBRARIES = libXQuartz.a

AM_CFLAGS = @XORG_CFLAGS@ @DIX_CFLAGS@
AM_OBJCFLAGS = @XORG_CFLAGS@ @DIX_CFLAGS@

INCLUDES = -I$(srcdir) -I$(srcdir)/.. @XORG_INCS@
AM_DEFS = -DHAS_CG_MACH_PORT -DHAS_KL_API
if HAVE_X_PLUGIN
AM_DEFS += -DBUILD_XPR
endif
DEFS = @DEFS@ $(AM_DEFS) -DXBINDIR=\"${bindir}\"

libXQuartz_a_SOURCES = \
		  Preferences.m \
	          XApplication.m \
	          XServer.m \
	          applewm.c \
	          keysym2ucs.c \
	          quartz.c \
	          quartzAudio.c \
	          quartzCocoa.m \
	          quartzPasteboard.c \
	          quartzKeyboard.c \
	          quartzStartup.c \
	          pseudoramiX.c

bin_PROGRAMS = XDarwinStartup

XDarwinStartup_SOURCES = XDarwinStartup.c
XDarwinStartup_LDFLAGS = -Wl,-framework,CoreFoundation \
			 -Wl,-framework,ApplicationServices
XDarwinStartupCFLAGS = -DXBINDIR="${bindir}"
XDARWINROOT = @APPLE_APPLICATIONS_DIR@
BINDIR = $(bindir)
install-exec-local:
	-(cd $(DESTDIR)$(BINDIR); rm X; $(LN_S) XDarwinStartup X)

man1_MANS = XDarwinStartup.man

EXTRA_DIST = \
	applewmExt.h \
	keysym2ucs.h \
	Preferences.h \
	pseudoramiX.h \
	quartzAudio.h \
	quartzCommon.h \
	quartzCursor.c \
	quartzCursor.h \
	quartz.h \
	quartzPasteboard.h \
	XApplication.h \
	XDarwin.pbproj/project.pbxproj \
	XServer.h \
	XDarwinStartup.man
