/* $Xorg: PsColor.c,v 1.4 2001/02/09 02:04:36 xorgcvs Exp $ */
/*

Copyright 1996, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

*/
/*
 * (c) Copyright 1996 Hewlett-Packard Company
 * (c) Copyright 1996 International Business Machines Corp.
 * (c) Copyright 1996 Sun Microsystems, Inc.
 * (c) Copyright 1996 Novell, Inc.
 * (c) Copyright 1996 Digital Equipment Corp.
 * (c) Copyright 1996 Fujitsu Limited
 * (c) Copyright 1996 Hitachi, Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the names of the copyright holders
 * shall not be used in advertising or otherwise to promote the sale, use
 * or other dealings in this Software without prior written authorization
 * from said copyright holders.
 */

/*******************************************************************
**
**    *********************************************************
**    *
**    *  File:		PsColor.c
**    *
**    *  Contents:	Color routines for the PS driver
**    *
**    *  Created By:	Roger Helmendach (Liberty Systems)
**    *
**    *  Copyright:	Copyright 1996 The Open Group, Inc.
**    *
**    *********************************************************
** 
********************************************************************/
/* $XFree86: xc/programs/Xserver/Xprint/ps/PsColor.c,v 1.3 2001/12/14 19:59:15 dawes Exp $ */

#include "Ps.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "colormapst.h"

Bool
PsCreateColormap(ColormapPtr pColor)
{
  int            i;
  unsigned short rgb;
  VisualPtr      pVisual = pColor->pVisual;

  if( pVisual->class==TrueColor )
  {
    for( i=0 ; i<pVisual->ColormapEntries ; i++ )
    {
      rgb = (i<<8)|i;

      pColor->red[i].fShared = FALSE;
      pColor->red[i].co.local.red     = rgb;
      pColor->red[i].co.local.green   = 0;
      pColor->red[i].co.local.blue    = 0;

      pColor->green[i].fShared = FALSE;
      pColor->green[i].co.local.red   = 0;
      pColor->green[i].co.local.green = rgb;
      pColor->green[i].co.local.blue  = 0;

      pColor->blue[i].fShared = FALSE;
      pColor->blue[i].co.local.red    = 0;
      pColor->blue[i].co.local.green  = 0;
      pColor->blue[i].co.local.blue   = rgb;
    }
  }
  return TRUE;
}

void
PsDestroyColormap(ColormapPtr pColor)
{
}

void
PsInstallColormap(ColormapPtr pColor)
{
  PsScreenPrivPtr pPriv =
    (PsScreenPrivPtr)pColor->pScreen->devPrivates[PsScreenPrivateIndex].ptr;
  pPriv->CMap = pColor;
}

void
PsUninstallColormap(ColormapPtr pColor)
{
}

int
PsListInstalledColormaps(
  ScreenPtr pScreen,
  XID      *pCmapList)
{
  return 0;
}

void
PsStoreColors(
  ColormapPtr  pColor,
  int          ndef,
  xColorItem  *pdefs)
{
  int  i;
  for( i=0 ; i<ndef ; i++ )
  {
    if( pdefs[i].flags&DoRed )
      pColor->red[pdefs[i].pixel].co.local.red   = pdefs[i].red;
    if( pdefs[i].flags&DoGreen )
      pColor->red[pdefs[i].pixel].co.local.green = pdefs[i].green;
    if( pdefs[i].flags&DoBlue )
      pColor->red[pdefs[i].pixel].co.local.blue  = pdefs[i].blue;
  }
}

void
PsResolveColor(
  unsigned short *pRed,
  unsigned short *pGreen,
  unsigned short *pBlue,
  VisualPtr       pVisual)
{
}

int
PsGetPixelColor(ColormapPtr cMap, int pixval)
{
  int r, g, b;
  if( cMap->pVisual->class==TrueColor ) return(pixval);
  if( pixval<0 || pixval>255 ) return(0);
  r = cMap->red[pixval].co.local.red>>8;
  g = cMap->red[pixval].co.local.green>>8;
  b = cMap->red[pixval].co.local.blue>>8;
  return((r<<16)|(g<<8)|b);
}

void
PsSetFillColor(DrawablePtr pDrawable, GCPtr pGC, PsOutPtr psOut,
               ColormapPtr cMap)
{
  switch(pGC->fillStyle)
  {
    case FillSolid:
      PsOut_Color(psOut, PsGetPixelColor(cMap, pGC->fgPixel));
      break;
    case FillTiled:
      if( !PsOut_BeginPattern(psOut, pGC->tile.pixmap,
             pGC->tile.pixmap->drawable.width,
             pGC->tile.pixmap->drawable.height, PsTile, 0, 0) )
      {
        PsReplayPixmap(pGC->tile.pixmap, pDrawable);
        PsOut_EndPattern(psOut);
      }
      PsOut_SetPattern(psOut, pGC->tile.pixmap, PsTile);
      break;
    case FillStippled:
      if( !PsOut_BeginPattern(psOut, pGC->stipple,
             pGC->stipple->drawable.width,
             pGC->stipple->drawable.height, PsStip, 0,
             PsGetPixelColor(cMap, pGC->fgPixel)) )
      {
        PsReplayPixmap(pGC->stipple, pDrawable);
        PsOut_EndPattern(psOut);
      }
      PsOut_SetPattern(psOut, pGC->stipple, PsStip);
      break;
    case FillOpaqueStippled:
      if( !PsOut_BeginPattern(psOut, pGC->stipple,
             pGC->stipple->drawable.width,
             pGC->stipple->drawable.height, PsOpStip,
             PsGetPixelColor(cMap, pGC->bgPixel),
             PsGetPixelColor(cMap, pGC->fgPixel)) )
      {
        PsReplayPixmap(pGC->stipple, pDrawable);
        PsOut_EndPattern(psOut);
      }
      PsOut_SetPattern(psOut, pGC->stipple, PsOpStip);
      break;
  }
}
