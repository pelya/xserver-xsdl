/*
 * $Xorg: cfbsolid.c,v 1.4 2001/02/09 02:04:38 xorgcvs Exp $
 *
Copyright 1990, 1998  The Open Group

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
 *
 * Author:  Keith Packard, MIT X Consortium
 */


#include "X.h"
#include "Xmd.h"
#include "servermd.h"
#include "gcstruct.h"
#include "window.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "cfb.h"
#include "cfbmskbits.h"
#include "cfbrrop.h"

#include "mi.h"
#include "mispans.h"

#if defined(FAST_CONSTANT_OFFSET_MODE) && (RROP != GXcopy)
# define Expand(left,right,leftAdjust) {\
    int part = nmiddle & 3; \
    int widthStep; \
    widthStep = widthDst - nmiddle - leftAdjust; \
    nmiddle >>= 2; \
    pdst = pdstRect; \
    while (h--) { \
	left \
	pdst += part; \
	switch (part) { \
	    RROP_UNROLL_CASE3(pdst) \
	} \
	m = nmiddle; \
	while (m) { \
	    pdst += 4; \
	    RROP_UNROLL_LOOP4(pdst,-4) \
	    m--; \
	} \
	right \
	pdst += widthStep; \
    } \
}
#else
# ifdef RROP_UNROLL
#  define Expand(left,right,leftAdjust) {\
    int part = nmiddle & RROP_UNROLL_MASK; \
    int widthStep; \
    widthStep = widthDst - nmiddle - leftAdjust; \
    nmiddle >>= RROP_UNROLL_SHIFT; \
    pdst = pdstRect; \
    while (h--) { \
	left \
	pdst += part; \
	switch (part) { \
	    RROP_UNROLL_CASE(pdst) \
	} \
	m = nmiddle; \
	while (m) { \
	    pdst += RROP_UNROLL; \
	    RROP_UNROLL_LOOP(pdst) \
	    m--; \
	} \
	right \
	pdst += widthStep; \
    } \
}

# else
#  define Expand(left, right, leftAdjust) { \
    while (h--) { \
	pdst = pdstRect; \
	left \
	m = nmiddle; \
	while (m--) {\
	    RROP_SOLID(pdst); \
	    pdst++; \
	} \
	right \
	pdstRect += widthDst; \
    } \
}
# endif
#endif
	

void
RROP_NAME(cfbFillRectSolid) (pDrawable, pGC, nBox, pBox)
    DrawablePtr	    pDrawable;
    GCPtr	    pGC;
    int		    nBox;
    BoxPtr	    pBox;
{
    register int    m;
    register unsigned long   *pdst;
    RROP_DECLARE
    register unsigned long   leftMask, rightMask;
    unsigned long   *pdstBase, *pdstRect;
    int		    nmiddle;
    int		    h;
    int		    w;
    int		    widthDst;
    cfbPrivGCPtr    devPriv;

    devPriv = cfbGetGCPrivate(pGC);

    cfbGetLongWidthAndPointer (pDrawable, widthDst, pdstBase)

    RROP_FETCH_GC(pGC)
    
    for (; nBox; nBox--, pBox++)
    {
    	pdstRect = pdstBase + pBox->y1 * widthDst;
    	h = pBox->y2 - pBox->y1;
	w = pBox->x2 - pBox->x1;
#if PSZ == 8
	if (w == 1)
	{
	    register char    *pdstb = ((char *) pdstRect) + pBox->x1;
	    int	    incr = widthDst * PGSZB;

	    while (h--)
	    {
		RROP_SOLID (pdstb);
		pdstb += incr;
	    }
	}
	else
	{
#endif
	pdstRect += (pBox->x1 >> PWSH);
	if ((pBox->x1 & PIM) + w <= PPW)
	{
	    maskpartialbits(pBox->x1, w, leftMask);
	    pdst = pdstRect;
	    while (h--) {
		RROP_SOLID_MASK (pdst, leftMask);
		pdst += widthDst;
	    }
	}
	else
	{
	    maskbits (pBox->x1, w, leftMask, rightMask, nmiddle);
	    if (leftMask)
	    {
		if (rightMask)	/* left mask and right mask */
		{
		    Expand(RROP_SOLID_MASK (pdst, leftMask); pdst++;,
			   RROP_SOLID_MASK (pdst, rightMask);, 1)
		}
		else	/* left mask and no right mask */
		{
		    Expand(RROP_SOLID_MASK (pdst, leftMask); pdst++;,
			   ;, 1)
		}
	    }
	    else
	    {
		if (rightMask)	/* no left mask and right mask */
		{
		    Expand(;,
			   RROP_SOLID_MASK (pdst, rightMask);, 0)
		}
		else	/* no left mask and no right mask */
		{
		    Expand(;,
			    ;, 0)
		}
	    }
	}
#if PSZ == 8
	}
#endif
    }
}

