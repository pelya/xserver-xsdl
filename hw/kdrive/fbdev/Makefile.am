INCLUDES = 					\
	@KDRIVE_INCS@				\
	@XSERVER_CFLAGS@

noinst_LIBRARIES = libfbdev.a

bin_PROGRAMS = Xfbdev

libfbdev_a_SOURCES = \
	fbdev.c

Xfbdev_SOURCES = \
	fbinit.c


if TSLIB
TSLIB_FLAG = -lts
endif

Xfbdev_LDADD = 						\
	libfbdev.a					\
	@KDRIVE_LIBS@					\
	@XSERVER_LIBS@					\
	$(TSLIB_FLAG)

Xfbdev_DEPENDENCIES =					\
	libfbdev.a					\
	@KDRIVE_LIBS@

