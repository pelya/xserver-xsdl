noinst_LTLIBRARIES = libos.la libcwrapper.la

AM_CFLAGS = $(DIX_CFLAGS)

# FIXME: Add support for these in configure.ac
K5AUTH_SOURCES = k5auth.c
SECURERPC_SOURCES = rpcauth.c
INTERNALMALLOC_SOURCES = xalloc.c

XCSECURITY_SOURCES = secauth.c
XDMCP_SOURCES = xdmcp.c
STRLCAT_SOURCES = strlcat.c strlcpy.c
XORG_SOURCES = log.c

libos_la_SOURCES = 	\
	WaitFor.c	\
	access.c	\
	auth.c		\
	connection.c	\
	io.c		\
	mitauth.c	\
	oscolor.c	\
	oscolor.h	\
	osdep.h		\
	osinit.c	\
	utils.c		\
	xdmauth.c	\
	xstrans.c	\
	xprintf.c	\
	$(XORG_SOURCES)

if XCSECURITY
libos_la_SOURCES += $(XCSECURITY_SOURCES)
endif

if XDMCP
libos_la_SOURCES += $(XDMCP_SOURCES)
endif

if NEED_STRLCAT
libos_la_SOURCES += $(STRLCAT_SOURCES)
endif

libcwrapper_la_SOURCES = \
	$(top_srcdir)/hw/xfree86/os-support/shared/libc_wrapper.c
libcwrapper_la_CFLAGS = \
	-DSELF_CONTAINED_WRAPPER \
	-I$(top_srcdir)/hw/xfree86/os-support \
	$(AM_CFLAGS)

EXTRA_DIST = $(K5AUTH_SOURCES) $(SECURERPC_SOURCES) $(INTERNALMALLOC_SOURCES) \
     $(XCSECURITY_SOURCES) $(XDMCP_SOURCES) $(LBX_SOURCES) $(STRLCAT_SOURCES)

if XSERVER_DTRACE
# Generate dtrace object code for probes in libos & libdix
dtrace.o: $(top_srcdir)/dix/Xserver.d $(am_libos_la_OBJECTS)
	$(DTRACE) -G -C -o $@ -s $(top_srcdir)/dix/Xserver.d .libs/*.o ../dix/.libs/*.o

noinst_LIBRARIES = os.O

os.O: dtrace.o $(am_libos_la_OBJECTS)
	ld -r -o $@ dtrace.o .libs/*.o

endif
