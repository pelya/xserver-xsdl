/* COPYRIGHT AND PERMISSION NOTICE

Copyright (c) 2000, 2001 Nokia Home Communications

All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, and/or sell copies of the Software, and to permit persons
to whom the Software is furnished to do so, provided that the above
copyright notice(s) and this permission notice appear in all copies of
the Software and that both the above copyright notice(s) and this
permission notice appear in supporting documentation.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
HOLDERS INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY
SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER
RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

Except as contained in this notice, the name of a copyright holder
shall not be used in advertising or otherwise to promote the sale, use
or other dealings in this Software without prior written authorization
of the copyright holder.

X Window System is a trademark of The Open Group */

/* Hardware accelerated drawing for KDrive i810 driver.
   Author: Pontus Lidman <pontus.lidman@nokia.com>
*/

#include "kdrive.h"
#ifdef XV
#include "kxv.h"
#endif
#include "i810.h"
#include "i810_reg.h"

#include	"Xmd.h"
#include	"gcstruct.h"
#include	"scrnintstr.h"
#include	"pixmapstr.h"
#include	"regionstr.h"
#include	"mistruct.h"
#include	"fontstruct.h"
#include	"dixfontstr.h"
#include	"fb.h"
#include	"migc.h"
#include	"miline.h"
#include	"picturestr.h"

#define NUM_STACK_RECTS	1024

void
i810Sync( KdScreenInfo *screen );
int
i810WaitLpRing( KdScreenInfo *screen, int n, int timeout_millis );

void 
i810EmitInvarientState(KdScreenInfo *screen)
{
    KdCardInfo	    *card = screen->card;
    I810CardInfo    *i810c = (I810CardInfo *) card->driver;

    BEGIN_LP_RING( 10 );

    OUT_RING( INST_PARSER_CLIENT | INST_OP_FLUSH | INST_FLUSH_MAP_CACHE );
    OUT_RING( GFX_CMD_CONTEXT_SEL | CS_UPDATE_USE | CS_USE_CTX0 );
    OUT_RING( INST_PARSER_CLIENT | INST_OP_FLUSH | INST_FLUSH_MAP_CACHE);
    OUT_RING( 0 );


    OUT_RING( GFX_OP_COLOR_CHROMA_KEY );
    OUT_RING( CC1_UPDATE_KILL_WRITE | 
              CC1_DISABLE_KILL_WRITE | 
              CC1_UPDATE_COLOR_IDX |
              CC1_UPDATE_CHROMA_LOW |
              CC1_UPDATE_CHROMA_HI |
              0);
    OUT_RING( 0 );
    OUT_RING( 0 );

    /* No depth buffer in KDrive yet */
    /*     OUT_RING( CMD_OP_Z_BUFFER_INFO ); */
    /*     OUT_RING( pI810->DepthBuffer.Start | pI810->auxPitchBits); */

    ADVANCE_LP_RING();      
}

static unsigned int i810PatternRop[16] = {
    0x00, /* GXclear      */
    0xA0, /* GXand        */
    0x50, /* GXandReverse */
    0xF0, /* GXcopy       */
    0x0A, /* GXandInvert  */
    0xAA, /* GXnoop       */
    0x5A, /* GXxor        */
    0xFA, /* GXor         */
    0x05, /* GXnor        */
    0xA5, /* GXequiv      */
    0x55, /* GXinvert     */
    0xF5, /* GXorReverse  */
    0x0F, /* GXcopyInvert */
    0xAF, /* GXorInverted */
    0x5F, /* GXnand       */
    0xFF  /* GXset        */
};

void 
i810SetupForSolidFill(KdScreenInfo *screen, int color, int rop,
		      unsigned int planemask)
{
    KdCardInfo	    *card = screen->card;
    I810CardInfo    *i810c = (I810CardInfo *) card->driver;

   if (I810_DEBUG & DEBUG_VERBOSE_ACCEL)
      ErrorF( "i810SetupForFillRectSolid color: %x rop: %x mask: %x\n", 
	      color, rop, planemask);

   /* Color blit, p166 */
   i810c->BR[13] = (BR13_SOLID_PATTERN | 
		    (i810PatternRop[rop] << 16) |
		    (screen->width * i810c->cpp));
   i810c->BR[16] = color;
}


