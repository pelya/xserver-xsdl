INCLUDES = 					\
        @KDRIVE_INCS@				\
	-I$(top_srcdir)/hw/kdrive/vesa		\
	@KDRIVE_CFLAGS@

bin_PROGRAMS = Xvia

if TSLIB
TSLIB_FLAG = -lts
endif

noinst_LIBRARIES = libvia.a

libvia_a_SOURCES = 	\
	via.c \
	viadraw.c \
	viadraw.h \
	via.h \
	via_regs.h

Xvia_SOURCES =	\
	viastub.c

VIA_LIBS =	\
	libvia.a	\
	$(top_builddir)/hw/kdrive/vesa/libvesa.a

Xvia_LDADD =						\
	$(VIA_LIBS)					\
	@KDRIVE_LIBS@					\
	@XSERVER_LIBS@					\
	$(TSLIB_FLAG)					

Xvia_DEPENDENCIES = $(VIA_LIBS)
