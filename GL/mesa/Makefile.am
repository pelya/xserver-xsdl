SUBDIRS = X
SUBDIRS += main math swrast swrast_setup tnl shader glapi vbo

noinst_LTLIBRARIES = libGLcore.la

libGLcore_la_SOURCES = dummy.c
MESA_LIBS           = main/libmain.la \
                      math/libmath.la \
                      swrast/libswrast.la \
                      swrast_setup/libss.la \
                      tnl/libtnl.la \
                      shader/libshader.la \
		      shader/grammar/libgrammar.la \
		      shader/slang/libslang.la \
                      vbo/libvbo.la

libGLcore_la_LIBADD = $(MESA_LIBS) \
                      X/libX.la
