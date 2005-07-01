noinst_LTLIBRARIES = libos.la

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#	k5auth.c	\
#	lbxio.c		\
#	rpcauth.c	\
#	genalloca.c	\
#	hpsocket.c	\
#	secauth.c
#	xalloc.c

if XCSECURITY
XCSECURITY_SOURCES = secauth.c
endif

if XDMCP
XDMCP_SOURCES = xdmcp.c
endif

XORG_SOURCES = log.c

libos_la_SOURCES = 	\
	WaitFor.c	\
	access.c	\
	auth.c		\
	connection.c	\
	io.c		\
	mitauth.c	\
	oscolor.c	\
	osdep.h		\
	osinit.c	\
	utils.c		\
	xdmauth.c	\
	xstrans.c	\
	xprintf.c	\
	$(XCSECURITY_SOURCES) \
	$(XDMCP_SOURCES) \
	$(XORG_SOURCES)
