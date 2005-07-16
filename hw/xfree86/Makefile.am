if DRI
DRI_SUBDIR = dri
endif

SUBDIRS = common ddc dummylib i2c x86emu int10 fbdevhw os-support parser rac \
          ramdac shadowfb vbe vgahw xaa xf1bpp xf4bpp xf8_16bpp \
	  xf8_32wid loader scanpci dixmods $(DRI_SUBDIR)

bin_PROGRAMS = Xorg

Xorg_SOURCES = dummy.c

DISTCLEANFILES = dummy.c

dummy.c:
	touch $@

OS_LIBS = os-support/bus/libbus.a \
          os-support/@XORG_OS_SUBDIR@/lib@XORG_OS_SUBDIR@.a os-support/misc/libmisc.a

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
             -ldl

Xorg_LDFLAGS = $(LD_EXPORT_SYMBOLS_FLAG)
