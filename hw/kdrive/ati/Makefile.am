INCLUDES = 					\
	@KDRIVE_INCS@				\
	-I$(top_srcdir)/hw/kdrive/vesa		\
	@XSERVER_CFLAGS@

bin_PROGRAMS = Xati

if TSLIB
TSLIB_FLAG = -lts
endif

noinst_LIBRARIES = libati.a

libati_a_SOURCES = 	\
	ati_draw.c	\
	ati.c		\
	ati.h		\
	ati_reg.h

Xati_SOURCES = \
	ati_stub.c

Xati_LDADD = \
	libati.a 				\
	$(top_builddir)/hw/kdrive/vesa/libvesa.a \
	@KDRIVE_LIBS@				\
	@XSERVER_LIBS@                          \
	$(TSLIB_FLAG)

