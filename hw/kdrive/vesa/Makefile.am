INCLUDES = 					\
	@KDRIVE_INCS@				\
	@XSERVER_CFLAGS@

noinst_LIBRARIES = libvesa.a

bin_PROGRAMS = Xvesa

if TSLIB
TSLIB_FLAG = -lts
endif


libvesa_a_SOURCES = \
	vesa.c		\
	vbe.c		\
	vga.c		\
	vm86.c

Xvesa_SOURCES = \
	vesainit.c

Xvesa_LDADD = \
	libvesa.a 				\
	@KDRIVE_LIBS@				\
	@XSERVER_LIBS@                          \
	$(TSLIB_FLAG)

Xvesa_DEPENDENCIES = \
	libvesa.a    \
	@KDRIVE_LIBS@
