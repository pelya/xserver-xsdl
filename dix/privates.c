/* $Xorg: privates.c,v 1.4 2001/02/09 02:04:40 xorgcvs Exp $ */
/*

Copyright 1993, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/

#include "X.h"
#include "scrnintstr.h"
#include "misc.h"
#include "os.h"
#include "windowstr.h"
#include "resource.h"
#include "dixstruct.h"
#include "gcstruct.h"
#include "colormapst.h"
#include "servermd.h"
#include "site.h"

/*
 *  See the Wrappers and devPrivates section in "Definition of the
 *  Porting Layer for the X v11 Sample Server" (doc/Server/ddx.tbl.ms)
 *  for information on how to use devPrivates.
 */

/*
 *  client private machinery
 */

static int  clientPrivateCount;
int clientPrivateLen;
unsigned *clientPrivateSizes;
unsigned totalClientSize;

void
ResetClientPrivates()
{
    clientPrivateCount = 0;
    clientPrivateLen = 0;
    xfree(clientPrivateSizes);
    clientPrivateSizes = (unsigned *)NULL;
    totalClientSize = sizeof(ClientRec);

}

int
AllocateClientPrivateIndex()
{
    return clientPrivateCount++;
}

Bool
AllocateClientPrivate(index, amount)
    int index;
    unsigned amount;
{
    unsigned oldamount;

    if (index >= clientPrivateLen)
    {
	unsigned *nsizes;
	nsizes = (unsigned *)xrealloc(clientPrivateSizes,
				      (index + 1) * sizeof(unsigned));
	if (!nsizes)
	    return FALSE;
	while (clientPrivateLen <= index)
	{
	    nsizes[clientPrivateLen++] = 0;
	    totalClientSize += sizeof(DevUnion);
	}
	clientPrivateSizes = nsizes;
    }
    oldamount = clientPrivateSizes[index];
    if (amount > oldamount)
    {
	clientPrivateSizes[index] = amount;
	totalClientSize += (amount - oldamount);
    }
    return TRUE;
}

/*
 *  screen private machinery
 */

int  screenPrivateCount;

void
ResetScreenPrivates()
{
    screenPrivateCount = 0;
}

/* this can be called after some screens have been created,
 * so we have to worry about resizing existing devPrivates
 */
int
AllocateScreenPrivateIndex()
{
    int		index;
    int		i;
    ScreenPtr	pScreen;
    DevUnion	*nprivs;

    index = screenPrivateCount++;
    for (i = 0; i < screenInfo.numScreens; i++)
    {
	pScreen = screenInfo.screens[i];
	nprivs = (DevUnion *)xrealloc(pScreen->devPrivates,
				      screenPrivateCount * sizeof(DevUnion));
	if (!nprivs)
	{
	    screenPrivateCount--;
	    return -1;
	}
	pScreen->devPrivates = nprivs;
    }
    return index;
}


/*
 *  window private machinery
 */

static int  windowPrivateCount;

void
ResetWindowPrivates()
{
    windowPrivateCount = 0;
}

int
AllocateWindowPrivateIndex()
{
    return windowPrivateCount++;
}

Bool
AllocateWindowPrivate(pScreen, index, amount)
    register ScreenPtr pScreen;
    int index;
    unsigned amount;
{
    unsigned oldamount;

    if (index >= pScreen->WindowPrivateLen)
    {
	unsigned *nsizes;
	nsizes = (unsigned *)xrealloc(pScreen->WindowPrivateSizes,
				      (index + 1) * sizeof(unsigned));
	if (!nsizes)
	    return FALSE;
	while (pScreen->WindowPrivateLen <= index)
	{
	    nsizes[pScreen->WindowPrivateLen++] = 0;
	    pScreen->totalWindowSize += sizeof(DevUnion);
	}
	pScreen->WindowPrivateSizes = nsizes;
    }
    oldamount = pScreen->WindowPrivateSizes[index];
    if (amount > oldamount)
    {
	pScreen->WindowPrivateSizes[index] = amount;
	pScreen->totalWindowSize += (amount - oldamount);
    }
    return TRUE;
}


