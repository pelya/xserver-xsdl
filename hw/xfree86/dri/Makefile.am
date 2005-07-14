libdri_la_LTLIBRARIES = libdri.la
libdri_la_CFLAGS = -I$(top_srcdir)/hw/xfree86/common -I$(top_srcdir)/hw/xfree86/os-support/bus -I$(top_srcdir)/GL/glx/ -I$(top_srcdir)/GL/include -DXF86DRI -DXORG_VERSION_CURRENT=\"100\"
libdri_la_LDFLAGS = -module -avoid-version
libdri_ladir = @moduledir@/extensions
libdri_la_SOURCES = \
	dri.c \
	dri.h \
	drimodule.c \
	dristruct.h \
	sarea.h \
	xf86dri.c

sdk_HEADERS = dri.h sarea.h dristruct.h
