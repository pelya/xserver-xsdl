noinst_LIBRARIES = libcommon.a libinit.a

if XORG_BUS_SBUS
SBUS_SOURCES = xf86sbusBus.c
endif

#if DEBUG
#DEBUGSOURCES = xf86Debug.c
#endif

#if BUILDXV
XVSOURCES = xf86xv.c xf86xvmc.c
XVSDKINCS = xf86xv.h xf86xvmc.h
#endif

#if XKB
XKBSOURCES = xf86XKB.c
#endif

#if XINPUT
XISOURCES = xf86Xinput.c xisb.c
XISDKINCS = xf86Xinput.h xisb.h
#endif

RANDRSOURCES = xf86RandR.c

BUSSOURCES = xf86isaBus.c xf86pciBus.c xf86fbBus.c xf86noBus.c $(SBUS_SOURCES)

#if LINUX
KBDSOURCES = xf86KbdLnx.c
# XXX psuedocodeish
#else if BSD
#KBDSOURCES = xf86KbdBSD.c
#else if Mach && !GNU/Mach
#KBDSOURCES = xf86KbdMach.c
#else
#KBDSOURCES = xf86Kbd.c
#endif

# shouldn't isaBus and pciBus only be built on the relevant architectures? -ds
# ditto DoScanPci
AM_LDFLAGS = -r
libcommon_a_SOURCES = xf86Configure.c xf86Bus.c xf86Config.c \
                      xf86Cursor.c xf86DGA.c xf86DPMS.c xf86DefModes.c \
                      xf86DoProbe.c xf86DoScanPci.c xf86Events.c \
                      xf86Globals.c xf86Io.c xf86AutoConfig.c \
                      xf86MiscExt.c xf86Option.c xf86PM.c \
                      xf86VidMode.c xf86fbman.c xf86cmap.c \
                      ../scanpci/xf86ScanPci.c xf86Helper.c \
                      xf86Mode.c xf86Build.h xorgHelper.c xf86Versions.c \
                      $(XVSOURCES) $(BUSSOURCES) $(XKBSOURCES) \
                      $(DEBUGSOURCES) $(XISOURCES) $(RANDRSOURCES) \
                      $(KBDSOURCES)
libinit_a_SOURCES = xf86Build.h xf86Init.c

INCLUDES = $(XORG_INCS) -I$(srcdir)/../ddc -I$(srcdir)/../i2c \
	   -I$(srcdir)/../loader -I$(srcdir)/../rac -I$(srcdir)/../parser \
           -I$(srcdir)/../scanpci -I$(srcdir)/../vbe -I$(srcdir)/../int10 \
	   -I$(srcdir)/../vgahw

sdk_HEADERS = compiler.h fourcc.h xf86.h xf86Module.h xf86Opt.h \
               xf86PciInfo.h xf86Priv.h xf86Privstr.h xf86Resources.h \
               xf86cmap.h xf86fbman.h xf86str.h $(XISDKINCS) \
               $(XVSDKINCS) atKeynames.h

DISTCLEANFILES = xf86Build.h

EXTRA_DIST = \
	atKeynames.h \
	compiler.h \
	fourcc.h \
	scoasm.h \
	xf86.h \
	xf86Build.h \
	xf86Bus.h \
	xf86Config.h \
	xf86InPriv.h \
	xf86Keymap.h \
	xf86Module.h \
	xf86Opt.h \
	xf86PciInfo.h \
	xf86Priv.h \
	xf86Privstr.h \
	xf86Resources.h \
	xf86Xinput.h \
	xf86cmap.h \
	xf86fbman.h \
	xf86pciBus.h \
	xf86sbusBus.h \
	xf86str.h \
	xf86xv.h \
	xf86xvmc.h \
	xf86xvpriv.h \
	xisb.h \
	xf86Build.h.in

AM_CFLAGS = $(XORG_CFLAGS)
