noinst_LTLIBRARIES = libpcl.la

INCLUDES = -I$(top_srcdir)/Xprint -I$(top_srcdir)/cfb -I$(top_srcdir)/mfb

AM_CFLAGS = @DIX_CFLAGS@ -D_XP_PRINT_SERVER_ -DPSZ=8

libpcl_la_SOURCES =		\
	PclArc.c		\
	PclArea.c		\
	PclAttr.c		\
	PclAttVal.c		\
	PclColor.c		\
	PclCursor.c		\
	PclDef.h		\
	PclFonts.c		\
	PclGC.c			\
	Pcl.h			\
	PclInit.c		\
	PclLine.c		\
	Pclmap.h		\
	PclMisc.c		\
	PclPixel.c		\
	PclPixmap.c		\
	PclPolygon.c		\
	PclPrint.c		\
	PclSFonts.c		\
	PclSFonts.h		\
	PclSpans.c		\
	PclText.c		\
	PclWindow.c
