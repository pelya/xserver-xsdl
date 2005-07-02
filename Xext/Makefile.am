noinst_LTLIBRARIES = libXext.la

sdk_HEADERS = dgaproc.h xvdix.h xvmcext.h

SUBDIRS = extmod

if MITSHM
MITSHM_SRCS = shm.c
endif

if XV
XV_SRCS = xvmain.c xvdisp.c xvmc.c xvdix.h xvmcext.h
endif

if RES
RES_SRCS = xres.c
endif

if SCREENSAVER
SCREENSAVER_SRCS = saver.c
endif

if XCSECURITY   
XCSECURITY_SRCS = security.c
endif

if XINERAMA
PANORAMIX_SRCS = panoramiX.c panoramiX.h panoramiXh.h panoramiXsrv.h panoramiXprocs.c panoramiXSwap.c 
endif

if XEVIE
XEVIE_SRCS = xevie.c EVIstruct.h
endif

INCLUDES = -I$(srcdir)/extmod

AM_CFLAGS = $(DIX_CFLAGS)

EXTRA_DIST = \
	dgaproc.h \
	vidmodeproc.h \
	xf86dgaext.h \
	xf86miscproc.h \
	appgroup.h \
	xvdisp.h \
	xvmodproc.h

# dmx.c has a hard dependency on the dmx DDX
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
	xf86dga.c		\
	xf86dgaext.h		\
	xf86dga2.c		\
	$(MITSHM_SRCS)		\
	$(XV_SRCS)		\
	$(RES_SRCS)		\
	$(SCREENSAVER_SRCS)	\
	$(PANORAMIX_SRCS)	\
	$(XCSECURITY_SRCS)	\
	$(XEVIE_SRCS)		\
	extmod/modinit.c
