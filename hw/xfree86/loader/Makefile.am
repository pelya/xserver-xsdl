noinst_LIBRARIES = libloader.a
INCLUDES = $(XORG_INCS) -I$(srcdir)/../parser -I$(srcdir)/../vbe \
           -I$(srcdir)/../ddc -I$(srcdir)/../int10 -I$(srcdir)/../i2c \
           -I$(srcdir)/../fbdevhw -I$(srcdir)/../scanpci -I$(srcdir)/../xaa \
           -I$(srcdir)/../vgahw -I$(srcdir)/../ramdac -I$(srcdir)/../shadowfb \
           -I$(srcdir)/../dixmods/extmod -I$(srcdir)/../../../miext/cw \
           -I$(srcdir)/../../../miext/damage

#AM_LDFLAGS = -r
AM_CFLAGS = -DIN_LOADER $(XORG_CFLAGS)

if XORG_LOADER_SPARC
SPARC_SOURCES = SparcMulDiv.S
endif

sdk_HEADERS = \
	elf.h

EXTRA_DIST = \
	aout.h \
	aoutloader.h \
	ar.h \
	coff.h \
	coffloader.h \
	dlloader.h \
	elf.h \
	elfloader.h \
	hash.h \
	loader.h \
	loaderProcs.h \
	sym.h \
	SparcMulDiv.S

libloader_a_SOURCES = \
	loader.c \
	loaderProcs.h \
	loadext.c \
        loadmod.c \
	loadfont.c \
        aoutloader.c \
        dlloader.c \
        elfloader.c \
        coffloader.c \
	os.c \
	dixsym.c \
	extsym.c \
	fontsym.c \
	misym.c \
	xf86sym.c \
	hash.c \
	$(SPARC_SOURCES)
