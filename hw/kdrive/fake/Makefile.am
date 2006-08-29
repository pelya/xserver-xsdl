INCLUDES = 					\
	@KDRIVE_INCS@				\
	@KDRIVE_CFLAGS@

noinst_LIBRARIES = libfake.a

bin_PROGRAMS = Xfake

if TSLIB
TSLIB_FLAG = -lts
endif

libfake_a_SOURCES =	\
	fake.c		\
	kbd.c		\
	os.c		\
	mouse.c		\
	fake.h

Xfake_SOURCES = \
	fakeinit.c

Xfake_LDADD = 						\
	libfake.a					\
	@KDRIVE_LIBS@					\
	@KDRIVE_LIBS@					\
	$(TSLIB_FLAG)                                   \
        @XSERVER_LIBS@

Xfake_DEPENDENCIES =					\
	libfake.a
