bin_PROGRAMS = Xvfb

SRCS =	InitInput.c \
	InitOutput.c \
	lk201kbd.h \
	$(top_srcdir)/Xext/dpmsstubs.c \
	$(top_srcdir)/Xi/stubs.c \
	$(top_srcdir)/mi/miinitext.c \
	$(top_srcdir)/fb/fbcmap.c

Xvfb_SOURCES = $(SRCS)

Xvfb_LDADD = $(XORG_CORE_LIBS) \
             $(XVFB_LIBS) \
             $(XSERVER_LIBS) \
             $(EXTENSION_LIBS) \
             $(XVFBMODULES_LIBS)

Xvfb_LDFLAGS =

AM_CFLAGS = -DHAVE_DIX_CONFIG_H \
            -DNO_HW_ONLY_EXTS \
            -DNO_MODULE_EXTS \
            -DXFree86Server \
            $(XVFBMODULES_CFLAGS)

man1_MANS = Xvfb.man

CLEANFILES = $(man1_MANS)
