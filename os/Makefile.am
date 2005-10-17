noinst_LTLIBRARIES = libos.la

AM_CFLAGS = $(DIX_CFLAGS)

# FIXME: Add support for these in configure.ac
K5AUTH_SOURCES = k5auth.c
SECURERPC_SOURCES = rpcauth.c
INTERNALMALLOC_SOURCES = xalloc.c

if XCSECURITY
XCSECURITY_SOURCES = secauth.c
endif

if XDMCP
XDMCP_SOURCES = xdmcp.c
endif

if LBX
LBX_SOURCES = lbxio.c
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
	$(XORG_SOURCES) \
	$(LBX_SOURCES)

EXTRA_DIST = $(K5AUTH_SOURCES) $(SECURERPC_SOURCES) $(INTERNALMALLOC_SOURCES)
