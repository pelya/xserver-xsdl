# Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
# 
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, and/or sell copies of the Software, and to permit persons
# to whom the Software is furnished to do so, provided that the above
# copyright notice(s) and this permission notice appear in all copies of
# the Software and that both the above copyright notice(s) and this
# permission notice appear in supporting documentation.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
# OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
# HOLDERS INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL
# INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING
# FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
# NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
# WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
# 
# Except as contained in this notice, the name of a copyright holder
# shall not be used in advertising or otherwise to promote the sale, use
# or other dealings in this Software without prior written authorization
# of the copyright holder.
# 

dist_bin_SCRIPTS = getconfig getconfig.pl

getconfigdir = $(libdir)/X11/getconfig
dist_getconfig_DATA = xorg.cfg cfg.sample

man1_MANS = getconfig.man

include $(top_srcdir)/cpprules.in

filemandir = $(mandir)/man$(FILE_MAN_SUFFIX)
fileman_DATA = getconfig.$(FILE_MAN_SUFFIX)

getconfig.$(FILE_MAN_SUFFIX): cfg.man
	-rm -f getconfig.$(FILE_MAN_SUFFIX)
	$(LN_S) cfg.man getconfig.$(FILE_MAN_SUFFIX)

CLEANFILES = $(fileman_DATA) cfg.man getconfig.man

EXTRA_DIST = cfg.man.pre getconfig.man.pre
