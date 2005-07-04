noinst_LTLIBRARIES = libxkb.la

AM_CFLAGS = $(DIX_CFLAGS)

DDX_SRCS = \
        ddxBeep.c \
        ddxCtrls.c \
        ddxFakeBtn.c \
        ddxFakeMtn.c \
        ddxInit.c \
        ddxKeyClick.c \
        ddxKillSrv.c \
        ddxLEDs.c \
        ddxVT.c \
        ddxLoad.c \
        ddxList.c \
        ddxConfig.c \
        ddxDevBtn.c \
        ddxPrivate.c

DIX_SRCS = \
        xkb.c \
        xkbUtils.c \
        xkbEvents.c \
        xkbAccessX.c \
        xkbSwap.c \
        xkbLEDs.c \
        xkbInit.c \
        xkbActions.c \
        xkbPrKeyEv.c

# this should be replaced by a common library or something, ideally -d
XKBFILE_SRCS = \
        maprules.c \
        xkmread.c \
        xkbtext.c \
        xkbfmisc.c \
        xkberrs.c \
        xkbconfig.c \
        xkbout.c

X11_SRCS = \
        XKBMisc.c \
        XKBAlloc.c \
        XKBGAlloc.c \
        XKBMAlloc.c

XI_SRCS = xkbPrOtherEv.c

libxkb_la_SOURCES = $(DDX_SRCS) $(DIX_SRCS) $(XI_SRCS) $(XKBFILE_SRCS) \
                    $(X11_SRCS)

EXTRA_DIST = xkb.h xkbDflts.h

INCLUDES = -I$(top_srcdir)/Xext/extmod
