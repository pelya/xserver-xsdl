noinst_LTLIBRARIES = libXdarwin.la
AM_CFLAGS = $(XSERVER_CFLAGS) $(DIX_CFLAGS)
AM_CPPFLAGS = \
	-DINXQUARTZ \
	-DUSE_NEW_CLUT \
	-DXFree86Server

SUBDIRS = . quartz

libXdarwin_la_SOURCES = \
	darwin.c \
	darwinEvents.c \
	darwinKeyboard.c \
	darwinXinput.c \
	$(top_srcdir)/fb/fbcmap_mi.c \
	$(top_srcdir)/mi/miinitext.c

EXTRA_DIST = \
	darwinClut8.h \
	darwin.h \
	darwinKeyboard.h
