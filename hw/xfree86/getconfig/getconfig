#!/bin/sh

# $XdotOrg: xc/programs/Xserver/hw/xfree86/getconfig/getconfig.sh,v 1.2 2004/04/23 19:54:01 eich Exp $

#
# Copyright 2003 by David H. Dawes.
# Copyright 2003 by X-Oz Technologies.
# All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
# OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.
# 
# Except as contained in this notice, the name of the copyright holder(s)
# and author(s) shall not be used in advertising or otherwise to promote
# the sale, use or other dealings in this Software without prior written
# authorization from the copyright holder(s) and author(s).
# 
# Author: David Dawes <dawes@XFree86.Org>.
#

# A simple wrapper to execute the real getconfig program.  So long as perl
# is in $PATH, we don't need to know where it is this way.

if echo $0 | grep / >/dev/null 2>&1; then
	DIR=`dirname $0`/
fi

exec perl ${DIR}getconfig.pl "$@"
