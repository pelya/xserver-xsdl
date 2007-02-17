noinst_LIBRARIES = libcr.a
AM_CFLAGS =  @XORG_CFLAGS@ @DIX_CFLAGS@
AM_OBJCFLAGS =  @XORG_CFLAGS@ @DIX_CFLAGS@
DEFS = @DEFS@ -DDEFER_NSWINDOW
INCLUDES = @XORG_INCS@ \
	   -I../fullscreen \
	   -I$(srcdir) -I$(srcdir)/.. -I$(srcdir)/../.. \
	   -I$(top_srcdir)/miext/rootless \
	   -I$(top_srcdir)/miext/rootless/safeAlpha \
	   -I$(top_srcdir)/mi

libcr_a_SOURCES = crAppleWM.m \
	           crFrame.m \
	           crScreen.m \
	           ../fullscreen/quartzCursor.c \
	           XView.m

EXTRA_DIST = \
	cr.h \
	XView.h 
