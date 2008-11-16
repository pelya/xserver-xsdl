AM_CPPFLAGS=-F/System/Library/Frameworks/ApplicationServices.framework/Frameworks
AM_LDFLAGS=-L/usr/X11/lib -lX11 -lXfixes -lAppleWM -framework AppKit -framework Foundation -framework ApplicationServices

noinst_LTLIBRARIES = libxpbproxy.la
libxpbproxy_la_SOURCES = \
	trick_autotools.c \
	main.m \
	x-input.m \
	x-selection.m

if STANDALONE_XPBPROXY

bin_PROGRAMS = xpbproxy
xpbproxy_SOURCES = app-main.m
xpbproxy_LDADD = libxpbproxy.la

endif

EXTRA_DIST = \
	pbproxy.h \
	x-selection.h
