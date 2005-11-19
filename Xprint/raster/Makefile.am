noinst_LTLIBRARIES = libraster.la

INCLUDES = -I$(top_srcdir)/Xprint -I$(top_srcdir)/cfb -I$(top_srcdir)/mfb

AM_CFLAGS = @DIX_CFLAGS@ @XPRINT_CFLAGS@ -D_XP_PRINT_SERVER_ -DPSZ=8

libraster_la_SOURCES =		\
	RasterAttVal.c		\
	Raster.c		\
	Raster.h
