SUBDIRS = input
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
	miinitext.c \
	dmxpict.c \
	libc_wrapper.c \
	$(GLX_SRCS) 



#if COMPOSITE
#Xdmx_SOURCES += fakecw.c
#endif

miinitext.c: 
	echo "#include <config.h>" > $@
	echo "#undef HAVE_CONFIG_H" >> $@
	echo "#define DMXEXT" >> $@
	echo "#undef BEZIER " >> $@
	echo "#undef PEXEXT " >> $@ 
	echo "#undef MULTIBUFFER  " >> $@
	echo "#undef XV  " >> $@
	echo "#undef XIE " >> $@ 
	echo "#undef DBE " >> $@ 
	echo "#undef XF86VIDMODE  " >> $@
	echo "#undef XF86MISC  " >> $@
	echo "#undef XFreeXDGA " >> $@ 
	echo "#undef XF86DRI  " >> $@
	echo "#undef MITSHM  " >> $@
	echo "#undef TOGCUP  " >> $@
	echo "#undef DPSEXT  " >> $@
	echo "#undef MITMISC " >> $@ 
	echo "#undef SCREENSAVER  " >> $@
	echo "#undef RANDR  " >> $@
	echo "#undef XFIXES " >> $@ 
	echo "#undef DAMAGE " >> $@
	echo "#undef COMPOSITE " >> $@
	echo "#undef FONTCACHE " >> $@
	echo "#undef XFree86LOADER " >> $@
	echo "#include \"$(top_srcdir)/mi/miinitext.c\"" >> $@

DISTCLEANFILES = miinitext.c libc_wrapper.c

libc_wrapper.c:
	echo "#define SELF_CONTAINED_WRAPPER" > $@
	echo "#include \"$(top_srcdir)/hw/xorg/os-support/shared/libc_wrapper.c\"" >> $@
	

Xdmx_LDFLAGS = -rdynamic
Xdmx_LDADD = $(XORG_CORE_LIBS) $(top_srcdir)/hw/xorg/os-support/@XORG_OS_SUBDIR@/lib@XORG_OS_SUBDIR@.a  \
	$(XDMX_LIBS) $(GLX_LIBS) $(top_srcdir)/miext/shadow/.libs/libshadow.a $(top_srcdir)/fb/.libs/libfb.a  \
	$(top_srcdir)/hw/xorg/dummylib/libdummy.a  input/libdmxinput.a \
	config/libdmxconfig.a -lfl $(XDMCP_LIBS) @DMXMODULES_LIBS@  -lXi

Xdmx_CFLAGS = @DMXMODULES_CFLAGS@
