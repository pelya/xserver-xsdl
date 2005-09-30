# Xserver.man covers options generic to all X servers built in this tree
MAN_SRCS = Xserver.man.pre

man1_MANS = Xserver.man

all-local: $(man1_MANS)

CLEANFILES = $(man1_MANS)

include $(top_srcdir)/cpprules.in

EXTRAMANDEFS = -D__default_font_path__="`echo $(COMPILEDDEFAULTFONTPATH) | sed -e 's/,/, /g'`"

# Docs about X server internals that we ship with source but don't install
DEVEL_DOCS = smartsched


EXTRA_DIST = $(DEVEL_DOCS) $(MAN_SRCS)