/*
 *  gc private machinery 
 */

static int  gcPrivateCount;

void
ResetGCPrivates()
{
    gcPrivateCount = 0;
}

int
AllocateGCPrivateIndex()
{
    return gcPrivateCount++;
}

Bool
AllocateGCPrivate(pScreen, index, amount)
    register ScreenPtr pScreen;
    int index;
    unsigned amount;
{
    unsigned oldamount;

    if (index >= pScreen->GCPrivateLen)
    {
	unsigned *nsizes;
	nsizes = (unsigned *)xrealloc(pScreen->GCPrivateSizes,
				      (index + 1) * sizeof(unsigned));
	if (!nsizes)
	    return FALSE;
	while (pScreen->GCPrivateLen <= index)
	{
	    nsizes[pScreen->GCPrivateLen++] = 0;
	    pScreen->totalGCSize += sizeof(DevUnion);
	}
	pScreen->GCPrivateSizes = nsizes;
    }
    oldamount = pScreen->GCPrivateSizes[index];
    if (amount > oldamount)
    {
	pScreen->GCPrivateSizes[index] = amount;
	pScreen->totalGCSize += (amount - oldamount);
    }
    return TRUE;
}


/*
 *  pixmap private machinery
 */
#ifdef PIXPRIV
static int  pixmapPrivateCount;

void
ResetPixmapPrivates()
{
    pixmapPrivateCount = 0;
}

int
AllocatePixmapPrivateIndex()
{
    return pixmapPrivateCount++;
}

Bool
AllocatePixmapPrivate(pScreen, index, amount)
    register ScreenPtr pScreen;
    int index;
    unsigned amount;
{
    unsigned oldamount;

    if (index >= pScreen->PixmapPrivateLen)
    {
	unsigned *nsizes;
	nsizes = (unsigned *)xrealloc(pScreen->PixmapPrivateSizes,
				      (index + 1) * sizeof(unsigned));
	if (!nsizes)
	    return FALSE;
	while (pScreen->PixmapPrivateLen <= index)
	{
	    nsizes[pScreen->PixmapPrivateLen++] = 0;
	    pScreen->totalPixmapSize += sizeof(DevUnion);
	}
	pScreen->PixmapPrivateSizes = nsizes;
    }
    oldamount = pScreen->PixmapPrivateSizes[index];
    if (amount > oldamount)
    {
	pScreen->PixmapPrivateSizes[index] = amount;
	pScreen->totalPixmapSize += (amount - oldamount);
    }
    return TRUE;
}
#endif


/*
 *  colormap private machinery
 */

int  colormapPrivateCount;

void
ResetColormapPrivates()
{
    colormapPrivateCount = 0;
}


int
AllocateColormapPrivateIndex (initPrivFunc)

InitCmapPrivFunc initPrivFunc;

{
    int		index;
    int		i;
    ColormapPtr	pColormap;
    DevUnion	*privs;

    index = colormapPrivateCount++;

    for (i = 0; i < screenInfo.numScreens; i++)
    {
	/*
	 * AllocateColormapPrivateIndex may be called after the
	 * default colormap has been created on each screen!
	 *
	 * We must resize the devPrivates array for the default
	 * colormap on each screen, making room for this new private.
	 * We also call the initialization function 'initPrivFunc' on
	 * the new private allocated for each default colormap.
	 */

	ScreenPtr pScreen = screenInfo.screens[i];

	pColormap = (ColormapPtr) LookupIDByType (
	    pScreen->defColormap, RT_COLORMAP);

	privs = (DevUnion *) xrealloc (pColormap->devPrivates,
	    colormapPrivateCount * sizeof(DevUnion));

	pColormap->devPrivates = privs;

	if (!privs || !(*initPrivFunc)(pColormap))
	{
	    colormapPrivateCount--;
	    return -1;
	}
    }

    return index;
}
