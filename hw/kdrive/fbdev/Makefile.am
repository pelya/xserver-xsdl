INCLUDES = 					\
	-I$(top_srcdir)/fb			\
	-I$(top_srcdir)/hw/kdrive/src		\
	-I$(top_srcdir)/include			\
	-I$(top_srcdir)/mi			\
	-I$(top_srcdir)/miext/layer		\
	-I$(top_srcdir)/miext/shadow		\
	-I$(top_srcdir)/randr			\
	-I$(top_srcdir)/render


bin_PROGRAMS = Xfbdev

noinst_LTLIBRARIES = libfbdev.la

libfbdev_la_SOURCES = \
	fbdev.c

Xfbdev_SOURCES = \
	fbinit.c

Xfbdev_LDADD = \
	$(top_builddir)/hw/kdrive/fbdev/libfbdev.la	\
	$(top_builddir)/dix/libdix.la			\
	$(top_builddir)/os/libos.la			\
	$(top_builddir)/miext/layer/liblayer.la		\
	$(top_builddir)/hw/kdrive/src/libkdrive.la  	\
	$(top_builddir)/hw/kdrive/linux/liblinux.la  	\
	$(top_builddir)/miext/shadow/libshadow.la 	\
	$(top_builddir)/randr/librandr.la		\
	$(top_builddir)/render/librender.la		\
	$(top_builddir)/fb/libfb.la			\
	$(top_builddir)/mi/libmi.la			\
	$(top_builddir)/Xext/libXext.la			\
	$(top_builddir)/randr/librandr.la		\
	$(top_builddir)/render/librender.la		\
	$(top_builddir)/xtrans/libxtrans.la		\
	@XFONT_LIBS@					\
	$(X_LIBS) -lXau -lXdmcp 			\
	$(top_builddir)/dix/libxpstubs.la		\
	-lfreetype -lm -lz

