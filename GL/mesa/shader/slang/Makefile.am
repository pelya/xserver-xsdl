noinst_LTLIBRARIES = libslang.la

INCLUDES = -I@MESA_SOURCE@/include \
           -I$(srcdir)/../grammar \
           -I$(srcdir)/../../X \
           -I$(srcdir)/../../array_cache \
           -I$(srcdir)/../../glapi \
           -I$(srcdir)/../../main \
           -I$(srcdir)/../../math \
           -I$(srcdir)/../../shader \
           -I$(srcdir)/../../swrast \
           -I$(srcdir)/../../swrast_setup \
           -I$(srcdir)/../../tnl \
           -I$(srcdir)/../..

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
