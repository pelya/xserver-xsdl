INCLUDES = 					\
	@KDRIVE_INCS@				\
	@KDRIVE_CFLAGS@

noinst_LIBRARIES = libfbdev.a

bin_PROGRAMS = Xfbdev

if TSLIB
TSLIB_FLAG = -lts
endif

libfbdev_a_SOURCES =	\
	fbdev.c		\
	fbdev.h

Xfbdev_SOURCES = \
	fbinit.c

Xfbdev_LDADD = 						\
	libfbdev.a					\
	@KDRIVE_LIBS@					\
	@XSERVER_LIBS@					\
	$(TSLIB_FLAG)

Xfbdev_DEPENDENCIES =					\
	libfbdev.a