void 
i810SubsequentSolidFillRect(KdScreenInfo *screen, int x, int y, int w, int h)
{
    KdCardInfo	    *card = screen->card;
    I810CardInfo    *i810c = (I810CardInfo *) card->driver;

   if (I810_DEBUG & DEBUG_VERBOSE_ACCEL)
      ErrorF( "i810SubsequentFillRectSolid %d,%d %dx%d\n",
	      x,y,w,h);

   {   
      BEGIN_LP_RING(6);
   
      OUT_RING( BR00_BITBLT_CLIENT | BR00_OP_COLOR_BLT | 0x3 );
      OUT_RING( i810c->BR[13] );
      OUT_RING( (h << 16) | (w * i810c->cpp));
      OUT_RING( i810c->bufferOffset + 
		(y * screen->width + x) * i810c->cpp);

      OUT_RING( i810c->BR[16]);
      OUT_RING( 0 );		/* pad to quadword */

      ADVANCE_LP_RING();
   }
}


BOOL
i810FillOk (GCPtr pGC)
{
    FbBits  depthMask;

    switch (pGC->fillStyle) {
    case FillSolid:
	return TRUE;
        /* More cases later... */
    }
    return FALSE;
}

void
i810FillBoxSolid (KdScreenInfo *screen, int nBox, BoxPtr pBox, 
                  unsigned long pixel, int alu, unsigned long planemask)
{
    i810SetupForSolidFill(screen, pixel, alu, planemask);
    while (nBox--) 
    {
        i810SubsequentSolidFillRect(screen, pBox->x1, pBox->y1, 
                                    pBox->x2-pBox->x1, pBox->y2-pBox->y1);
	pBox++;
    }
    KdMarkSync(screen->pScreen);
}


