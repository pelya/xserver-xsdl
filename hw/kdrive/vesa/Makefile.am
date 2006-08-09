INCLUDES = 					\
	@KDRIVE_INCS@				\
	@KDRIVE_CFLAGS@

noinst_LIBRARIES = libvesa.a

bin_PROGRAMS = Xvesa

if TSLIB
TSLIB_FLAG = -lts
endif


libvesa_a_SOURCES = \
	vesa.c		\
	vesa.h		\
	vbe.c		\
	vbe.h		\
	vga.c		\
	vga.h		\
	vm86.c		\
	vm86.h

Xvesa_SOURCES = \
	vesainit.c

Xvesa_LDADD = \
	libvesa.a 				\
	@KDRIVE_LIBS@				\
	@KDRIVE_LIBS@                          \
	$(TSLIB_FLAG)				\
	@XSERVER_LIBS@
