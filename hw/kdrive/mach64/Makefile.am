INCLUDES = 					\
	-I$(top_srcdir)/fb			\
	-I$(top_srcdir)/hw/kdrive/src		\
	-I$(top_srcdir)/hw/kdrive/linux		\
	-I$(top_srcdir)/hw/kdrive/vesa		\
	-I$(top_srcdir)/mi			\
	-I$(top_srcdir)/miext/layer		\
	-I$(top_srcdir)/miext/shadow		\
	-I$(top_srcdir)/randr			\
	-I$(top_srcdir)/render			\
	$(XSERVER_CFLAGS)

bin_PROGRAMS = Xmach64

noinst_LIBRARIES = libmach64.a

libmach64_a_SOURCES = 	\
	mach64.c	\
	mach64draw.c	\
	mach64video.c	\
	fourcc.h	\
	mach64.h	\
	mach64draw.h

	
Xmach64_SOURCES = \
	mach64stub.c

Xmach64_LDADD = \
	$(top_builddir)/hw/kdrive/mach64/libmach64.a \
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
