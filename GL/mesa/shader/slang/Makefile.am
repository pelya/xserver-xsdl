noinst_LTLIBRARIES = libslang.la

AM_CFLAGS = \
	$(DIX_CFLAGS) \
	-DXFree86Server \
	@GLX_DEFINES@

INCLUDES = -I@MESA_SOURCE@/include \
           -I../grammar \
           -I../../X \
           -I../../glapi \
           -I../../main \
           -I../../math \
           -I../../shader \
           -I../../swrast \
           -I../../swrast_setup \
           -I../../tnl \
           -I../.. \
           -I$(top_srcdir)/hw/xfree86/os-support

nodist_libslang_la_SOURCES = slang_builtin.c \
		      slang_codegen.c \
		      slang_compile.c \
		      slang_compile_function.c \
		      slang_compile_operation.c \
		      slang_compile_struct.c \
		      slang_compile_variable.c \
		      slang_emit.c \
		      slang_ir.c \
		      slang_label.c \
		      slang_library_noise.c \
		      slang_link.c \
		      slang_log.c \
		      slang_mem.c \
		      slang_preprocess.c \
		      slang_print.c \
		      slang_simplify.c \
		      slang_storage.c \
		      slang_typeinfo.c \
		      slang_utility.c \
		      slang_vartable.c
