noinst_LTLIBRARIES = libslang.la

INCLUDES = -I@MESA_SOURCE@/include \
           -I../grammar \
           -I../../X \
           -I../../array_cache \
           -I../../glapi \
           -I../../main \
           -I../../math \
           -I../../shader \
           -I../../swrast \
           -I../../swrast_setup \
           -I../../tnl \
           -I../..

nodist_libslang_la_SOURCES = slang_assemble_assignment.c \
		      slang_assemble.c \
		      slang_assemble_conditional.c \
		      slang_assemble_constructor.c \
		      slang_assemble_typeinfo.c \
		      slang_compile.c \
		      slang_execute.c \
		      slang_preprocess.c \
		      slang_storage.c \
		      slang_utility.c
