INCLUDES = 					\
	@KDRIVE_INCS@				\
	@KDRIVE_CFLAGS@				\
	-I$(srcdir)/../../../exa

noinst_LIBRARIES = libxephyr-hostx.a libxephyr-hostxv.a libxephyr.a

bin_PROGRAMS = Xephyr


libxephyr_hostx_a_SOURCES =                             \
	hostx.c                                         \
	hostx.h

libxephyr_hostx_a_INCLUDES = @XEPHYR_INCS@

libxephyr_hostxv_a_SOURCES=				\
	ephyrhostvideo.c \
	ephyrhostvideo.h

libxephyr_a_SOURCES =	\
	ephyr.c		\
	ephyr_draw.c	\
	ephyrvideo.c	\
	XF86dri.c	\
	ephyrdriext.c	\
	ephyrdri.c	\
	ephyrdri.h	\
	ephyrglxext.c	\
	ephyrglxext.h	\
	ephyrhostglx.c	\
	ephyrhostglx.h	\
	os.c		\
	hostx.h         \
	ephyr.h		\
	ephyrlog.h

libxephyr_a_CFLAGS = \
@LIBDRM_CFLAGS@ \
-I$(top_srcdir) \
@DRIPROTO_CFLAGS@

Xephyr_SOURCES = \
	ephyrinit.c

Xephyr_LDADD = 						\
	libxephyr.a					\
	libxephyr-hostx.a			        \
	libxephyr-hostxv.a			        \
	../../../exa/libexa.la				\
	@KDRIVE_LIBS@					\
        @XEPHYR_LIBS@ 					\
	@LIBDRM_LIBS@					\
	-lGL

Xephyr_DEPENDENCIES =	\
	libxephyr.a					\
	libxephyr-hostx.a			        \
	libxephyr-hostxv.a			        \
	@KDRIVE_LOCAL_LIBS@

relink:
	rm -f $(bin_PROGRAMS) && make $(bin_PROGRAMS)
