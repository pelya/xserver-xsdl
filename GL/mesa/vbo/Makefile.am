noinst_LTLIBRARIES = libvbo.la

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
           -I../shader/slang \
	   -I../shader/slang \
           -I../swrast \
           -I../swrast_setup \
           -I../tnl \
           -I.. \
           -I$(top_srcdir)/hw/xfree86/os-support

nodist_libvbo_la_SOURCES = vbo_context.c \
			vbo_exec_api.c \
			vbo_exec_array.c \
			vbo_exec.c \
			vbo_exec_draw.c \
			vbo_exec_eval.c \
			vbo_rebase.c \
			vbo_save_api.c \
			vbo_save.c \
			vbo_save_draw.c \
			vbo_save_loopback.c \
			vbo_split.c \
			vbo_split_copy.c \
			vbo_split_inplace.c
