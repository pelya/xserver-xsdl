# Xserver.man covers options generic to all X servers built in this tree
MAN_SRCS = Xorg.man.pre Xorg.conf.man.pre

man1_MANS = Xorg.man

filemandir = $(mandir)/man$(FILE_MAN_SUFFIX)
fileman_DATA = xorg.conf.$(FILE_MAN_SUFFIX)

CLEANFILES = $(man1_MANS) $(fileman_DATA)

xorg.conf.$(FILE_MAN_SUFFIX): xorg.conf.man
	-rm -f xorg.conf.$(FILE_MAN_SUFFIX)
	$(LN_S) xorg.conf.man xorg.conf.$(FILE_MAN_SUFFIX)

include $(top_srcdir)/cpprules.in

EXTRAMANDEFS = -D__logdir__=$(logdir)

EXTRA_DIST = $(MAN_SRCS)