if DRI
DRI_SUBDIR = dri
endif

if BUILDDOCS
DOC_SUBDIR = doc
endif


SUBDIRS = common ddc dummylib i2c x86emu int10 fbdevhw os-support parser rac \
          ramdac shadowfb vbe vgahw xaa xf1bpp xf4bpp xf8_16bpp \
	  xf8_32bpp xf8_32wid loader scanpci dixmods exa $(DRI_SUBDIR) utils \
	  $(DOC_SUBDIR) getconfig

DIST_SUBDIRS = common ddc dummylib i2c x86emu int10 fbdevhw os-support \
               parser rac ramdac shadowfb vbe vgahw xaa xf1bpp xf4bpp \
               xf8_16bpp xf8_32bpp xf8_32wid loader scanpci dixmods dri exa \
	       utils doc getconfig

bin_PROGRAMS = Xorg

AM_CFLAGS = @XORG_CFLAGS@
INCLUDES = @XORG_INCS@ 

Xorg_SOURCES = xorg.c

DISTCLEANFILES = xorg.c

xorg.c:
	touch $@

OS_LIBS = os-support/libxorgos.la

XORG_LIBS = \
            @XORG_CORE_LIBS@ \
            common/libinit.a \
            loader/libloader.a \
            common/libcommon.a \
            parser/libparser.a \
            $(OS_LIBS) \
            dummylib/libdummy.a \
	    dixmods/libdixmods.la \
            @XORG_LIBS@

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

install-data-local:
	$(mkdir_p) $(DESTDIR)$(logdir)


install-exec-local:
if INSTALL_SETUID
	chown root $(DESTDIR)$(bindir)/Xorg
	chmod u+s $(DESTDIR)$(bindir)/Xorg
endif

