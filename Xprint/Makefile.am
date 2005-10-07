SUBDIRS = doc pcl raster ps etc

bin_PROGRAMS = Xprt

Xprt_CFLAGS = @DIX_CFLAGS@ -DPRINT_ONLY_SERVER -D_XP_PRINT_SERVER_  -DXPRINTDIR=\"$(prefix)/X11/xserver\"  

Xprt_LDFLAGS = -L$(top_srcdir)
Xprt_LDADD = @XPRINT_LIBS@ pcl/libpcl.la raster/libraster.la

miinitext-wrapper.c:
	echo "#include \"$(top_srcdir)/mi/miinitext.c\"" >> $@

dpmsstubs-wrapper.c:
	echo "#include \"$(top_srcdir)/Xext/dpmsstubs.c\"" >> $@

Xprt_SOURCES =			\
	attributes.c		\
	attributes.h		\
	AttrValid.c		\
	AttrValid.h		\
	ddxInit.c		\
	DiPrint.h		\
	Init.c			\
	mediaSizes.c		\
	Oid.c			\
	OidDefs.h		\
	Oid.h			\
	OidStrs.h		\
	spooler.c		\
	spooler.h		\
	Util.c			\
	miinitext-wrapper.c	\
	dpmsstubs-wrapper.c
