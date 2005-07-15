SUBDIRS = input config
bin_PROGRAMS = Xdmx

if XINERAMA
PANORAMIX_SRCS = $(top_srcdir)/Xext/panoramiX.c
endif

if GLX
SUBDIRS += glxProxy
GLX_LIBS = glxProxy/libglxproxy.a
GLX_SRCS = $(PANORAMIX_SRCS) dmx_glxvisuals.c
endif

Xdmx_SOURCES = dmxinit.c \
	dmxscrinit.c \
	dmxshadow.c \
	dmxcursor.c \
	dmxinput.c \
	dmxdpms.c \
	dmxgc.c \
	dmxgcops.c \
	dmxwindow.c \
	dmxpixmap.c \
	dmxfont.c \
	dmxcmap.c \
	dmxvisual.c \
	dmxlog.c \
	dmxcb.c \
	dmxprop.c \
	dmxstat.c \
	dmxsync.c \
	dmxextension.c \
	dmxpict.c \
	dmx.c \
	$(top_srcdir)/fb/fbcmap.c \
	$(top_srcdir)/mi/miinitext.c \
	$(GLX_SRCS) 


#if COMPOSITE
#Xdmx_SOURCES += fakecw.c
#endif

Xdmx_LDFLAGS = -rdynamic
Xdmx_LDADD = $(XORG_CORE_LIBS) \
             $(XDMX_LIBS) \
             $(GLX_LIBS) \
             input/libdmxinput.a \
             config/libdmxconfig.a \
             @DMXMODULES_LIBS@

Xdmx_CFLAGS = -DHAVE_DMX_CONFIG_H @DMXMODULES_CFLAGS@ -DXFree86Server
