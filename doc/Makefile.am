appmandir =  $(mandir)/man$(APP_MAN_SUFFIX)

# Xserver.man covers options generic to all X servers built in this tree
appman_SOURCES = Xserver.man.pre

appman_PROCESSED = $(appman_SOURCES:man.pre=man)

appman_DATA = $(appman_SOURCES:man.pre=@APP_MAN_SUFFIX@)

BUILT_SOURCES = $(appman_PROCESSED)

CLEANFILES = $(appman_PROCESSED) $(appman_DATA)

include $(top_srcdir)/cpprules.in

.man.$(APP_MAN_SUFFIX):
	cp $< $@

EXTRAMANDEFS = -D__default_font_path__="`echo $(COMPILEDDEFAULTFONTPATH) | sed -e 's/,/, /g'`"

# Docs about X server internals that we ship with source but don't install
DEVEL_DOCS = smartsched


EXTRA_DIST = $(DEVEL_DOCS) $(appman_SOURCES)
