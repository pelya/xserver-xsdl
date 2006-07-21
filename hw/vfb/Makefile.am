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
 \
            $(XVFBMODULES_CFLAGS)

# Man page
include $(top_srcdir)/cpprules.in

appmandir = $(APP_MAN_DIR)

appman_PRE = Xvfb.man
appman_DATA = $(appman_PRE:man=@APP_MAN_SUFFIX@)

BUILT_SOURCES = $(appman_PRE)
CLEANFILES = $(appman_PRE) $(appman_DATA)

SUFFIXES += .$(APP_MAN_SUFFIX) .man

.man.$(APP_MAN_SUFFIX):
	-rm -f $@
	$(LN_S) $< $@

EXTRA_DIST = Xvfb.man.pre

relink:
	rm -f Xvfb && $(MAKE) Xvfb
