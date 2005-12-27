INCLUDES = 					\
	@KDRIVE_INCS@				\
	@KDRIVE_CFLAGS@

noinst_LIBRARIES = libxephyr.a libxephyr-hostx.a

if TSLIB
TSLIB_LIBS = -lts
endif


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
	@KDRIVE_LIBS@					\
        $(TSLIB_LIBS)                                   \
        @XEPHYR_LIBS@
	

Xephyr_DEPENDENCIES =					\
	libxephyr.a					\
	libxephyr-hostx.a                               \
	@KDRIVE_LIBS@					

