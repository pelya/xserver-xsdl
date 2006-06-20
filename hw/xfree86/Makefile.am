if DRI
DRI_SUBDIR = dri
endif

DOC_SUBDIR = doc

SUBDIRS = common ddc dummylib i2c x86emu int10 fbdevhw os-support parser rac \
          ramdac shadowfb vbe vgahw xaa xf1bpp xf4bpp xf8_16bpp \
	  xf8_32bpp loader dixmods exa \
	  $(DRI_SUBDIR) utils $(DOC_SUBDIR) getconfig

DIST_SUBDIRS = common ddc dummylib i2c x86emu int10 fbdevhw os-support \
               parser rac ramdac shadowfb vbe vgahw xaa xf1bpp xf4bpp \
               xf8_16bpp xf8_32bpp loader dixmods dri exa \
	       utils doc getconfig

bin_PROGRAMS = Xorg

AM_CFLAGS = @XORG_CFLAGS@ @SERVER_DEFINES@ @LOADER_DEFINES@
INCLUDES = @XORG_INCS@ 

Xorg_SOURCES = xorg.c

# libxorgos and libcommon need symbols from each other
noinst_LTLIBRARIES = libosandcommon.la
libosandcommon_la_SOURCES = osandcommon.c
libosandcommon_la_LIBADD = \
            os-support/libxorgos.la \
            common/libcommon.la

osandcommon.c xorg.c:
	touch $@

DISTCLEANFILES = osandcommon.c xorg.c

XORG_LIBS = \
            @XORG_CORE_LIBS@ \
            common/libinit.a \
            libosandcommon.la \
	    rac/librac.a \
            parser/libxf86config.a \
            dummylib/libdummy.a \
	    dixmods/libdixmods.la \
            loader/libloader.a \
            @XORG_LIBS@

Xorg_DEPENDENCIES = \
	$(XORG_LIBS)

Xorg_LDADD = $(XORG_LIBS) \
             @XSERVER_LIBS@ \
	     dixmods/libxorgxkb.la

Xorg_LDFLAGS = $(LD_EXPORT_SYMBOLS_FLAG)


if SOLARIS_ASM_INLINE
# Needs to be built before any files are compiled when using Sun compilers
# so in*/out* inline definitions are properly processed.

BUILT_SOURCES = os-support/solaris/solaris-$(SOLARIS_INOUT_ARCH).il

os-support/solaris/solaris-$(SOLARIS_INOUT_ARCH).il:
	cd os-support/solaris ; make solaris-$(SOLARIS_INOUT_ARCH).il
endif

# do not use $(mkdir_p) if you want automake 1.7 to work
install-data-local:
	mkdir -p $(DESTDIR)$(logdir)


install-exec-local: install-binPROGRAMS
	(cd $(DESTDIR)$(bindir) && rm -f X && ln -s Xorg X)
if INSTALL_SETUID
	chown root $(DESTDIR)$(bindir)/Xorg
	chmod u+s $(DESTDIR)$(bindir)/Xorg
endif

optionsdir = $(libdir)/X11
dist_options_DATA = Options

EXTRA_DIST = xorgconf.cpp

relink:
	rm -f Xorg && $(MAKE) Xorg
