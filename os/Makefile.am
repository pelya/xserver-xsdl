noinst_LTLIBRARIES = libos.la libcwrapper.la

AM_CFLAGS = $(DIX_CFLAGS) @SERVER_DEFINES@ @LOADER_DEFINES@

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

if NEED_STRLCAT
STRLCAT_SOURCES = strlcat.c strlcpy.c
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
	$(LBX_SOURCES) \
	$(STRLCAT_SOURCES)

libcwrapper_la_SOURCES = \
	$(top_srcdir)/hw/xfree86/os-support/shared/libc_wrapper.c
libcwrapper_la_CFLAGS = \
	-DSELF_CONTAINED_WRAPPER \
	-I$(top_srcdir)/hw/xfree86/os-support \
	$(AM_CFLAGS)

EXTRA_DIST = $(K5AUTH_SOURCES) $(SECURERPC_SOURCES) $(INTERNALMALLOC_SOURCES)
