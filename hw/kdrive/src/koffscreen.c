/*
 * $Id$
 *
 * Copyright © 2003 Anders Carlsson
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Anders Carlsson not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Anders Carlsson makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * ANDERS CARLSSON DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ANDERS CARLSSON BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
/* $Header$ */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "kdrive.h"

static unsigned long offGeneration = 0;
static int offScreenPrivateIndex;
static int offPixmapPrivateIndex;

typedef struct
{
    CARD8 *base;
    int size;

    CreatePixmapProcPtr CreatePixmap;

    DestroyPixmapProcPtr DestroyPixmap;
} OffScreenPrivRec, *OffScreenPrivPtr;

typedef struct
{
    int score;
} OffPixmapPrivRec, *OffPixmapPrivPtr;

#define offPixmapPtr(p) OffPixmapPrivPtr pOffPixmap = (OffPixmapPrivPtr) (p)->devPrivates[offPixmapPrivateIndex].ptr
#define offScreenPriv(s) OffScreenPrivPtr pOffScr = (OffScreenPrivPtr) (s)->devPrivates[offScreenPrivateIndex].ptr

static PixmapPtr
KdOffscreenCreatePixmap (ScreenPtr pScreen, int w, int h, int depth)
{
    offScreenPriv (pScreen);
    
    fprintf (stderr, "creating pixmap: %dx%d with depth %d\n", w, h, depth);

    return (* pOffScr->CreatePixmap) (pScreen, w, h, depth);
}

static Bool
KdOffscreenDestroyPixmap (PixmapPtr pixmap)
{
    offScreenPriv (pixmap->drawable.pScreen);

    return (* pOffScr->DestroyPixmap) (pixmap);
}

Bool
KdOffscreenInit (ScreenPtr pScreen, CARD8 *off_screen_memory, int off_screen_size)
{
    OffScreenPrivPtr pOffScr;

    fprintf (stderr, "Initializing offscreen manager, size is: %d\n", off_screen_size);
    
    if (offGeneration != serverGeneration)
    {
	offScreenPrivateIndex = AllocateScreenPrivateIndex ();
	
	offGeneration = serverGeneration;
    }

    AllocatePixmapPrivate (pScreen, offPixmapPrivateIndex, sizeof (OffPixmapPrivRec));
    
    pOffScr = xalloc (sizeof (OffScreenPrivRec));
    pScreen->devPrivates[offScreenPrivateIndex].ptr = pOffScr;

    pOffScr->base = off_screen_memory;
    pOffScr->size = off_screen_size;

    /* Override functions */
    pOffScr->CreatePixmap = pScreen->CreatePixmap;
    pScreen->CreatePixmap = KdOffscreenCreatePixmap;
    pOffScr->DestroyPixmap = pScreen->DestroyPixmap;
    pScreen->DestroyPixmap = KdOffscreenDestroyPixmap;
    
    return TRUE;
}
