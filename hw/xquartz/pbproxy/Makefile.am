AM_CPPFLAGS=-F/System/Library/Frameworks/ApplicationServices.framework/Frameworks
AM_LDFLAGS=-L/usr/X11/lib -lX11 -lAppleWM -framework AppKit -framework Foundation -framework ApplicationServices

noinst_PROGRAMS = pbproxy

pbproxy_SOURCES = \
	trick_autotools.c \
	main.m \
	x-input.m \
	x-selection.m

EXTRA_DIST = \
	pbproxy.h \
	x-selection.h

