noinst_LTLIBRARIES = libexa.la

sdk_HEADERS = exa.h

INCLUDES = \
	$(XORG_INCS) \
	-I$(srcdir)/../miext/cw

AM_CFLAGS = $(XORG_CFLAGS) @SERVER_DEFINES@ @LOADER_DEFINES@ $(DIX_CFLAGS)

libexa_la_SOURCES = \
	exa.c \
	exa.h \
	exa_accel.c \
	exa_migration.c \
	exa_offscreen.c \
	exa_render.c \
	exa_priv.h \
	exa_unaccel.c

