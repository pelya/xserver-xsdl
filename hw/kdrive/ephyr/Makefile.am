INCLUDES = 					\
	@KDRIVE_INCS@				\
	@XSERVER_CFLAGS@

noinst_LIBRARIES = libxephyr.a libxephyr-hostx.a

bin_PROGRAMS = Xephyr

libxephyr_a_SOURCES =	\
	ephyr.c		\
	os.c		\
	hostx.h          \
	ephyr.h

libxephyr_hostx_a_SOURCES =                             \
	hostx.c                                         \
	hostx.h    

libxephyr_hostx_a_INCLUDES = @XEPHYR_INCS@ 

Xephyr_SOURCES = \
	ephyrinit.c

Xephyr_LDADD = 						\
	libxephyr.a					\
	libxephyr-hostx.a			        \
	@KDRIVE_LIBS@					\
	@XSERVER_LIBS@					\
        @XEPHYR_LIBS@

Xephyr_DEPENDENCIES =					\
	libxephyr.a					\
	libxephyr-hostx.a                               \
	@KDRIVE_LIBS@					

