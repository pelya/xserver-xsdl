INCLUDES = 					\
        @KDRIVE_INCS@				\
	-I$(top_srcdir)/hw/kdrive/vesa		\
	@KDRIVE_CFLAGS@

bin_PROGRAMS = Xmach64

if TSLIB
TSLIB_FLAG = -lts
endif


noinst_LIBRARIES = libmach64.a

libmach64_a_SOURCES = 	\
	mach64.c	\
	mach64draw.c	\
	mach64video.c	\
	mach64.h	\
	mach64draw.h

Xmach64_SOURCES = \
	mach64stub.c

MACH64_LIBS =						\
	libmach64.a 					\
	$(top_builddir)/hw/kdrive/vesa/libvesa.a        


Xmach64_LDADD = 					\
	$(MACH64_LIBS)					\
	@KDRIVE_LIBS@					\
	@XSERVER_LIBS@					\
	$(TSLIB_FLAG)


Xmach64_DEPENDENCIES = $(MACH64_LIBS)
