SUBDIRS =	\
	vesa

INCLUDES = 					\
	-I$(top_srcdir)/fb			\
	-I$(top_srcdir)/include			\
	-I$(top_srcdir)/mi			\
	-I$(top_srcdir)/miext/shadow		\
	-I$(top_srcdir)/randr			\
	-I$(top_srcdir)/render			


noinst_LTLIBRARIES = libkdrive.la

libkdrive_la_SOURCES =	\
	kaa.c		\
	kasync.c	\
	kcmap.c		\
	kcolor.c	\
	kcurscol.c	\
	kdrive.c	\
	kinfo.c		\
	kinput.c	\
	kmap.c		\
	kmode.c		\
	knoop.c		\
	kpict.c		\
	kshadow.c	\
	ktest.c		\
	kxv.c		\
	vga.c

libkdrive_la_LIBADD = 				\
	$(top_builddir)/dix/libdix.la		\
	$(top_builddir)/fb/libfb.la		\
	$(top_builddir)/mi/libmi.la		\
	$(top_builddir)/render/librender.la	\
	$(top_builddir)/randr/librandr.la	\
	$(top_builddir)/os/libos.la		\
	$(X_LIBS) -lXfont


bin_PROGRAMS = Xvesa

Xvesa_SOURCES = # Intentionally left blank

Xvesa_LDADD = $(top_builddir)/hw/kdrive/libkdrive.la
