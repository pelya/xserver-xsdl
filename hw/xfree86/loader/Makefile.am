noinst_LIBRARIES = libloader.a

INCLUDES = $(XORG_INCS) -I$(srcdir)/../parser

#AM_LDFLAGS = -r
AM_CFLAGS = -DIN_LOADER $(XORG_CFLAGS) @SERVER_DEFINES@ @LOADER_DEFINES@

if XORG_LOADER_SPARC
SPARC_SOURCES = SparcMulDiv.S
endif

EXTRA_DIST = \
	dlloader.h \
	loader.h \
	loaderProcs.h \
	SparcMulDiv.S

libloader_a_SOURCES = \
	loader.c \
	loaderProcs.h \
	loadext.c \
        loadmod.c \
	loadfont.c \
        dlloader.c \
	os.c \
	$(SPARC_SOURCES)
