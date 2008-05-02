INCLUDES = 					\
	@KDRIVE_PURE_INCS@				\
	@KDRIVE_CFLAGS@	\
	@XSDL_INCS@

bin_PROGRAMS = Xsdl

Xsdl_SOURCES = sdl.c

if GLX
Xsdl_LDFLAGS = $(LD_EXPORT_SYMBOLS_FLAG)
endif

Xsdl_LDADD = \
	@KDRIVE_LIBS@                                  \
	@XSDL_LIBS@

Xsdl_DEPENDENCIES = @KDRIVE_LOCAL_LIBS@

relink:
	rm -f $(bin_PROGRAMS) && make $(bin_PROGRAMS)
