SUBDIRS = grammar slang

noinst_LTLIBRARIES = libshader.la

AM_CFLAGS = \
	$(DIX_CFLAGS) \
	-DXFree86Server \
	@GLX_DEFINES@

INCLUDES = -I@MESA_SOURCE@/include \
           -I../X \
           -I../glapi \
           -I../main \
           -I../math \
           -I../shader \
	   -I../shader/grammar \
	   -I../shader/slang \
           -I../swrast \
           -I../swrast_setup \
           -I../tnl \
           -I.. \
           -I$(top_srcdir)/hw/xfree86/os-support

nodist_libshader_la_SOURCES = \
                      arbprogparse.c \
                      arbprogram.c \
                      atifragshader.c \
                      nvfragparse.c \
                      nvprogram.c \
                      nvvertparse.c \
                      prog_debug.c \
                      prog_execute.c \
                      prog_instruction.c \
                      prog_parameter.c \
                      prog_print.c \
                      program.c \
                      programopt.c \
                      prog_statevars.c \
                      shader_api.c
