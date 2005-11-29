noinst_LTLIBRARIES = libxtrap.la libxtrapmodule.la

AM_CFLAGS = $(DIX_CFLAGS) @LOADER_DEFINES@

libxtrap_la_SOURCES = xtrapddmi.c xtrapdi.c xtrapdiswp.c xtrapditbl.c

libxtrapmodule_la_CFLAGS = \
	$(AM_CFLAGS) \
	@MODULE_DEFINES@ \
	-I$(top_srcdir)/hw/xfree86/os-support
libxtrapmodule_la_SOURCES = xtrapddmi.c xtrapdi.c xtrapdiswp.c xtrapditbl.c
