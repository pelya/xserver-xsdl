SUBDIRS = main math array_cache swrast swrast_setup tnl shader X glapi

noinst_LTLIBRARIES = libGLcore.la

libGLcore_la_SOURCES = dummy.c
libGLcore_la_LIBADD = main/libmain.la \
                      math/libmath.la \
                      array_cache/libac.la \
                      swrast/libswrast.la \
                      swrast_setup/libss.la \
                      tnl/libtnl.la \
                      shader/libshader.la \
		      shader/grammar/libgrammar.la \
		      shader/slang/libslang.la \
                      X/libX.la
