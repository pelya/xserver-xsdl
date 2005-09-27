# Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
# 
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, and/or sell copies of the Software, and to permit persons
# to whom the Software is furnished to do so, provided that the above
# copyright notice(s) and this permission notice appear in all copies of
# the Software and that both the above copyright notice(s) and this
# permission notice appear in supporting documentation.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
# OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
# HOLDERS INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL
# INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING
# FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
# NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
# WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
# 
# Except as contained in this notice, the name of a copyright holder
# shall not be used in advertising or otherwise to promote the sale, use
# or other dealings in this Software without prior written authorization
# of the copyright holder.
# 

if BUILD_XORGCFG
bin_PROGRAMS = xorgcfg

INCLUDES = $(XORG_INCS) -I$(top_srcdir)/hw/xfree86/parser

xorgcfg_CFLAGS = $(XORG_CFLAGS) $(CURSESDEFINES)
xorgcfg_LDADD = $(XORGCFG_DEP_LIBS) ../../parser/libparser.a $(LOADERLIB) \
	../../os-support/libxorgos.la ../../dummylib/libdummy-nonserver.a

#if DoLoadableServer
LDSRCS  = \
	$(top_srcdir)/hw/xfree86/os-support/shared/libc_wrapper.c \
	loader.c loadmod.c
LOADERLIB = ../../loader/libloader.a
#endif

if USE_CURSES
TEXTSRC = text-mode.c
TEXTOBJ = text-mode.o
CURSESDEFINES  = -DHAS_NCURSES
endif

xorgcfg_SOURCES =	\
        accessx.c \
        card-cfg.c \
        cards.c \
        config.c \
        expert.c \
        help.c \
        interface.c \
        keyboard-cfg.c \
        $(LDSRCS)  \
        monitor-cfg.c \
        mouse-cfg.c \
        options.c \
        screen-cfg.c \
        screen.c \
        $(SNPRINTFSRCS) \
        startx.c \
        $(STRLSRCS) \
        stubs.c \
        $(TEXTSRC) \
        vidmode.c \
        xf86config.c \
        card-cfg.h \
        cards.h \
        config.h \
        help.h \
        keyboard-cfg.h \
        loader.h \
        monitor-cfg.h \
        mouse-cfg.h \
        options.h \
        screen-cfg.h \
        screen.h \
        stubs.h \
        vidmode.h \
        xf86config.h

XBMdir = $(includedir)/X11/bitmaps
XPMdir = $(includedir)/X11/pixmaps

XBM_DATA = \
        card.xbm \
        keyboard.xbm \
        monitor.xbm \
        mouse.xbm \
        left.xbm \
        right.xbm \
        up.xbm \
        down.xbm \
        wider.xbm \
        narrower.xbm \
        shorter.xbm \
        taller.xbm

XPM_DATA = \
        card.xpm \
        computer.xpm \
        keyboard.xpm \
        monitor.xpm \
        mouse.xpm

include $(top_srcdir)/cpprules.in

# App default files  (*.ad)

appdefaultdir = $(sysconfdir)/X11/app-defaults

APPDEFAULTFILES = XOrgCfg

appdefault_DATA = $(APPDEFAULTFILES)

# Man page

SUFFIXES += .$(APP_MAN_SUFFIX) .man

XORGRELSTRING = @PACKAGE_STRING@
  XORGMANNAME = X Version 11

MANDEFS = -D__appmansuffix__=$(APP_MAN_SUFFIX) \
        -D__vendorversion__="\"$(XORGRELSTRING)\" \"$(XORGMANNAME)\"" \
	-D__xservername__=Xorg -D__xconfigfile__=xorg.conf \
	-D__projectroot__=$(prefix)

CPP_MAN_FLAGS = $(MANDEFS) $(EXTRAMANDEFS)

.man.$(APP_MAN_SUFFIX):
	$(RAWCPP) $(RAWCPPFLAGS) $(CPP_MAN_FLAGS) < $< | $(CPP_SED_MAGIC) > $@

dist_man1_MANS =  \
	xorgcfg.man

EXTRA_DIST = $(XBM_DATA) $(XPM_DATA) XOrgCfg.pre
endif
