INCLUDES = 					\
	@KDRIVE_INCS@				\
	@KDRIVE_CFLAGS@

bin_PROGRAMS = Xi810

noinst_LIBRARIES = libi810.a

if TSLIB
TSLIB_FLAG = -lts
endif

libi810_a_SOURCES = 		\
	i810_cursor.c	\
	i810_video.c	\
	i810draw.c	\
	i810draw.h	\
	i810_reg.h	\
	i810.c		\
	i810.h

Xi810_SOURCES = \
	i810stub.c

I810_LIBS =						\
	libi810.a 					\
	@KDRIVE_LIBS@

Xi810_LDADD = \
	$(I810_LIBS)					\
	@XSERVER_LIBS@                                  \
	$(TSLIB_FLAG)

Xi810_DEPENDENCIES = $(I810_LIBS) @KDRIVE_LIBS@
