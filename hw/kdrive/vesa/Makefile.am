INCLUDES = 					\
	-I$(top_srcdir)/fb			\
	-I$(top_srcdir)/hw/kdrive		\
	-I$(top_srcdir)/include			\
	-I$(top_srcdir)/mi			\
	-I$(top_srcdir)/miext/layer		\
	-I$(top_srcdir)/miext/shadow		\
	-I$(top_srcdir)/randr			\
	-I$(top_srcdir)/render


bin_PROGRAMS = Xvesa

Xvesa_SOURCES = \
	vesa.c		\
	vesainit.c	\
	vbe.c		\
	vga.c		\
	vm86.c

Xvesa_LDADD = \
	$(top_builddir)/dix/libdix.la		\
	$(top_builddir)/hw/kdrive/libkdrive.la  \
	$(top_builddir)/hw/kdrive/linux/liblinux.la  \
	$(top_builddir)/fb/libfb.la		\
	$(top_builddir)/mi/libmi.la		\
	$(top_builddir)/Xext
	$(top_builddir)/miext/layer/liblayer.la	\
	$(top_builddir)/miext/shadow/libshadow.la \
	$(top_builddir)/render/librender.la	\
	$(top_builddir)/randr/librandr.la	\
	$(top_builddir)/os/libos.la		\
	$(top_builddir)/xtrans/libxtrans.la	\
	$(X_LIBS) -lXfont -lXau -lXdmcp 	\
	-lm -lz
