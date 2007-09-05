noinst_LTLIBRARIES = libdix.la libxpstubs.la

AM_CFLAGS = $(DIX_CFLAGS) \
	-DVENDOR_NAME=\""@VENDOR_NAME@"\" \
	-DVENDOR_RELEASE="@VENDOR_RELEASE@"

libdix_la_SOURCES = 	\
	atom.c		\
	colormap.c	\
	cursor.c	\
	devices.c	\
	dispatch.c	\
	dispatch.h	\
	dixfonts.c	\
	dixutils.c	\
	events.c	\
	extension.c	\
	ffs.c		\
	gc.c		\
	getevents.c	\
	globals.c	\
	glyphcurs.c	\
	grabs.c		\
	initatoms.c	\
	main.c		\
	pixmap.c	\
	privates.c	\
	property.c	\
	resource.c	\
	swaprep.c	\
	swapreq.c	\
	tables.c	\
	window.c	\
	strcasecmp.c

libxpstubs_la_SOURCES =	\
	xpstubs.c

INCLUDES = -I$(top_srcdir)/Xprint

EXTRA_DIST = buildatoms BuiltInAtoms CHANGES Xserver.d Xserver-dtrace.h.in

if XSERVER_DTRACE
# Generate dtrace header file for C sources to include
BUILT_SOURCES = Xserver-dtrace.h

Xserver-dtrace.h: $(srcdir)/Xserver.d
	$(DTRACE) -C -h -o $@ -s $(srcdir)/Xserver.d \
		|| cp Xserver-dtrace.h.in $@

# Generate dtrace object code for probes in libdix
dtrace-dix.o: $(top_srcdir)/dix/Xserver.d $(am_libdix_la_OBJECTS)
	$(DTRACE) -G -C -o $@ -s $(top_srcdir)/dix/Xserver.d .libs/*.o

noinst_PROGRAMS = dix.O

dix.O: dtrace-dix.o $(am_libdix_la_OBJECTS)
	ld -r -o $@ .libs/*.o
endif

dix.c:
	touch $@

CLEANFILES = dix.c
