extsmoduledir = $(moduledir)/extensions
extsmodule_LTLIBRARIES = libextmod.la

AM_CFLAGS = @DIX_CFLAGS@ @XORG_CFLAGS@
INCLUDES = @XORG_INCS@ \
           -I$(top_srcdir)/dbe \
           -I$(top_srcdir)/hw/xfree86/loader \
           -I$(top_srcdir)/miext/shadow

libextmod_la_LDFLAGS = -module -avoid-version $(LD_NO_UNDEFINED_FLAG)
libextmod_la_SOURCES = modinit.c
libextmod_la_LIBADD = $(top_builddir)/Xext/libXextmodule.la
