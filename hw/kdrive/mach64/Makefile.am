INCLUDES = 					\
	-I$(top_srcdir)/fb			\
	-I$(top_srcdir)/hw/kdrive/src		\
	-I$(top_srcdir)/hw/kdrive/vesa		\
	-I$(top_srcdir)/include			\
	-I$(top_srcdir)/mi			\
	-I$(top_srcdir)/miext/layer		\
	-I$(top_srcdir)/miext/shadow		\
	-I$(top_srcdir)/randr			\
	-I$(top_srcdir)/render


bin_PROGRAMS = Xmach64

noinst_LTLIBRARIES = libmach64.la

libmach64_la_SOURCES = 	\
	mach64.c	\
	mach64draw.c	\
	mach64video.c	\
	fourcc.h	\
	mach64.h	\
	mach64draw.h

	
Xmach64_SOURCES = \
	mach64stub.c

Xmach64_LDADD = \
	$(top_builddir)/hw/kdrive/mach64/libmach64.la \
	$(top_builddir)/hw/kdrive/vesa/libvesa.la \
	$(top_builddir)/dix/libdix.la		\
	$(top_builddir)/os/libos.la		\
	$(top_builddir)/miext/layer/liblayer.la	\
	$(top_builddir)/hw/kdrive/src/libkdrive.la  \
	$(top_builddir)/hw/kdrive/linux/liblinux.la  \
	$(top_builddir)/miext/shadow/libshadow.la \
	$(top_builddir)/randr/librandr.la	\
	$(top_builddir)/render/librender.la	\
	$(top_builddir)/fb/libfb.la		\
	$(top_builddir)/mi/libmi.la		\
	$(top_builddir)/Xext/libXext.la		\
	$(top_builddir)/randr/librandr.la	\
	$(top_builddir)/render/librender.la	\
	$(top_builddir)/xtrans/libxtrans.la	\
	@XFONT_LIBS@				\
	$(X_LIBS) -lXau -lXdmcp 		\
	$(top_builddir)/dix/libxpstubs.la	\
	-lfreetype -lm -lz
