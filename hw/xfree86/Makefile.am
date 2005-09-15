if DRI
DRI_SUBDIR = dri
endif

SUBDIRS = common ddc dummylib i2c x86emu int10 fbdevhw os-support parser rac \
          ramdac shadowfb vbe vgahw xaa xf1bpp xf4bpp xf8_16bpp \
	  xf8_32bpp xf8_32wid loader scanpci dixmods exa $(DRI_SUBDIR)

DIST_SUBDIRS = common ddc dummylib i2c x86emu int10 fbdevhw os-support \
               parser rac ramdac shadowfb vbe vgahw xaa xf1bpp xf4bpp \
               xf8_16bpp xf8_32bpp xf8_32wid loader scanpci dixmods dri exa

bin_PROGRAMS = Xorg

AM_CFLAGS = @XORG_CFLAGS@
INCLUDES = @XORG_INCS@ 

Xorg_SOURCES = xorg.c

DISTCLEANFILES = xorg.c

xorg.c:
	touch $@

xorgos.c:
	touch $@

# to get the grouping semantics right, you have to glom these three together
# as one library, otherwise libtool will actively defeat your attempts to
# list them multiple times on the link line.
noinst_LTLIBRARIES = libxorgos.la
libxorgos_la_SOURCES = xorgos.c
libxorgos_la_LIBADD = os-support/@XORG_OS_SUBDIR@/lib@XORG_OS_SUBDIR@.la \
                      os-support/bus/libbus.la \
                      os-support/misc/libmisc.la

OS_LIBS = libxorgos.la

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
	$(mkdir_p) $(logdir)

install-exec-local:
	chown root $(DESTDIR)$(bindir)/Xorg
	chmod u+s $(DESTDIR)$(bindir)/Xorg