void
i810PolyFillRect (DrawablePtr pDrawable, GCPtr pGC, 
		int nrectFill, xRectangle *prectInit)
{


    xRectangle	    *prect;
    RegionPtr	    prgnClip;
    register BoxPtr pbox;
    register BoxPtr pboxClipped;
    BoxPtr	    pboxClippedBase;
    BoxPtr	    pextent;
    BoxRec	    stackRects[NUM_STACK_RECTS];
    FbGCPrivPtr	    fbPriv = fbGetGCPrivate (pGC);
    int		    numRects;
    int		    n;
    int		    xorg, yorg;
    int		    x, y;
    KdScreenPriv(pDrawable->pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    
    if (!i810FillOk (pGC))
    {
	KdCheckPolyFillRect (pDrawable, pGC, nrectFill, prectInit);
	return;
    }
    prgnClip = fbGetCompositeClip(pGC);
    xorg = pDrawable->x;
    yorg = pDrawable->y;
    
    if (xorg || yorg)
    {
	prect = prectInit;
	n = nrectFill;
	while(n--)
	{
	    prect->x += xorg;
	    prect->y += yorg;
	    prect++;
	}
    }
    
    prect = prectInit;

    numRects = REGION_NUM_RECTS(prgnClip) * nrectFill;
    if (numRects > NUM_STACK_RECTS)
    {
	pboxClippedBase = (BoxPtr)xalloc(numRects * sizeof(BoxRec));
	if (!pboxClippedBase)
	    return;
    }
    else
	pboxClippedBase = stackRects;

    pboxClipped = pboxClippedBase;
	
    if (REGION_NUM_RECTS(prgnClip) == 1)
    {
	int x1, y1, x2, y2, bx2, by2;

	pextent = REGION_RECTS(prgnClip);
	x1 = pextent->x1;
	y1 = pextent->y1;
	x2 = pextent->x2;
	y2 = pextent->y2;
    	while (nrectFill--)
    	{
	    if ((pboxClipped->x1 = prect->x) < x1)
		pboxClipped->x1 = x1;
    
	    if ((pboxClipped->y1 = prect->y) < y1)
		pboxClipped->y1 = y1;
    
	    bx2 = (int) prect->x + (int) prect->width;
	    if (bx2 > x2)
		bx2 = x2;
	    pboxClipped->x2 = bx2;
    
	    by2 = (int) prect->y + (int) prect->height;
	    if (by2 > y2)
		by2 = y2;
	    pboxClipped->y2 = by2;

	    prect++;
	    if ((pboxClipped->x1 < pboxClipped->x2) &&
		(pboxClipped->y1 < pboxClipped->y2))
	    {
		pboxClipped++;
	    }
    	}
    }
    else
    {
	int x1, y1, x2, y2, bx2, by2;

	pextent = REGION_EXTENTS(pGC->pScreen, prgnClip);
	x1 = pextent->x1;
	y1 = pextent->y1;
	x2 = pextent->x2;
	y2 = pextent->y2;
    	while (nrectFill--)
    	{
	    BoxRec box;
    
	    if ((box.x1 = prect->x) < x1)
		box.x1 = x1;
    
	    if ((box.y1 = prect->y) < y1)
		box.y1 = y1;
    
	    bx2 = (int) prect->x + (int) prect->width;
	    if (bx2 > x2)
		bx2 = x2;
	    box.x2 = bx2;
    
	    by2 = (int) prect->y + (int) prect->height;
	    if (by2 > y2)
		by2 = y2;
	    box.y2 = by2;
    
	    prect++;
    
	    if ((box.x1 >= box.x2) || (box.y1 >= box.y2))
	    	continue;
    
	    n = REGION_NUM_RECTS (prgnClip);
	    pbox = REGION_RECTS(prgnClip);
    
	    /* clip the rectangle to each box in the clip region
	       this is logically equivalent to calling Intersect()
	    */
	    while(n--)
	    {
		pboxClipped->x1 = max(box.x1, pbox->x1);
		pboxClipped->y1 = max(box.y1, pbox->y1);
		pboxClipped->x2 = min(box.x2, pbox->x2);
		pboxClipped->y2 = min(box.y2, pbox->y2);
		pbox++;

		/* see if clipping left anything */
		if(pboxClipped->x1 < pboxClipped->x2 && 
		   pboxClipped->y1 < pboxClipped->y2)
		{
		    pboxClipped++;
		}
	    }
    	}
    }
    if (pboxClipped != pboxClippedBase)
    {
	switch (pGC->fillStyle) {
	case FillSolid:
	    i810FillBoxSolid(screen,
                             pboxClipped-pboxClippedBase, pboxClippedBase,
                             pGC->fgPixel, pGC->alu, pGC->planemask);
	    break;
            /* More cases later... */
	}
    }
    if (pboxClippedBase != stackRects)
    	xfree(pboxClippedBase);
}

void 
i810RefreshRing(KdScreenInfo *screen)
{
    KdCardInfo	    *card = screen->card;
    I810CardInfo    *i810c = (I810CardInfo *) card->driver;
      
   i810c->LpRing.head = INREG(LP_RING + RING_HEAD) & HEAD_ADDR;
   i810c->LpRing.tail = INREG(LP_RING + RING_TAIL);
   i810c->LpRing.space = i810c->LpRing.head - (i810c->LpRing.tail+8);
   if (i810c->LpRing.space < 0) 
      i810c->LpRing.space += i810c->LpRing.mem.Size;

   i810c->NeedToSync = TRUE;
}

int
i810WaitLpRing( KdScreenInfo *screen, int n, int timeout_millis )
{
    KdCardInfo	    *card = screen->card;
    I810CardInfo    *i810c = (I810CardInfo *) card->driver;
    I810RingBuffer *ring = &(i810c->LpRing);
    int iters = 0;
    int start = 0;
    int now = 0;
    int last_head = 0;
    int first = 0;
    
    /* If your system hasn't moved the head pointer in 2 seconds, I'm going to
    * call it crashed.
    */
   if (timeout_millis == 0)
      timeout_millis = 2000;

   if (I810_DEBUG) {
      fprintf(stderr, "i810WaitLpRing %d\n", n); 
      first = GetTimeInMillis();
   }

   while (ring->space < n) 
   {
      int i;

      ring->head = INREG(LP_RING + RING_HEAD) & HEAD_ADDR;
      ring->space = ring->head - (ring->tail+8);

      if (ring->space < 0) 
	 ring->space += ring->mem.Size;
      
      iters++;
      now = GetTimeInMillis();
      if ( start == 0 || now < start || ring->head != last_head) {
	 if (I810_DEBUG)
	    if (now > start) 
	       fprintf(stderr, "space: %d wanted %d\n", ring->space, n ); 
	 start = now;
	 last_head = ring->head;
      } else if ( now - start > timeout_millis ) { 

	 i810PrintErrorState( screen->card ); 
	 fprintf(stderr, "space: %d wanted %d\n", ring->space, n );
	 FatalError("lockup\n"); 
      }

      for (i = 0 ; i < 2000 ; i++)
	 ;
   }

   if (I810_DEBUG)
   {
      now = GetTimeInMillis();
      if (now - first) {
	 fprintf(stderr,"Elapsed %d ms\n", now - first);
	 fprintf(stderr, "space: %d wanted %d\n", ring->space, n );
      }
   }

   return iters;
}

void
i810Sync( KdScreenInfo *screen ) 
{
    KdCardInfo	    *card = screen->card;
    I810CardInfo    *i810c = card->driver;

   if (I810_DEBUG)
      fprintf(stderr, "i810Sync\n");
   
   /* Send a flush instruction and then wait till the ring is empty.
    * This is stronger than waiting for the blitter to finish as it also
    * flushes the internal graphics caches.
    */
   {
       BEGIN_LP_RING(2);   
       OUT_RING( INST_PARSER_CLIENT | INST_OP_FLUSH | INST_FLUSH_MAP_CACHE );
       OUT_RING( 0 );		/* pad to quadword */
       ADVANCE_LP_RING();
   }

   i810WaitLpRing(screen, i810c->LpRing.mem.Size - 8, 0 );	

   i810c->LpRing.space = i810c->LpRing.mem.Size - 8;			
   i810c->nextColorExpandBuf = 0;
}

static const GCOps	i810Ops = {
    KdCheckFillSpans,
    KdCheckSetSpans,
    KdCheckPutImage,
    KdCheckCopyArea,
    KdCheckCopyPlane,
    KdCheckPolyPoint,
    KdCheckPolylines,
    KdCheckPolySegment,
    miPolyRectangle,
    KdCheckPolyArc,
    miFillPolygon,
    i810PolyFillRect,
    miPolyFillArc,
    miPolyText8,
    miPolyText16,
    miImageText8,
    miImageText16,
    KdCheckImageGlyphBlt,
    KdCheckPolyGlyphBlt,
    KdCheckPushPixels,
#ifdef NEED_LINEHELPER
    ,NULL
#endif
};

void
i810ValidateGC (GCPtr pGC, Mask changes, DrawablePtr pDrawable)
{
    FbGCPrivPtr fbPriv = fbGetGCPrivate(pGC);
    
    fbValidateGC (pGC, changes, pDrawable);
    
    if (pDrawable->type == DRAWABLE_WINDOW)
	pGC->ops = (GCOps *) &i810Ops;
    else
	pGC->ops = (GCOps *) &kdAsyncPixmapGCOps;
}

GCFuncs	i810GCFuncs = {
    i810ValidateGC,
    miChangeGC,
    miCopyGC,
    miDestroyGC,
    miChangeClip,
    miDestroyClip,
    miCopyClip
};

int
i810CreateGC (GCPtr pGC)
{
    if (!fbCreateGC (pGC))
	return FALSE;

    if (pGC->depth != 1)
	pGC->funcs = &i810GCFuncs;
    
    return TRUE;
}

static void
i810SetRingRegs( KdScreenInfo *screen ) {
   unsigned int itemp;

   KdCardInfo	    *card = screen->card;
   I810CardInfo    *i810c = (I810CardInfo *) card->driver;

   OUTREG(LP_RING + RING_TAIL, 0 );
   OUTREG(LP_RING + RING_HEAD, 0 );

   itemp = INREG(LP_RING + RING_START);
   itemp &= ~(START_ADDR);
   itemp |= i810c->LpRing.mem.Start;
   OUTREG(LP_RING + RING_START, itemp );

   itemp = INREG(LP_RING + RING_LEN);
   itemp &= ~(RING_NR_PAGES | RING_REPORT_MASK | RING_VALID_MASK);
   itemp |= ((i810c->LpRing.mem.Size-4096) | RING_NO_REPORT | RING_VALID);
   OUTREG(LP_RING + RING_LEN, itemp );
}

Bool
i810InitAccel(ScreenPtr pScreen)
{

/*     fprintf(stderr,"i810InitAccel\n"); */

    /*
     * Hook up asynchronous drawing
     */
    KdScreenInitAsync (pScreen);
    /*
     * Replace various fb screen functions
     */
    pScreen->CreateGC = i810CreateGC;

    return TRUE;
}

void
i810EnableAccel(ScreenPtr pScreen)
{

    KdScreenPriv(pScreen);
    KdScreenInfo    *screen = pScreenPriv->screen;
    KdCardInfo	    *card = screen->card;
    I810CardInfo    *i810c = (I810CardInfo *) card->driver;

/*     fprintf(stderr,"i810EnableAccel\n"); */

    if (i810c->LpRing.mem.Size == 0) {
        ErrorF("No memory for LpRing!! Acceleration not functional!!\n");
    }

    i810SetRingRegs( screen );

    KdMarkSync (pScreen);
}


void
i810SyncAccel(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo    *screen = pScreenPriv->screen;

    i810Sync(screen);
}

void
i810DisableAccel(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo    *screen = pScreenPriv->screen;
    
/*     fprintf(stderr,"i810DisableAccel\n"); */
    i810RefreshRing( screen );
    i810Sync( screen );
}

void
i810FiniAccel(ScreenPtr pScreen)
{
/*     fprintf(stderr,"i810FiniAccel\n"); */

}
