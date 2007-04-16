noinst_LTLIBRARIES = libtnl.la

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
           -I../swrast \
           -I../swrast_setup \
           -I../tnl \
           -I.. \
           -I$(top_srcdir)/hw/xfree86/os-support

nodist_libtnl_la_SOURCES = t_context.c \
                      t_draw.c \
                      t_pipeline.c \
                      t_vb_cull.c \
                      t_vb_fog.c \
                      t_vb_light.c \
                      t_vb_normals.c \
                      t_vb_points.c \
                      t_vb_program.c \
                      t_vb_render.c \
                      t_vb_texgen.c \
                      t_vb_texmat.c \
                      t_vb_vertex.c \
                      t_vertex.c \
                      t_vertex_generic.c \
                      t_vertex_sse.c \
                      t_vp_build.c
