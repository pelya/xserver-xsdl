/*
 * Support for accelerated rootless code
 */
/*
 * Copyright (c) 2004 Torrey T. Lyons. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written authorization.
 */
/* $XdotOrg: $ */

#include "rootless.h"
#include "rlAccel.h"

typedef struct _rlAccelScreenRec {
    CreateGCProcPtr CreateGC;
} rlAccelScreenRec, *rlAccelScreenPtr;

static int rlAccelScreenPrivateIndex = -1;

#define RLACCELREC(pScreen) \
   ((rlAccelScreenRec *)(pScreen)->devPrivates[rlAccelScreenPrivateIndex].ptr)


/*
 * Screen function to create a graphics context
 */
static Bool
rlCreateGC(GCPtr pGC)
{
    ScreenPtr pScreen = pGC->pScreen;
    rlAccelScreenRec *s = RLACCELREC(pScreen);
    Bool result;

    // Unwrap and call
    pScreen->CreateGC = s->CreateGC;
    result = s->CreateGC(pGC);

    // Accelerated GC ops replace some ops
    pGC->ops->FillSpans = rlFillSpans;
    pGC->ops->CopyArea = rlCopyArea;
    pGC->ops->PolyFillRect = rlPolyFillRect;
    pGC->ops->ImageGlyphBlt = rlImageGlyphBlt;

    // Rewrap
    s->CreateGC = pScreen->CreateGC;
    pScreen->CreateGC = rlCreateGC;

    return result;
}


/*
 * RootlessAccelInit
 *  Called by the rootless implementation to initialize accelerated
 *  rootless drawing.
 */
Bool
RootlessAccelInit(ScreenPtr pScreen)
{
    static unsigned long rlAccelGeneration = 0;
    rlAccelScreenRec *s;

    if (rlAccelGeneration != serverGeneration) {
        rlAccelScreenPrivateIndex = AllocateScreenPrivateIndex();
        if (rlAccelScreenPrivateIndex == -1) return FALSE;
    }

    s = xalloc(sizeof(rlAccelScreenRec));
    if (!s) return FALSE;
    RLACCELREC(pScreen) = s;

    // Wrap the one screen function we need
    s->CreateGC = pScreen->CreateGC;
    pScreen->CreateGC = rlCreateGC;

    return TRUE;
}
