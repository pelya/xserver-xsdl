/*
 *
 * Copyright 1990, 1998  The Open Group
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of The Open Group shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from The Open Group.
 *
 * Author:  Keith Packard, MIT X Consortium
 */

/*
 * dmxeq.c is derived from mi/mieq.c so that XInput events can be handled
 *
 * Modified by: Rickard E. (Rik) Faith <faith@redhat.com>
 *
 * Copyright 2002-2003 Red Hat Inc., Durham, North Carolina.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation on the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT.  IN NO EVENT SHALL RED HAT AND/OR THEIR SUPPLIERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/** \file
 * This file provides an event queue that knows about XInput events.
 * All of the code is based on mi/mieq.c and was modified as little as
 * possible to provide XInput event support (the copyright and some of
 * the comments are from The Open Group, Keith Packard, MIT X
 * Consortium).  (Another example of similar code is provided in
 * hw/xfree86/common/xf86Xinput.c.) */

#ifdef HAVE_DMX_CONFIG_H
#include <dmx-config.h>
#endif

#define DMX_EQ_DEBUG 0

#include "dmx.h"
#include "dmxeq.h"
#include "dmxinput.h"
#include "dmxlog.h"
#include "dmxdpms.h"

#include "inputstr.h"
#include "scrnintstr.h"         /* For screenInfo */

#include <X11/extensions/XIproto.h>

#if DMX_EQ_DEBUG
#define DMXDBG2(f,a,b)           dmxLog(dmxDebug,f,a,b)
#define DMXDBG5(f,a,b,c,d,e)     dmxLog(dmxDebug,f,a,b,c,d,e)
#else
#define DMXDBG2(f,a,b)
#define DMXDBG5(f,a,b,c,d,e)
#endif
