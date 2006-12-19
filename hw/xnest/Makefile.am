bin_PROGRAMS = Xnest

SRCS =	Args.c \
	Args.h \
	Color.c \
	Color.h \
	Cursor.c \
	Display.c \
	Display.h \
	Drawable.h \
	Events.c \
	Events.h \
	Font.c \
	GC.c \
	GCOps.c \
	GCOps.h \
	Handlers.c \
	Handlers.h \
	Init.c \
	Init.h \
	Keyboard.c \
	Keyboard.h \
	Pixmap.c \
	Pointer.c \
	Pointer.h \
	Screen.c \
	Screen.h \
	Visual.c \
	Visual.h \
	Window.c \
	XNCursor.h \
	Xnest.h \
	XNFont.h \
	XNGC.h \
	XNPixmap.h \
	XNWindow.h \
	xnest-config.h \
	$(top_srcdir)/Xext/dpmsstubs.c \
	$(top_srcdir)/Xi/stubs.c \
	$(top_srcdir)/mi/miinitext.c \
	$(top_srcdir)/fb/fbcmap.c


Xnest_SOURCES = $(SRCS)

Xnest_LDADD = $(XORG_CORE_LIBS) \
              $(XNEST_LIBS) \
              $(XNESTMODULES_LIBS)

Xnest_LDFLAGS =

AM_CFLAGS = -DHAVE_XNEST_CONFIG_H \
            -DNO_HW_ONLY_EXTS \
            -DXFree86Server \
            $(DIX_CFLAGS) \
            $(XNESTMODULES_CFLAGS)

EXTRA_DIST = os2Stub.c \
             icon \
             screensaver \
	     Xnest.man.pre

# -UDPMSExtension for miinitext? can't put into 
# OS_DEFINES???
# EXT_DEFINES???
# ICONFIGFILES -- SpecialCObjectRule

# Man page
include $(top_srcdir)/cpprules.in

appmandir = $(APP_MAN_DIR)

appman_PRE = Xnest.man
appman_DATA = $(appman_PRE:man=@APP_MAN_SUFFIX@)

EXTRAMANDEFS = \
	-D__XCONFIGFILE__=$(__XCONFIGFILE__) \
	-D__XSERVERNAME__=$(XSERVERNAME)

BUILT_SOURCES = $(appman_PRE)
CLEANFILES = $(appman_PRE) $(appman_DATA)

SUFFIXES += .$(APP_MAN_SUFFIX) .man

.man.$(APP_MAN_SUFFIX):
	-rm -f $@
	$(LN_S) $< $@

relink:
	rm -f Xnest && $(MAKE) Xnest
