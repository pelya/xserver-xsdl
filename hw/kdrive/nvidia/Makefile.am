INCLUDES = 					\
	@KDRIVE_INCS@				\
	-I$(top_srcdir)/hw/kdrive/vesa		\
	@XSERVER_CFLAGS@

bin_PROGRAMS = Xnvidia

noinst_LIBRARIES = libnvidia.a

#	nvidiavideo.c
	
libnvidia_a_SOURCES = 	\
	nvidia.c	\
	nvidiadraw.c	\
	fourcc.h	\
	nvidia.h	\
	nvidiadraw.h

	
Xnvidia_SOURCES = \
	nvidiastub.c

Xnvidia_LDADD = \
	libnvidia.a 				\
	$(top_builddir)/hw/kdrive/vesa/libvesa.a \
	@KDRIVE_LIBS@				\
	@XSERVER_LIBS@
