include $(top_srcdir)/cpprules.in

noinst_PRE = xprint
BUILT_SOURCES = $(noinst_PRE)
CLEANFILES = $(noinst_PRE) xprint.pre

CPP_FILES_FLAGS = -DProjectRoot='"$(PROJECTROOT)"' -DOS_LINUX

xprint.pre: xprint.cpp
	$(SED) -e 's/^\(\s\+\)#/\1XCOMM/' $< > $@

EXTRA_DIST = xprint.cpp