void
RROP_NAME(cfbSolidSpans) (pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int		nInit;			/* number of spans to fill */
    DDXPointPtr pptInit;		/* pointer to list of start points */
    int		*pwidthInit;		/* pointer to list of n widths */
    int 	fSorted;
{
    unsigned long   *pdstBase;
    int		    widthDst;

    RROP_DECLARE
    
    register unsigned long  *pdst;
    register int	    nlmiddle;
    register unsigned long  startmask, endmask;
    register int	    w;
    int			    x;
    
				/* next three parameters are post-clip */
    int		    n;		/* number of spans to fill */
    DDXPointPtr	    ppt;	/* pointer to list of start points */
    int		    *pwidthFree;/* copies of the pointers to free */
    DDXPointPtr	    pptFree;
    int		    *pwidth;
    cfbPrivGCPtr    devPriv;

    devPriv = cfbGetGCPrivate(pGC);
    RROP_FETCH_GCPRIV(devPriv)
    n = nInit * miFindMaxBand(devPriv->pCompositeClip);
    pwidthFree = (int *)ALLOCATE_LOCAL(n * sizeof(int));
    pptFree = (DDXPointRec *)ALLOCATE_LOCAL(n * sizeof(DDXPointRec));
    if(!pptFree || !pwidthFree)
    {
	if (pptFree) DEALLOCATE_LOCAL(pptFree);
	if (pwidthFree) DEALLOCATE_LOCAL(pwidthFree);
	return;
    }
    pwidth = pwidthFree;
    ppt = pptFree;
    n = miClipSpans(devPriv->pCompositeClip,
		     pptInit, pwidthInit, nInit,
		     ppt, pwidth, fSorted);

    cfbGetLongWidthAndPointer (pDrawable, widthDst, pdstBase)

    while (n--)
    {
	x = ppt->x;
	pdst = pdstBase + (ppt->y * widthDst);
	++ppt;
	w = *pwidth++;
	if (!w)
	    continue;
#if PSZ == 8
	if (w <= PGSZB)
	{
	    register char   *addrb;

	    addrb = ((char *) pdst) + x;
	    while (w--)
	    {
		RROP_SOLID (addrb);
		addrb++;
	    }
	}
#else
	if ((x & PIM) + w <= PPW)
	{
	    pdst += x >> PWSH;
	    maskpartialbits (x, w, startmask);
	    RROP_SOLID_MASK (pdst, startmask);
	}
#endif
	else
	{
	    pdst += x >> PWSH;
	    maskbits (x, w, startmask, endmask, nlmiddle);
	    if (startmask)
	    {
		RROP_SOLID_MASK (pdst, startmask);
		++pdst;
	    }
	    
	    RROP_SPAN(pdst,nlmiddle)
	    if (endmask)
	    {
		RROP_SOLID_MASK (pdst, endmask);
	    }
	}
    }
    DEALLOCATE_LOCAL(pptFree);
    DEALLOCATE_LOCAL(pwidthFree);
}
