INCLUDES = 			\
	@KDRIVE_INCS@		\
	@KDRIVE_CFLAGS@		\
	@XEPHYR_INCS@		\
	-I$(top_srcdir)		\
	-I$(top_srcdir)/exa

if XEPHYR_HAS_DRI
INCLUDES +=			\
	@LIBDRM_CFLAGS@		\
	@DRIPROTO_CFLAGS@
endif

if XV
LIBXEPHYR_HOSTXV=libxephyr-hostxv.a
endif

if XEPHYR_HAS_DRI
LIBXEPHYR_HOSTDRI=libxephyr-hostdri.a
endif

noinst_LIBRARIES = libxephyr-hostx.a $(LIBXEPHYR_HOSTXV) $(LIBXEPHYR_HOSTDRI) libxephyr.a

bin_PROGRAMS = Xephyr

libxephyr_hostx_a_SOURCES =	\
	hostx.c			\
	hostx.h

if XV
libxephyr_hostxv_a_SOURCES=	\
	ephyrvideo.c		\
	ephyrhostvideo.c	\
	ephyrhostvideo.h
endif

if XEPHYR_HAS_DRI
libxephyr_hostdri_a_SOURCES=	\
	ephyrdriext.c		\
	ephyrdriext.h		\
	ephyrdri.c		\
	ephyrdri.h		\
	XF86dri.c		\
	ephyrglxext.c		\
	ephyrglxext.h		\
	ephyrhostglx.c		\
	ephyrhostglx.h
endif

libxephyr_a_SOURCES =		\
	ephyr.c			\
	ephyr.h			\
	ephyrlog.h		\
	ephyr_draw.c		\
	os.c

Xephyr_SOURCES = \
	ephyrinit.c

if GLX
Xephyr_LDFLAGS = $(LD_EXPORT_SYMBOLS_FLAG)
endif

Xephyr_LDADD = 						\
	libxephyr.a					\
	libxephyr-hostx.a				\
	$(LIBXEPHYR_HOSTXV)				\
	$(LIBXEPHYR_HOSTDRI)				\
	$(top_builddir)/exa/libexa.la			\
	@KDRIVE_LIBS@					\
	@XEPHYR_LIBS@ 					\
	@LIBDRM_LIBS@					\
	@XEPHYR_DRI_LIBS@

Xephyr_DEPENDENCIES =	\
	libxephyr.a					\
	libxephyr-hostx.a				\
	$(LIBXEPHYR_HOSTXV)				\
	$(LIBXEPHYR_HOSTDRI)				\
	@KDRIVE_LOCAL_LIBS@

relink:
	rm -f $(bin_PROGRAMS) && make $(bin_PROGRAMS)
