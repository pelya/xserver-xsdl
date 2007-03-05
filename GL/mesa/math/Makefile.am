noinst_LTLIBRARIES = libmath.la

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
           -I../swrast \
           -I../swrast_setup \
           -I../tnl \
           -I.. \
           -I$(top_srcdir)/hw/xfree86/os-support

nodist_libmath_la_SOURCES = m_debug_clip.c \
                       m_debug_norm.c \
                       m_debug_xform.c \
                       m_eval.c \
                       m_matrix.c \
                       m_translate.c \
                       m_vector.c \
                       m_xform.c
