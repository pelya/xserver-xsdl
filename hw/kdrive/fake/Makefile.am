INCLUDES = 					\
	@KDRIVE_INCS@				\
	@XSERVER_CFLAGS@

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
	@XSERVER_LIBS@					\
	$(TSLIB_FLAG)

Xfake_DEPENDENCIES =					\
	libfake.a					\
	@KDRIVE_LIBS@					

