INCLUDES = 					\
	-I$(top_srcdir)/fb			\
	-I$(top_srcdir)/hw/kdrive/src		\
	-I$(top_srcdir)/hw/kdrive/vesa		\
	-I$(top_srcdir)/hw/kdrive/linux		\
	-I$(top_srcdir)/include			\
	-I$(top_srcdir)/mi			\
	-I$(top_srcdir)/miext/layer		\
	-I$(top_srcdir)/miext/shadow		\
	-I$(top_srcdir)/randr			\
	-I$(top_srcdir)/render			\
	$(XSERVER_CFLAGS)

bin_PROGRAMS = Xmga

noinst_LIBRARIES = libmga.a

libmga_a_SOURCES = 	\
	mgadraw.c	\
	mga.c		\
	mga.h	

Xmga_SOURCES = \
	mgastub.c

Xmga_LDADD = \
	libmga.a 				\
	$(top_builddir)/hw/kdrive/vesa/libvesa.a \
	$(top_builddir)/dix/libdix.a		\
	$(top_builddir)/os/libos.a		\
	$(top_builddir)/miext/layer/liblayer.a	\
	$(top_builddir)/hw/kdrive/src/libkdrive.a  \
	$(top_builddir)/hw/kdrive/linux/liblinux.a  \
	$(top_builddir)/miext/shadow/libshadow.a \
	$(top_builddir)/randr/librandr.a	\
	$(top_builddir)/render/librender.a	\
	$(top_builddir)/xfixes/libxfixes.a	\
	$(top_builddir)/fb/libfb.a		\
	$(top_builddir)/mi/libmi.a		\
	$(top_builddir)/Xext/libXext.a		\
	$(top_builddir)/randr/librandr.a	\
	$(top_builddir)/render/librender.a	\
	$(top_builddir)/xfixes/libxfixes.a	\
	$(top_builddir)/dix/libxpstubs.a	\
	$(XSERVER_LIBS)				\
	-lm -lz
