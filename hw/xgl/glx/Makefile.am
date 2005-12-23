if XGLSERVER
MODULE_SUBDIRS = module
endif

SUBDIRS =		  \
	.		  \
	$(MODULE_SUBDIRS)

INCLUDES =		    \
	@XGLX_INCS@	    \
	@XSERVER_CFLAGS@    \
	@XGLSERVER_CFLAGS@  \
	@XGLXSERVER_CFLAGS@

noinst_LTLIBRARIES = libxglx.la

libxglx_la_SOURCES = \
	xglx.h	     \
	xglx.c

if XGLXSERVER
Xglx_DEPENDENCIES = @XGLX_LIBS@
Xglx_LDFLAGS	  = -export-dynamic
Xglx_SOURCES	  = xglxinit.c
Xglx_LDADD	  =	  \
	@XGLX_LIBS@	  \
	@XSERVER_LIBS@    \
	@XGLSERVER_LIBS@  \
	@XGLXSERVER_LIBS@
Xglx_programs	  = Xglx
endif

bin_PROGRAMS = $(Xglx_programs)
