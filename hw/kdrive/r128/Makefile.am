INCLUDES = 					\
	@KDRIVE_INCS@				\
	-I$(top_srcdir)/hw/kdrive/vesa		\
	@XSERVER_CFLAGS@

bin_PROGRAMS = Xr128

noinst_LIBRARIES = libr128.a

libr128_a_SOURCES = 	\
	r128draw.c	\
	r128.c		\
	r128.h	

Xr128_SOURCES = \
	r128stub.c

Xr128_LDADD = \
	libr128.a 				\
	$(top_builddir)/hw/kdrive/vesa/libvesa.a \
	@KDRIVE_LIBS@				\
	@XSERVER_LIBS@
