/*
 * $Id$
 *
 * Copyright © 2004 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _XGL_H_
#define _XGL_H_

#include <stdio.h>
#include <X11/X.h>
#define NEED_EVENTS
#include <X11/Xproto.h>
#include <X11/Xos.h>
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "servermd.h"
#include "mibstore.h"
#include "colormapst.h"
#include "gcstruct.h"
#include "inputstr.h"
#include "mipointer.h"
#include "mi.h"
#include "dix.h"
#include "shadow.h"
#include "randrstr.h"
#include "micmap.h"
#include "migc.h"

extern WindowPtr    *WindowTable;

Bool
xglCreateGC (GCPtr pGC);

void
xglFillSpans (DrawablePtr   pDrawable,
	      GCPtr	    pGC,
	      int	    n,
	      DDXPointPtr   ppt,
	      int	    *pwidth,
	      int	    fSorted);

void
xglSetSpans (DrawablePtr    pDrawable,
	     GCPtr	    pGC,
	     char	    *src,
	     DDXPointPtr    ppt,
	     int	    *pwidth,
	     int	    nspans,
	     int	    fSorted);

void
xglGetSpans(DrawablePtr	pDrawable, 
	    int		wMax, 
	    DDXPointPtr	ppt, 
	    int		*pwidth, 
	    int		nspans, 
	    char	*pchardstStart);

void
xglPushPixels (GCPtr	    pGC,
	       PixmapPtr    pBitmap,
	       DrawablePtr  pDrawable,
	       int	    dx,
	       int	    dy,
	       int	    xOrg,
	       int	    yOrg);

PixmapPtr
xglCreatePixmap (ScreenPtr  pScreen,
		 int	    width,
		 int	    height, 
		 int	    depth);

Bool
xglDestroyPixmap (PixmapPtr pPixmap);

#endif /* _XGL_H_ */
