INCLUDES = 					\
	@KDRIVE_PURE_INCS@				\
	@KDRIVE_CFLAGS@	\
	@XSDL_INCS@

bin_PROGRAMS = Xsdl

Xsdl_SOURCES = sdl.c android_main.c sdl_send_text.c sdl_send_text.h sdl_kdrive.h sdl_clipboard.c

Xsdl_LDADD = \
	@KDRIVE_LIBS@                                  \
	@XSDL_LIBS@

Xsdl_DEPENDENCIES = @KDRIVE_LOCAL_LIBS@

Xsdl_LDFLAGS = $(LD_EXPORT_SYMBOLS_FLAG)

relink:
	rm -f $(bin_PROGRAMS) && make $(bin_PROGRAMS)
