module_LTLIBRARIES = libexa.la

libexa_la_LDFLAGS = -avoid-version

INCLUDES = \
	$(XORG_INCS) \
	-I$(srcdir)/../../../exa \
	-I$(srcdir)/../../../miext/cw

AM_CFLAGS = $(XORG_CFLAGS) @SERVER_DEFINES@ @MODULE_DEFINES@ @LOADER_DEFINES@

libexa_la_SOURCES = \
	examodule.c

libexa_la_LIBADD = \
	../../../exa/libexa.la
