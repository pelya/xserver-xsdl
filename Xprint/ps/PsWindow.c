/* $Xorg: PsWindow.c,v 1.4 2001/02/09 02:04:36 xorgcvs Exp $ */
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
**    *  File:		PsWindow.c
**    *
**    *  Contents:	Window code for PS driver.
**    *
**    *  Created By:	Roger Helmendach (Liberty Systems)
**    *
**    *  Copyright:	Copyright 1996 The Open Group, Inc.
**    *
**    *********************************************************
** 
********************************************************************/
/* $XFree86: xc/programs/Xserver/Xprint/ps/PsWindow.c,v 1.12 2001/12/19 21:55:59 dawes Exp $ */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "mistruct.h"
#include "regionstr.h"
#include "windowstr.h"
#include "gcstruct.h"

#include "Ps.h"

#if 0
/*
 * The following list of strings defines the properties which will be
 * placed on the screen's root window if the property was defined in
 * the start-up configuration resource database.
 */
static char *propStrings[] = {
	DT_PRINT_JOB_HEADER,
	DT_PRINT_JOB_TRAILER,
	DT_PRINT_JOB_COMMAND,
	DT_PRINT_JOB_EXEC_COMMAND,
	DT_PRINT_JOB_EXEC_OPTIONS,
	DT_PRINT_PAGE_HEADER,
	DT_PRINT_PAGE_TRAILER,
	DT_PRINT_PAGE_COMMAND,
	(char *)NULL
};
#endif

/*
 * PsCreateWindow - watch for the creation of the root window.
 * When it's created, register the screen with the print extension,
 * and put the default command/header properties on it.
 */
/*ARGSUSED*/

Bool
PsCreateWindow(WindowPtr pWin)
{
  PsWindowPrivPtr pPriv;

#if 0
    Bool status = Success;
    ScreenPtr pScreen = pWin->drawable.pScreen;
    PsScreenPrivPtr pScreenPriv = (PsScreenPrivPtr) 
		     pScreen->devPrivates[PsScreenPrivateIndex].ptr;
    PsWindowPrivPtr pWinPriv = (PsWindowPrivPtr)
			 pWin->devPrivates[PsWindowPrivateIndex].ptr;

    /*
     * Initialize this window's private struct.
     */
    pWinPriv->jobFileName = (char *)NULL;
    pWinPriv->pJobFile = (FILE *)NULL;
    pWinPriv->pageFileName = (char *)NULL;
    pWinPriv->pPageFile = (FILE *)NULL;
    
    if(pWin->parent == (WindowPtr)NULL)  /* root window? */
    {
	Atom propName; /* type = XA_STRING */
	char *propVal;
	int i;
        XrmDatabase rmdb = pScreenPriv->resDB;

        /*
         * Put the defaults spec'd in the config files in properties on this
	 * screen's root window.
         */
	for(i = 0; propStrings[i] != (char *)NULL; i++)
	{
            if((propVal = _DtPrintGetPrinterResource(pWin, rmdb, 
						     propStrings[i])) !=
	       (char *)NULL)
	    {
                propName = MakeAtom(propStrings[i], strlen(propStrings[i]),
				    TRUE);
	        ChangeWindowProperty(pWin, propName, XA_STRING, 8, 
			             PropModeReplace,  strlen(propVal), 
			             (pointer)propVal, FALSE);
	        xfree(propVal);
	    }
	}
    }

    return status;
#endif

  pPriv = (PsWindowPrivPtr)pWin->devPrivates[PsWindowPrivateIndex].ptr;
  pPriv->validContext = 0;

  return TRUE;
}


/*ARGSUSED*/
Bool PsMapWindow(WindowPtr pWindow)
{
  return TRUE;
}

/*ARGSUSED*/
Bool 
PsPositionWindow(
  WindowPtr pWin,
  int x,
  int y)
{
  return TRUE;
}

/*ARGSUSED*/
Bool 
PsUnmapWindow(WindowPtr pWindow)
{
  return TRUE;
}

