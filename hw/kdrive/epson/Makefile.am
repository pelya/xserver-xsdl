INCLUDES = 					\
	@KDRIVE_INCS@				\
	@XSERVER_CFLAGS@

bin_PROGRAMS = Xepson

if TSLIB
TSLIB_FLAG = -lts
endif

noinst_LIBRARIES = libepson.a

libepson_a_SOURCES = 	\
	epson13806.c	\
	epson13806draw.c
Xepson_SOURCES = \
	epson13806stub.c

EPSON_LIBS =					\
	libepson.a 				\
	@KDRIVE_LIBS@

Xepson_LDADD = \
	$(EPSON_LIBS)				\
	@XSERVER_LIBS@                          \
	$(TSLIB_FLAG)

Xepson_DEPENDENCIES = $(EPSON_LIBS) @KDRIVE_LIBS@
