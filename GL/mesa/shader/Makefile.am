SUBDIRS = grammar slang

noinst_LTLIBRARIES = libshader.la

AM_CFLAGS = \
	$(DIX_CFLAGS) \
	-DXFree86Server \
	@GLX_DEFINES@

INCLUDES = -I@MESA_SOURCE@/include \
           -I../X \
           -I../array_cache \
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
                      nvvertexec.c \
		      nvvertparse.c \
                      program.c \
                      shaderobjects.c \
		      shaderobjects_3dlabs.c