/*ARGSUSED*/
void 
PsCopyWindow(
  WindowPtr   pWin,
  DDXPointRec ptOldOrg,
  RegionPtr   prgnSrc)
{
}

/*ARGSUSED*/
Bool
PsChangeWindowAttributes(
  WindowPtr     pWin,
  unsigned long mask)
{
  return TRUE;
}


void
PsPaintWindow(
  WindowPtr pWin,
  RegionPtr pRegion,
  int       what)
{
  WindowPtr pRoot;

#define FUNCTION        0
#define FOREGROUND      1
#define TILE            2
#define FILLSTYLE       3
#define ABSX            4
#define ABSY            5
#define CLIPMASK        6
#define SUBWINDOW       7
#define COUNT_BITS      8

  pointer              gcval[7];
  pointer              newValues [COUNT_BITS];

  BITS32               gcmask, index, mask;
  RegionRec            prgnWin;
  DDXPointRec          oldCorner;
  BoxRec               box;
  WindowPtr            pBgWin;
  GCPtr                pGC;
  register int         i;
  register BoxPtr      pbox;
  register ScreenPtr   pScreen = pWin->drawable.pScreen;
  register xRectangle *prect;
  int                  numRects;

  gcmask = 0;

  /*
   * We don't want to paint a window that has no place to put the
   * PS output.
   */
  if( PsGetContextFromWindow(pWin)==(XpContextPtr)NULL ) return;

  if( what==PW_BACKGROUND )
  {
    switch(pWin->backgroundState)
    {
      case None: return;
      case ParentRelative:
        (*pWin->parent->drawable.pScreen->PaintWindowBackground)
          (pWin->parent, pRegion, what);
        return;
      case BackgroundPixel:
        newValues[FOREGROUND] = (pointer)pWin->background.pixel;
        newValues[FILLSTYLE] = (pointer)FillSolid;
        gcmask |= GCForeground | GCFillStyle;
        break;
      case BackgroundPixmap:
        newValues[TILE] = (pointer)pWin->background.pixmap;
        newValues[FILLSTYLE] = (pointer)FillTiled;
        gcmask |= GCTile | GCFillStyle | GCTileStipXOrigin | GCTileStipYOrigin;
        break;
    }
  }
  else
  {
    if( pWin->borderIsPixel )
    {
      newValues[FOREGROUND] = (pointer)pWin->border.pixel;
      newValues[FILLSTYLE] = (pointer)FillSolid;
      gcmask |= GCForeground | GCFillStyle;
    }
    else
    {
      newValues[TILE] = (pointer)pWin->border.pixmap;
      newValues[FILLSTYLE] = (pointer)FillTiled;
      gcmask |= GCTile | GCFillStyle | GCTileStipXOrigin | GCTileStipYOrigin;
    }
  }

  prect = (xRectangle *)ALLOCATE_LOCAL(REGION_NUM_RECTS(pRegion) *
                                         sizeof(xRectangle));
  if( !prect ) return;

  newValues[FUNCTION] = (pointer)GXcopy;
  gcmask |= GCFunction | GCClipMask;

  i = pScreen->myNum;
  pRoot = WindowTable[i];

  pBgWin = pWin;
  if (what == PW_BORDER)
  {
    while( pBgWin->backgroundState==ParentRelative ) pBgWin = pBgWin->parent;
  }

  pGC = GetScratchGC(pWin->drawable.depth, pWin->drawable.pScreen);
  if( !pGC )
  {
    DEALLOCATE_LOCAL(prect);
    return;
  }
  /*
   * mash the clip list so we can paint the border by
   * mangling the window in place, pretending it
   * spans the entire screen
   */
  if( what==PW_BORDER )
  {
    prgnWin = pWin->clipList;
    oldCorner.x = pWin->drawable.x;
    oldCorner.y = pWin->drawable.y;
    pWin->drawable.x = pWin->drawable.y = 0;
    box.x1 = 0;
    box.y1 = 0;
    box.x2 = pScreen->width;
    box.y2 = pScreen->height;
    REGION_INIT(pScreen, &pWin->clipList, &box, 1);
    pWin->drawable.serialNumber = NEXT_SERIAL_NUMBER;
    newValues[ABSX] = (pointer)(long)pBgWin->drawable.x;
    newValues[ABSY] = (pointer)(long)pBgWin->drawable.y;
  }
  else
  {
    newValues[ABSX] = (pointer)0;
    newValues[ABSY] = (pointer)0;
  }

/*
 * XXX Backing store is turned off for the PS driver

  if( pWin->backStorage )
    (*pWin->drawable.pScreen->DrawGuarantee) (pWin, pGC, GuaranteeVisBack);
 */

  mask = gcmask;
  gcmask = 0;
  i = 0;
  while( mask )
  {
    index = lowbit (mask);
    mask &= ~index;
    switch(index)
    {
      case GCFunction:
        if( (pointer)(long)pGC->alu!=newValues[FUNCTION] )
        {
          gcmask |= index;
          gcval[i++] = newValues[FUNCTION];
        }
        break;
      case GCTileStipXOrigin:
        if( (pointer)(long)pGC->patOrg.x!=newValues[ABSX] )
        {
          gcmask |= index;
          gcval[i++] = newValues[ABSX];
        }
        break;
      case GCTileStipYOrigin:
        if( (pointer)(long)pGC->patOrg.y!=newValues[ABSY] )
        {
          gcmask |= index;
          gcval[i++] = newValues[ABSY];
        }
        break;
      case GCClipMask:
        if( (pointer)(long)pGC->clientClipType!=(pointer)CT_NONE )
        {
          gcmask |= index;
          gcval[i++] = (pointer)CT_NONE;
        }
        break;
      case GCSubwindowMode:
        if( (pointer)(long)pGC->subWindowMode!=newValues[SUBWINDOW] )
        {
          gcmask |= index;
          gcval[i++] = newValues[SUBWINDOW];
        }
        break;
      case GCTile:
        if( pGC->tileIsPixel || (pointer)pGC->tile.pixmap!=newValues[TILE] )
        {
          gcmask |= index;
          gcval[i++] = newValues[TILE];
        }
        break;
      case GCFillStyle:
        if( (pointer)(long)pGC->fillStyle!=newValues[FILLSTYLE] )
        {
          gcmask |= index;
          gcval[i++] = newValues[FILLSTYLE];
        }
        break;
      case GCForeground:
        if( (pointer)pGC->fgPixel!=newValues[FOREGROUND] )
        {
          gcmask |= index;
          gcval[i++] = newValues[FOREGROUND];
        }
        break;
    }
  }

  if( gcmask ) DoChangeGC(pGC, gcmask, (XID *)gcval, 1);

  if( pWin->drawable.serialNumber!=pGC->serialNumber )
    ValidateGC((DrawablePtr)pWin, pGC);

  numRects = REGION_NUM_RECTS(pRegion);
  pbox = REGION_RECTS(pRegion);
  for( i=numRects ; --i >= 0 ; pbox++,prect++ )
  {
    prect->x = pbox->x1 - pWin->drawable.x;
    prect->y = pbox->y1 - pWin->drawable.y;
    prect->width = pbox->x2 - pbox->x1;
    prect->height = pbox->y2 - pbox->y1;
  }
  prect -= numRects;
  (*pGC->ops->PolyFillRect)((DrawablePtr)pWin, pGC, numRects, prect);
  DEALLOCATE_LOCAL(prect);

/*
 * XXX Backing store is turned off for the PS driver

  if( pWin->backStorage )
    (*pWin->drawable.pScreen->DrawGuarantee) (pWin, pGC, GuaranteeNothing);
 */

  if( what==PW_BORDER )
  {
    REGION_UNINIT(pScreen, &pWin->clipList);
    pWin->clipList = prgnWin;
    pWin->drawable.x = oldCorner.x;
    pWin->drawable.y = oldCorner.y;
    pWin->drawable.serialNumber = NEXT_SERIAL_NUMBER;
  }
  FreeScratchGC(pGC);
}

/*ARGSUSED*/
Bool
PsDestroyWindow(WindowPtr pWin)
{
  return TRUE;
}
