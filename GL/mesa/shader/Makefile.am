noinst_LTLIBRARIES = libshader.la

INCLUDES = -I@MESA_SOURCE@/include \
           -I$(srcdir)/../X \
           -I$(srcdir)/../array_cache \
           -I$(srcdir)/../glapi \
           -I$(srcdir)/../main \
           -I$(srcdir)/../math \
           -I$(srcdir)/../shader \
           -I$(srcdir)/../swrast \
           -I$(srcdir)/../swrast_setup \
           -I$(srcdir)/../tnl \
           -I$(srcdir)/..

libshader_la_SOURCES = arbfragparse.c \
                      arbprogparse.c \
                      arbprogram.c \
                      arbvertparse.c \
                      atifragshader.c \
                      nvfragparse.c \
                      nvprogram.c \
                      nvvertexec.c \
                      nvvertparse.c \
                      program.c \
                      shaderobjects.c \
                      shaderobjects_3dlabs.c \
                      grammar_mesa.c
