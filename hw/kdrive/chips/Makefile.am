INCLUDES = 					\
	@KDRIVE_INCS@				\
	-I$(top_srcdir)/hw/kdrive/vesa		\
	@XSERVER_CFLAGS@

bin_PROGRAMS = Xchips

if TSLIB
TSLIB_FLAG = -lts
endif

noinst_LIBRARIES = libchips.a

libchips_a_SOURCES = 	\
	chipsdraw.c	\
	chips.c		\
	chips.h	

Xchips_SOURCES = \
	chipsstub.c

Xchips_LDADD = \
	libchips.a 				\
	$(top_builddir)/hw/kdrive/vesa/libvesa.a \
	@KDRIVE_LIBS@				\
	@XSERVER_LIBS@                          \
	$(TSLIB_FLAG)

