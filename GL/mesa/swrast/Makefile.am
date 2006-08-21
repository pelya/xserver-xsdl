noinst_LTLIBRARIES = libswrast.la

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
           -I../shader/slang \
	   -I../shader/slang \
           -I../swrast \
           -I../swrast_setup \
           -I../tnl \
           -I.. \
           -I$(top_srcdir)/hw/xfree86/os-support

nodist_libswrast_la_SOURCES = s_aaline.c \
                      s_aatriangle.c \
                      s_accum.c \
                      s_alpha.c \
                      s_arbshader.c \
                      s_atifragshader.c \
                      s_bitmap.c \
                      s_blend.c \
                      s_blit.c \
                      s_buffers.c \
                      s_context.c \
                      s_copypix.c \
                      s_depth.c \
                      s_drawpix.c \
                      s_feedback.c \
                      s_fog.c \
                      s_imaging.c \
                      s_lines.c \
                      s_logic.c \
                      s_masking.c \
                      s_nvfragprog.c \
                      s_points.c \
                      s_readpix.c \
                      s_span.c \
                      s_stencil.c \
		      s_texcombine.c \
		      s_texfilter.c \
                      s_texstore.c \
                      s_triangle.c \
                      s_zoom.c
