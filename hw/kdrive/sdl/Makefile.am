INCLUDES = 					\
	@KDRIVE_PURE_INCS@				\
	@KDRIVE_CFLAGS@	\
	@XSDL_INCS@

bin_PROGRAMS = Xsdl

if TSLIB
TSLIB_FLAG = -lts
endif

Xsdl_SOURCES = sdl.c

Xsdl_LDADD = @KDRIVE_PURE_LIBS@				\
	@KDRIVE_LIBS@                                  \
	@XSERVER_LIBS@					\
	$(TSLIB_FLAG)					\
	@XSDL_LIBS@
