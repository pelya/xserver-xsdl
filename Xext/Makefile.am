# libXext.la:        includes all extensions and should be linked into Xvfb,
#                    Xnest, Xdmx and Xprt
# libXextbuiltin.la: includes those extensions that are built directly into
#                    Xorg by default
# libXextmodule.la:  includes those extensions that are built into a module
#                    that Xorg loads
noinst_LTLIBRARIES = libXext.la libXextbuiltin.la libXextmodule.la

INCLUDES = -I$(top_srcdir)/hw/xfree86/dixmods/extmod -I$(top_srcdir)/hw/xfree86/os-support

AM_CFLAGS = $(DIX_CFLAGS) @LOADER_DEFINES@

sdk_HEADERS = xvdix.h xvmcext.h

if MITSHM
MITSHM_SRCS = shm.c
endif

if XV
XV_SRCS = xvmain.c xvdisp.c xvmc.c xvdix.h xvmcext.h xvdisp.h
endif

if RES
RES_SRCS = xres.c
endif

if SCREENSAVER
SCREENSAVER_SRCS = saver.c
endif

if XCSECURITY   
XCSECURITY_SRCS = security.c
SERVERCONFIGdir = $(libdir)/xserver
SERVERCONFIG_DATA = SecurityPolicy
AM_CFLAGS += -DDEFAULTPOLICYFILE=\"$(SERVERCONFIGdir)/SecurityPolicy\"
endif

if XINERAMA
PANORAMIX_SRCS = panoramiX.c panoramiX.h panoramiXh.h panoramiXsrv.h panoramiXprocs.c panoramiXSwap.c 
endif

if XEVIE
XEVIE_SRCS = xevie.c
endif

if XPRINT
XPRINT_SRCS = xprint.c
endif

if APPGROUP
APPGROUP_SRCS = appgroup.c appgroup.h
endif

if CUP
CUP_SRCS = cup.c
endif

if EVI
EVI_SRCS = EVI.c sampleEVI.c EVIstruct.h
endif

if MULTIBUFFER
MULTIBUFFER_SRCS = mbuf.c
endif
EXTRA_MULTIBUFFER_SRCS = mbufbf.c mbufpx.c

if FONTCACHE
FONTCACHE_SRCS = fontcache.c
endif

EXTRA_DIST = \
	README.xtest1-ddx \
	SecurityPolicy \
	$(EXTRA_MULTIBUFFER_SRCS)

libXext_la_SOURCES =		\
	bigreq.c		\
	mitmisc.c		\
	shape.c			\
	sleepuntil.c		\
	sleepuntil.h		\
	sync.c			\
	xtest.c			\
	xtest1di.c		\
	xtest1dd.c		\
	xtest1dd.h		\
	xcmisc.c		\
	dpms.c			\
	dpmsproc.h		\
	xf86bigfont.c		\
	$(MITSHM_SRCS)		\
	$(XV_SRCS)		\
	$(RES_SRCS)		\
	$(SCREENSAVER_SRCS)	\
	$(PANORAMIX_SRCS)	\
	$(XCSECURITY_SRCS)	\
	$(XEVIE_SRCS)		\
	$(XPRINT_SRCS)		\
	$(APPGROUP_SRCS)	\
	$(CUP_SRCS)		\
	$(EVI_SRCS)		\
	$(MULTIBUFFER_SRCS)	\
	$(FONTCACHE_SRCS)

libXextbuiltin_la_SOURCES =	\
	shape.c			\
	sleepuntil.c		\
	sleepuntil.h		\
	xtest.c			\
	xtest1di.c		\
	xtest1dd.c		\
	xtest1dd.h		\
	xf86bigfont.c		\
	$(MITSHM_SRCS)		\
	$(PANORAMIX_SRCS)	\
	$(XCSECURITY_SRCS)	\
	$(XEVIE_SRCS)		\
	$(XPRINT_SRCS)		\
	$(APPGROUP_SRCS)

libXextmodule_la_CFLAGS = $(AM_CFLAGS) @MODULE_DEFINES@ -DEXTMODULE
libXextmodule_la_SOURCES =	\
	bigreq.c		\
	mitmisc.c		\
	shape.c			\
	sync.c			\
	xcmisc.c		\
	dpms.c			\
	dpmsproc.h		\
	$(XV_SRCS)		\
	$(RES_SRCS)		\
	$(SCREENSAVER_SRCS)	\
	$(CUP_SRCS)		\
	$(EVI_SRCS)		\
	$(MULTIBUFFER_SRCS)	\
	$(FONTCACHE_SRCS)
