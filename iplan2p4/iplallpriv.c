/* $XFree86: xc/programs/Xserver/iplan2p4/iplallpriv.c,v 3.0 1996/08/18 01:54:33 dawes Exp $ */
/*
 * $XConsortium: iplallpriv.c,v 1.5 94/04/17 20:28:42 dpw Exp $
 *
Copyright (c) 1991  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.
 *
 * Author:  Keith Packard, MIT X Consortium
 */

/* Modified nov 94 by Martin Schaller (Martin_Schaller@maus.r.de) for use with
interleaved planes */

#include "X.h"
#include "Xmd.h"
#include "servermd.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "resource.h"
#include "colormap.h"
#include "colormapst.h"
#include "ipl.h"
#include "mi.h"
#include "mistruct.h"
#include "dix.h"
#include "mibstore.h"

int iplWindowPrivateIndex;
int iplGCPrivateIndex;
#ifdef CFB_NEED_SCREEN_PRIVATE
int iplScreenPrivateIndex;
#endif

extern RegionPtr (*iplPuntCopyPlane)();

Bool
iplAllocatePrivates(pScreen, window_index, gc_index)
    ScreenPtr	pScreen;
    int		*window_index, *gc_index;
{
    if (!window_index || !gc_index ||
	*window_index == -1 && *gc_index == -1)
    {
    	if (!mfbAllocatePrivates(pScreen,
			     	 &iplWindowPrivateIndex, &iplGCPrivateIndex))
	    return FALSE;
    	if (window_index)
	    *window_index = iplWindowPrivateIndex;
    	if (gc_index)
	    *gc_index = iplGCPrivateIndex;
    }
    else
    {
	iplWindowPrivateIndex = *window_index;
	iplGCPrivateIndex = *gc_index;
    }
    if (!AllocateWindowPrivate(pScreen, iplWindowPrivateIndex,
			       sizeof(iplPrivWin)) ||
	!AllocateGCPrivate(pScreen, iplGCPrivateIndex, sizeof(iplPrivGC)))
	return FALSE;
	
    iplPuntCopyPlane = miCopyPlane;
#ifdef CFB_NEED_SCREEN_PRIVATE
    iplScreenPrivateIndex = AllocateScreenPrivateIndex ();
    if (iplScreenPrivateIndex == -1)
	return FALSE;
#endif
    return TRUE;
}
