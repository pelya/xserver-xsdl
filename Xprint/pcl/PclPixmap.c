/* $Xorg: PclPixmap.c,v 1.3 2000/08/17 19:48:08 cpqbld Exp $ */
/*******************************************************************
**
**    *********************************************************
**    *
**    *  File:		PclPixmap.c
**    *
**    *  Contents:
**    *                 Pixmap handling code for the PCL DDX driver
**    *
**    *  Created:	2/19/96
**    *
**    *********************************************************
** 
********************************************************************/
/*
(c) Copyright 1996 Hewlett-Packard Company
(c) Copyright 1996 International Business Machines Corp.
(c) Copyright 1996 Sun Microsystems, Inc.
(c) Copyright 1996 Novell, Inc.
(c) Copyright 1996 Digital Equipment Corp.
(c) Copyright 1996 Fujitsu Limited
(c) Copyright 1996 Hitachi, Ltd.

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
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the names of the copyright holders shall
not be used in advertising or otherwise to promote the sale, use or other
dealings in this Software without prior written authorization from said
copyright holders.
*/
/* $XFree86: xc/programs/Xserver/Xprint/pcl/PclPixmap.c,v 1.4 2001/01/17 22:36:30 dawes Exp $ */

#include "Pcl.h"
#include "cfb.h"
#include "cfb32.h"
#include "mfb.h"
#include "pixmapstr.h"

PixmapPtr
PclCreatePixmap(ScreenPtr pScreen,
		int width,
		int height,
		int depth)
{
    if( depth == 1 )
      return mfbCreatePixmap( pScreen, width, height, depth );
    else if( depth <= 8 )
      return cfbCreatePixmap( pScreen, width, height, depth );
    else if( depth <= 32 )
      return cfb32CreatePixmap( pScreen, width, height, depth );
    return 0;
}


Bool
PclDestroyPixmap(PixmapPtr pPixmap)
{
    if( pPixmap->drawable.depth == 1 )
      return mfbDestroyPixmap( pPixmap );
    else if( pPixmap->drawable.depth <= 8 )
      return cfbDestroyPixmap( pPixmap );
    else if( pPixmap->drawable.depth <= 32 )
      return cfb32DestroyPixmap( pPixmap );
    return 0;
}
