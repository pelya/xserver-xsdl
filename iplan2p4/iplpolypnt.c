/* $XFree86: xc/programs/Xserver/iplan2p4/iplpolypnt.c,v 3.1 1998/03/20 21:08:09 hohndel Exp $ */
/************************************************************

Copyright (c) 1989  X Consortium

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

********************************************************/

/* $XConsortium: iplpolypnt.c,v 5.17 94/04/17 20:28:57 dpw Exp $ */

/* Modified nov 94 by Martin Schaller (Martin_Schaller@maus.r.de) for use with
interleaved planes */

#include "X.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "regionstr.h"
#include "scrnintstr.h"
#include "ipl.h"
#include "iplmskbits.h"

#define isClipped(c,ul,lr)  ((((c) - (ul)) | ((lr) - (c))) & ClipMask)

/* WARNING: pbox contains two shorts. This code assumes they are packed
 * and can be referenced together as an INT32.
 */

#define PointLoop(fill) { \
    for (nbox = REGION_NUM_RECTS(cclip), pbox = REGION_RECTS(cclip); \
	 --nbox >= 0; \
	 pbox++) \
    { \
	c1 = *((INT32 *) &pbox->x1) - off; \
	c2 = *((INT32 *) &pbox->x2) - off - 0x00010001; \
	for (ppt = (INT32 *) pptInit, i = npt; --i >= 0;) \
	{ \
	    pt = *ppt++; \
	    if (!isClipped(pt,c1,c2)) { \
		fill \
	    } \
	} \
    } \
}

void
iplPolyPoint(pDrawable, pGC, mode, npt, pptInit)
    DrawablePtr pDrawable;
    GCPtr pGC;
    int mode;
    int npt;
    xPoint *pptInit;
{
    register INT32   pt;
    register INT32   c1, c2;
    register unsigned long   ClipMask = 0x80008000;
    INTER_DECLAREG(*addrg);
    register int    ngwidth;
    register int    xoffset;
    INTER_DECLAREG(*addrgt);
    register INT32  *ppt;
    RegionPtr	    cclip;
    int		    nbox;
    register int    i;
    register BoxPtr pbox;
    INTER_DECLARERRAX(and);
    INTER_DECLARERRAX(xor);
    int		    rop = pGC->alu;
    int		    off;
    iplPrivGCPtr    devPriv;
    xPoint	    *pptPrev;

    devPriv =iplGetGCPrivate(pGC);
    rop = devPriv->rop;
    if (rop == GXnoop)
	return;
    cclip = pGC->pCompositeClip;
    xor = devPriv->xorg;
    and = devPriv->andg;
    if ((mode == CoordModePrevious) && (npt > 1))
    {
	for (pptPrev = pptInit + 1, i = npt - 1; --i >= 0; pptPrev++)
	{
	    pptPrev->x += (pptPrev-1)->x;
	    pptPrev->y += (pptPrev-1)->y;
	}
    }
    off = *((int *) &pDrawable->x);
    off -= (off & 0x8000) << 1;
    iplGetGroupWidthAndPointer(pDrawable, ngwidth, addrg);
    addrg = addrg + pDrawable->y * ngwidth + 
		(pDrawable->x >> INTER_PGSH) * INTER_PLANES;
    xoffset = pDrawable->x & INTER_PIM;
    PointLoop(   addrgt = addrg + intToY(pt) * ngwidth
 	               + ((intToX(pt) + xoffset) >> INTER_PGSH) * INTER_PLANES;
		   INTER_DoMaskRRop(addrgt, and, xor, 
			iplmask[(intToX(pt) + xoffset) & INTER_PIM], addrgt);
	     )
}
