INCLUDES = 					\
	@KDRIVE_INCS@				\
	-I$(top_srcdir)/hw/kdrive/vesa		\
	@XSERVER_CFLAGS@

bin_PROGRAMS = Xneomagic

noinst_LIBRARIES = libneomagic.a

libneomagic_a_SOURCES = 	\
	neomagic.c	\
	neomagic.h	\
	neo_draw.c

Xneomagic_SOURCES = \
	neomagicstub.c

NEOMAGIC_LIBS =					\
	libneomagic.a 				\
	$(top_builddir)/hw/kdrive/vesa/libvesa.a \
	@KDRIVE_LIBS@

Xneomagic_LDADD = \
	$(NEOMAGIC_LIBS)				\
	@KDRIVE_LIBS@				\
	@XSERVER_LIBS@

Xneomagic_DEPENDENCIES = $(NEOMAGIC_LIBS) @KDRIVE_LIBS@
