INCLUDES = 					\
	@KDRIVE_INCS@				\
	-I$(top_srcdir)/hw/kdrive/vesa		\
	@XSERVER_CFLAGS@

bin_PROGRAMS = Xr128

if TSLIB
TSLIB_FLAG = -lts
endif

noinst_LIBRARIES = libr128.a

libr128_a_SOURCES = 	\
	r128draw.c	\
	r128.c		\
	r128.h	

Xr128_SOURCES = \
	r128stub.c

R128_LIBS =					\
	libr128.a 				\
	$(top_builddir)/hw/kdrive/vesa/libvesa.a \
	@KDRIVE_LIBS@

Xr128_LDADD = \
	$(R128_LIBS)				\
	@XSERVER_LIBS@                          \
	$(TSLIB_FLAG)

Xr128_DEPENDENCIES = $(R128_LIBS) @KDRIVE_LIBS@
