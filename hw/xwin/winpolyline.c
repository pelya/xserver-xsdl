/*
 *Copyright (C) 1994-2000 The XFree86 Project, Inc. All Rights Reserved.
 *
 *Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 *"Software"), to deal in the Software without restriction, including
 *without limitation the rights to use, copy, modify, merge, publish,
 *distribute, sublicense, and/or sell copies of the Software, and to
 *permit persons to whom the Software is furnished to do so, subject to
 *the following conditions:
 *
 *The above copyright notice and this permission notice shall be
 *included in all copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *NONINFRINGEMENT. IN NO EVENT SHALL THE XFREE86 PROJECT BE LIABLE FOR
 *ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 *CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *Except as contained in this notice, the name of the XFree86 Project
 *shall not be used in advertising or otherwise to promote the sale, use
 *or other dealings in this Software without prior written authorization
 *from the XFree86 Project.
 *
 * Authors:	Harold L Hunt II
 */
/* $XFree86: xc/programs/Xserver/hw/xwin/winpolyline.c,v 1.4 2003/08/07 23:47:58 alanh Exp $ */

#include "win.h"

/* See Porting Layer Definition - p. 50 */
void
winPolyLineNativeGDI (DrawablePtr	pDrawable,
		      GCPtr		pGC,
		      int		mode,
		      int		npt,
		      DDXPointPtr	ppt)
{
  switch (pGC->lineStyle)
    {
    case LineSolid:
      if (pGC->lineWidth == 0)
	return miZeroLine (pDrawable, pGC, mode, npt, ppt);
      else
	miWideLine (pDrawable, pGC, mode, npt, ppt);
      break;
    case LineOnOffDash:
    case LineDoubleDash:
      miWideDash (pDrawable, pGC, mode, npt, ppt);
      break;
    }
}
