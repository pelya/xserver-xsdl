MAN_SRCS = Xprt.man.pre

appmandir =  $(mandir)/man$(APP_MAN_SUFFIX)

appman_SOURCES = Xprt.man
appman_DATA = $(appman_SOURCES:man=@APP_MAN_SUFFIX@)

include $(top_srcdir)/cpprules.in

BUILT_SOURCES = $(appman_SOURCES)
CLEANFILES = $(appman_SOURCES) $(appman_DATA)

SUFFIXES += .$(APP_MAN_SUFFIX) .man

.man.$(APP_MAN_SUFFIX):
	-rm -f $@
	$(LN_S) $< $@

EXTRA_DIST = $(MAN_SRCS) Xprt.html Xprt.sgml
