INCLUDES = 					\
	@KDRIVE_INCS@				\
	@KDRIVE_CFLAGS@

noinst_LIBRARIES = libfbdev.a

if TSLIB
TSLIB_FLAG = -lts
endif

libfbdev_a_SOURCES =	\
	fbdev.c		\
	fbdev.h

if KDRIVEFBDEV
bin_PROGRAMS = Xfbdev

Xfbdev_SOURCES = \
	fbinit.c

Xfbdev_LDADD = 						\
	libfbdev.a					\
	@KDRIVE_LIBS@					\
	@XSERVER_LIBS@					\
	$(TSLIB_FLAG)
endif
