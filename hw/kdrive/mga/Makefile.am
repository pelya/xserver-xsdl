INCLUDES = 					\
	@KDRIVE_INCS@				\
	-I$(top_srcdir)/hw/kdrive/vesa		\
	@XSERVER_CFLAGS@

bin_PROGRAMS = Xmga

noinst_LIBRARIES = libmga.a

libmga_a_SOURCES = 	\
	mgadraw.c	\
	mga.c		\
	mga.h	

Xmga_SOURCES = \
	mgastub.c

Xmga_LDADD = \
	libmga.a 					\
	$(top_builddir)/hw/kdrive/vesa/libvesa.a 	\
	@KDRIVE_LIBS@					\
	@XSERVER_LIBS@
