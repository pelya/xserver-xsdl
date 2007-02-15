DIST_SUBDIRS = input config glxProxy examples doc

SUBDIRS = input config examples
bin_PROGRAMS = Xdmx
noinst_LIBRARIES = libfbcmap.a

if XINERAMA
PANORAMIX_SRCS = $(top_srcdir)/Xext/panoramiX.c
endif

if GLX
SUBDIRS += glxProxy
GLX_LIBS = glxProxy/libglxproxy.a
GLX_SRCS = $(PANORAMIX_SRCS) dmx_glxvisuals.c dmx_glxvisuals.h
GLX_INCS = -I$(top_srcdir)/hw/xfree86/dixmods/extmod \
           -I@MESA_SOURCE@/include
GLX_DEFS = @GL_CFLAGS@ 
endif

if BUILDDOCS
SUBDIRS += doc
endif

AM_CFLAGS = \
      -DHAVE_DMX_CONFIG_H \
      $(DIX_CFLAGS) \
      $(GLX_INCS) \
      $(GLX_DEFS) \
      @DMXMODULES_CFLAGS@

libfbcmap_a_SOURCES = libfbcmap.a
libfbcmap_a_CFLAGS = $(AM_CFLAGS) -DXFree86Server

Xdmx_SOURCES = dmx.c \
               dmxcb.c \
               dmxcb.h \
               dmxclient.h \
               dmxcmap.c \
               dmxcmap.h \
               dmx-config.h \
               dmxcursor.c \
               dmxcursor.h \
               dmxdpms.c \
               dmxdpms.h \
               dmxextension.c \
               dmxextension.h \
               dmxfont.c \
               dmxfont.h \
               dmxgc.c \
               dmxgc.h \
               dmxgcops.c \
               dmxgcops.h \
               dmx.h \
               dmxinit.c \
               dmxinit.h \
               dmxinput.c \
               dmxinput.h \
               dmxlog.c \
               dmxlog.h \
               dmxpict.c \
               dmxpict.h \
               dmxpixmap.c \
               dmxpixmap.h \
               dmxprop.c \
               dmxprop.h \
               dmxscrinit.c \
               dmxscrinit.h \
               dmxshadow.c \
               dmxshadow.h \
               dmxstat.c \
               dmxstat.h \
               dmxsync.c \
               dmxsync.h \
               dmxvisual.c \
               dmxvisual.h \
               dmxwindow.c \
               dmxwindow.h \
               $(top_srcdir)/mi/miinitext.c \
               $(GLX_SRCS) 


#if COMPOSITE
#Xdmx_SOURCES += fakecw.c
#endif

Xdmx_LDFLAGS = $(LD_EXPORT_SYMBOLS_FLAG)
Xdmx_LDADD = $(XORG_CORE_LIBS) \
             $(XDMX_LIBS) \
             $(GLX_LIBS) \
             input/libdmxinput.a \
             config/libdmxconfig.a \
	     libfbcmap.a \
             @DMXMODULES_LIBS@

# Man page
appmandir = $(APP_MAN_DIR)

appman_PRE = Xdmx.man
appman_DATA = $(appman_PRE:man=@APP_MAN_SUFFIX@)

CLEANFILES = $(appman_DATA)

# Strings to replace in man pages
XORGRELSTRING = @PACKAGE_STRING@
  XORGMANNAME = X Version 11

MAN_SUBSTS = \
	-e 's|__vendorversion__|"$(XORGRELSTRING)" "$(XORGMANNAME)"|' \
	-e 's|__miscmansuffix__|$(MISC_MAN_SUFFIX)|g'

SUFFIXES = .$(APP_MAN_SUFFIX) .man

.man.$(APP_MAN_SUFFIX):
	sed $(MAN_SUBSTS) < $< > $@

EXTRA_DIST = $(appman_PRE)

relink:
	rm -f Xdmx && $(MAKE) Xdmx
