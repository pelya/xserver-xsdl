noinst_LTLIBRARIES = libglapi.la

AM_CFLAGS = @SERVER_DEFINES@ @MODULE_DEFINES@ @LOADER_DEFINES@ \
	-DNO_LIBCWRAPPER

INCLUDES = -I@MESA_SOURCE@/include \
           -I../X \
           -I../array_cache \
           -I../glapi \
           -I../main \
           -I../math \
           -I../shader \
           -I../swrast \
           -I../swrast_setup \
           -I../tnl \
           -I.. \
           -I$(top_srcdir)/hw/xfree86/os-support

nodist_libglapi_la_SOURCES = glapi.c glthread.c
