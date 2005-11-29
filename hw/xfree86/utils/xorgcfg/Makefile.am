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

xorgcfg_CFLAGS = $(XORG_CFLAGS) $(CURSESDEFINES) $(XORGCFG_DEP_CFLAGS)
xorgcfg_LDADD = $(XORGCFG_DEP_LIBS) ../../parser/libxf86config.a $(LOADERLIB) \
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

if NEED_STRLCAT
STRL_SRCS = $(top_srcdir)/os/strlcat.c $(top_srcdir)/os/strlcpy.c
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
        xf86config.h \
	$(STRL_SRCS)

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

# Rules needed to cpp man page & app-defaults
include $(top_srcdir)/cpprules.in

# App default files  (*.ad)

appdefaultdir = $(libdir)/X11/app-defaults

APPDEFAULTFILES = XOrgCfg

appdefault_DATA = $(APPDEFAULTFILES)

CPP_FILES_FLAGS = -D__VENDORNAME__="@VENDOR_STRING_SHORT@" \
                  -D__VENDORVERS__="@VENDOR_MAN_VERSION@"

# dear automake: it would be really nice if you would actually put things into
# .SUFFIXES so i wouldnt' have to do this.
XOrgCfg: XOrgCfg.pre
	$(RAWCPP) $(RAWCPPFLAGS) $(CPP_FILES_FLAGS) < XOrgCfg.pre | $(CPP_SED_MAGIC) > $@

# Man page
appmandir = $(APP_MAN_DIR)

appman_SOURCES = xorgcfg.man
appman_DATA = $(appman_SOURCES:man=@APP_MAN_SUFFIX@)

all-local: $(appman_SOURCES) $(appman_DATA)

EXTRA_DIST = $(XBM_DATA) $(XPM_DATA) XOrgCfg.pre xorgcfg.man.pre
BUILT_SOURCES = $(appman_SOURCES)
CLEANFILES = $(APPDEFAULTFILES) $(BUILT_SOURCES) $(appman_DATA)

SUFFIXES += .$(APP_MAN_SUFFIX) .man

.man.$(APP_MAN_SUFFIX):
	-rm -f $@
	$(LN_S) $< $@

endif
