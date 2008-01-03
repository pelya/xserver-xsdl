/*
 * Copyright 1999 SuSE, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of SuSE not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  SuSE makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * SuSE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL SuSE
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Keith Packard, SuSE, Inc.
 */

#ifndef _S3DRAW_H_
#define _S3DRAW_H_

extern DevPrivateKey s3GCPrivateKey;
extern DevPrivateKey s3WindowPrivateKey;

typedef struct _s3Pattern {
    S3PatternCache    	*cache;
    int			id;
    PixmapPtr		pPixmap;
    int			fillStyle;
    int			xrot, yrot;
    unsigned int	fore, back;
} s3PatternRec, *s3PatternPtr;

typedef struct _s3PrivGC {
    int    	    type;	    /* type of drawable validated against */
    int		    ma;		    /* stream descriptor */
    s3PatternPtr    pPattern;	    /* pattern */
} s3PrivGCRec, *s3PrivGCPtr;

#define s3GetGCPrivate(g) ((s3PrivGCPtr) \
    dixLookupPrivate(&(g)->devPrivates, s3GCPrivateKey))

#define s3GCPrivate(g)    s3PrivGCPtr s3Priv = s3GetGCPrivate(g)

#define s3GetWindowPrivate(w) ((s3PatternPtr) \
    dixLookupPrivate(&(w)->devPrivates, s3WindowPrivateKey))

#define s3SetWindowPrivate(w,p) \
    dixSetPrivate(&(w)->devPrivates, s3WindowPrivateKey, p)

void	_s3LoadPattern (ScreenPtr pScreen, int fb, s3PatternPtr pPattern);

#define SetupS3(s)  KdScreenPriv(s); \
		    s3CardInfo(pScreenPriv); \
		    S3Ptr s3 = s3c->s3

#ifdef S3_SYNC_DEBUG
#define SYNC_DEBUG()	fprintf (stderr, "Sync at %s:%d\n", __FILE__,__LINE__)
#else
#define SYNC_DEBUG()
#endif

#define S3_ASYNC
#ifdef S3_ASYNC
#define CheckSyncS3(s)		KdCheckSync(s)
#define MarkSyncS3(s)		KdMarkSync(s)
#define RegisterSync(screen)	KdScreenInitAsync (screen)
#else
#define CheckSyncS3(s3c)
#define MarkSyncS3(s3c)		_s3WaitIdleEmpty(s3c->s3)
#define RegisterSync(screen)	
#endif

#define WIDEN(x)    ((unsigned long) (x))
#define MERGE(a,b)  ((WIDEN(a) << 16) | WIDEN(b))

#define s3BitmapDescriptor(_stream) ((_stream) + 1)

#ifdef S3_TRIO
#define s3DrawMap(pDraw)    0
#define s3SetGlobalBitmap(s,d)
#else
#define s3DrawMap(pDraw)    ((pDraw)->depth == \
			     getS3ScreenInfo(pScreenPriv)->primary_depth ? 0 : 1)
#endif

#define s3GCMap(pGC)	    (s3GetGCPrivate(pGC)->ma)
			     
/*
 * Ok, so the S3 is broken -- it expects bitmaps to come MSB bit order,
 * but it's willing to take them in LSB byte order.  These macros
 * flip bits around without flipping bytes.  Instead of using a table
 * and burning memory bandwidth, do them in place with the CPU.
 */

/* The MIPS compiler automatically places these constants in registers */
#define S3InvertBits32(v) { \
    v = ((v & 0x55555555) << 1) | ((v >> 1) & 0x55555555); \
    v = ((v & 0x33333333) << 2) | ((v >> 2) & 0x33333333); \
    v = ((v & 0x0f0f0f0f) << 4) | ((v >> 4) & 0x0f0f0f0f); \
}

#define S3InvertBits16(v) { \
    v = ((v & 0x5555) << 1) | ((v >> 1) & 0x5555); \
    v = ((v & 0x3333) << 2) | ((v >> 2) & 0x3333); \
    v = ((v & 0x0f0f) << 4) | ((v >> 4) & 0x0f0f); \
}

#define S3InvertBits8(v) { \
    v = ((v & 0x55) << 1) | ((v >> 1) & 0x55); \
    v = ((v & 0x33) << 2) | ((v >> 2) & 0x33); \
    v = ((v & 0x0f) << 4) | ((v >> 4) & 0x0f); \
}

#define S3ByteSwap32(x)		((x) = (((x) >> 24) | \
					(((x) >> 8) & 0xff00) | \
					(((x) << 8) & 0xff0000) | \
					((x) << 24)))

#define S3ByteSwap16(x)		((x) = ((x) << 8) | ((x) >> 8))

#if BITMAP_BIT_ORDER == LSBFirst
#define S3AdjustBits32(x)	S3InvertBits32(x)
#define S3AdjustBits16(x)	S3InvertBits16(x)
#else
#define S3AdjustBits32(x)	S3ByteSwap32(x)
#define S3AdjustBits16(x)	S3ByteSwap16(x)
#endif

#define _s3WaitSlot(s3)		_s3WaitSlots(s3,1)

#define _s3SetFg(s3,_fg) { \
    DRAW_DEBUG ((DEBUG_REGISTERS, " fg <- 0x%x", _fg));\
    s3->fg = (_fg); \
}

#define _s3SetBg(s3,_bg) { \
    DRAW_DEBUG ((DEBUG_REGISTERS, " bg <- 0x%x", _bg));\
    s3->bg = (_bg); \
}

#define _s3SetWriteMask(s3,_mask) {\
    DRAW_DEBUG((DEBUG_REGISTERS," write_mask <- 0x%x", _mask)); \
    s3->write_mask = (_mask); \
}

#define _s3SetReadMask(s3,_mask) {\
    DRAW_DEBUG((DEBUG_REGISTERS," read_mask <- 0x%x", _mask)); \
    s3->read_mask = (_mask); \
}

#define _s3SetPixelControl(s3,_ctl) { \
    DRAW_DEBUG((DEBUG_REGISTERS, " pix_cntl <- 0x%x", PIX_CNTL | (_ctl))); \
    s3->pix_cntl_mult_misc2 = MERGE (CONTROL_MISC2, PIX_CNTL | (_ctl)); \
}

#define _s3SetFgMix(s3,_mix) { \
    DRAW_DEBUG((DEBUG_REGISTERS, " fg_mix <- 0x%x", _mix)); \
    s3->enh_fg_mix = (_mix); \
}
	       
#define _s3SetBgMix(s3,_mix) { \
    DRAW_DEBUG((DEBUG_REGISTERS, " bg_mix <- 0x%x", _mix)); \
    s3->enh_bg_mix = (_mix); \
}
	       
#define _s3SetMix(s3,fg_mix,bg_mix) { \
    DRAW_DEBUG((DEBUG_REGISTERS, " alt_mix <- 0x%x", MERGE(fg_mix,bg_mix))); \
    s3->alt_mix = MERGE(fg_mix,bg_mix); \
}

#define _s3SetCur(s3,_x,_y) { \
    DRAW_DEBUG ((DEBUG_REGISTERS, " alt_curxy <- 0x%x", MERGE(_x,_y))); \
    s3->alt_curxy = MERGE(_x,_y); \
}

#define _s3SetStep(s3,_x,_y) { \
    DRAW_DEBUG ((DEBUG_REGISTERS, " alt_step <- 0x%x", MERGE(_x,_y))); \
    s3->alt_step = MERGE(_x,_y); \
}
		 
#define _s3SetErr(s3,_e) { \
    DRAW_DEBUG ((DEBUG_REGISTERS, " err_term <- 0x%x", _e)); \
    s3->err_term = (_e); \
}
		 
#define _s3SetPcnt(s3,_x,_y) { \
    DRAW_DEBUG ((DEBUG_REGISTERS, " alt_pcnt <- 0x%x", MERGE(_x,_y))); \
    s3->alt_pcnt = MERGE(_x,_y); \
}
		 
#define _s3SetScissorsTl(s3,t,l) {\
    DRAW_DEBUG ((DEBUG_REGISTERS, " scissors_tl <- 0x%x", MERGE(t,l))); \
    s3->scissors_tl = MERGE(t,l); \
}

#define _s3SetScissorsBr(s3,b,r) {\
    DRAW_DEBUG ((DEBUG_REGISTERS, " scissors_br <- 0x%x", MERGE(b,r))); \
    s3->scissors_br = MERGE(b,r); \
}

#define _s3CmdWait(s3)

#define _s3SetCmd(s3,_cmd) { \
    DRAW_DEBUG((DEBUG_REGISTERS, " cmd <- 0x%x", _cmd)); \
    _s3CmdWait(s3); \
    s3->cmd_gp_stat = (_cmd); \
    /* { CARD32	__junk__; __junk__ = s3->cmd_gp_stat; } */ \
}
    
#define _s3SetSolidFill(s3,pix,alu,mask) { \
    DRAW_DEBUG((DEBUG_SET,"set fill 0x%x %d 0x%x",pix,alu,mask)); \
    _s3WaitSlots(s3,4); \
    _s3SetFg (s3, pix); \
    _s3SetWriteMask(s3,mask); \
    _s3SetMix (s3, FSS_FRGDCOL | s3alu[alu], BSS_BKGDCOL | MIX_SRC); \
    _s3SetPixelControl (s3, MIXSEL_FRGDMIX); \
    DRAW_DEBUG((DEBUG_SET,"  done")); \
}

#define _s3SolidRect(s3,x,y,w,h) {\
    DRAW_DEBUG((DEBUG_RENDER,"solid rect %d,%d %dx%d",x,y,w,h)); \
    _s3WaitSlots(s3,3); \
    _s3SetCur(s3, x, y); \
    _s3SetPcnt (s3, (w)-1, (h)-1); \
    _s3SetCmd (s3, CMD_RECT|INC_X|INC_Y|DRAW|WRTDATA); \
    DRAW_DEBUG((DEBUG_RENDER,"  done")); \
}

#define _s3SolidLine(s3,maj,min,len,cmd) { \
    DRAW_DEBUG ((DEBUG_RENDER, "solid line 0x%x 0x%x 0x%x", maj, min, cmd)); \
    _s3WaitSlots(s3,4); \
    _s3SetPcnt(s3, (len), 0); \
    _s3SetStep(s3, 2*((min) - (maj)), 2*(min)); \
    _s3SetErr(s3, 2*(min) - (maj)); \
    _s3SetCmd (s3, CMD_LINE | (cmd) | DRAW | WRTDATA); \
}

#define _s3ClipLine(s3,cmd,e1,e2,e,len) {\
    DRAW_DEBUG ((DEBUG_RENDER, "clip line 0x%x 0x%x 0x%x 0x%x 0x%x", cmd,e1,e2,e,len)); \
    _s3WaitSlots(s3, 4); \
    _s3SetPcnt (s3, (len), 0); \
    _s3SetStep (s3, e2, e1); \
    _s3SetErr (s3, e); \
    _s3SetCmd (s3, CMD_LINE | (cmd) | DRAW | WRTDATA); \
}

#define _s3SetTile(s3,alu,mask) { \
    DRAW_DEBUG ((DEBUG_SET,"set tile %d 0x%x", alu, mask)); \
    _s3WaitSlots(s3,3); \
    _s3SetWriteMask(s3, mask); \
    _s3SetMix(s3, FSS_BITBLT | s3alu[alu], BSS_BITBLT|s3alu[alu]); \
    _s3SetPixelControl (s3, MIXSEL_FRGDMIX); \
    DRAW_DEBUG((DEBUG_SET,"  done")); \
}

/*
 * For some reason, MIX_DST doesn't work in this mode; use MIX_OR with
 * an explicit 0 pixel value
 */
#define _s3SetStipple(s3,alu,mask,_fg) {\
    DRAW_DEBUG ((DEBUG_SET,"set stipple 0x%x %d 0x%x", _fg, alu, mask)); \
    _s3WaitSlots(s3,5); \
    _s3SetFg (s3, _fg); \
    _s3SetBg (s3, 0); \
    _s3SetWriteMask(s3,mask); \
    _s3SetMix (s3, FSS_FRGDCOL | s3alu[alu], BSS_BKGDCOL|MIX_OR); \
    _s3SetPixelControl (s3, MIXSEL_EXPBLT); \
    DRAW_DEBUG((DEBUG_SET,"  done")); \
}

#define _s3SetOpaqueStipple(s3,alu,mask,_fg,_bg) {\
    DRAW_DEBUG ((DEBUG_SET,"set opaque stipple 0x%x 0x%x %d 0x%x", _fg, _bg, alu, mask)); \
    _s3WaitSlots(s3,5); \
    _s3SetFg (s3, _fg); \
    _s3SetBg (s3, _bg); \
    _s3SetWriteMask(s3,mask); \
    _s3SetMix (s3, FSS_FRGDCOL | s3alu[alu], BSS_BKGDCOL|s3alu[alu]); \
    _s3SetPixelControl (s3, MIXSEL_EXPBLT); \
    DRAW_DEBUG((DEBUG_SET,"  done")); \
}

#define _s3PatRect(s3,px,py,x,y,w,h) {\
    DRAW_DEBUG ((DEBUG_RENDER, "pat rect %d,%d %dx%d", x,y,w,h)); \
    _s3WaitSlots(s3, 4); \
    _s3SetCur (s3, px, py); \
    _s3SetStep (s3, x, y); \
    _s3SetPcnt (s3, (w)-1, (h)-1); \
    _s3SetCmd (s3, CMD_PATBLT|INC_X|INC_Y|DRAW|PLANAR|WRTDATA); \
    DRAW_DEBUG((DEBUG_RENDER,"  done")); \
}

#define _s3SetBlt(s3,alu,mask) { \
    DRAW_DEBUG ((DEBUG_SET,"set blt %d 0x%x", alu, mask)); \
    _s3WaitSlots(s3,3); \
    _s3SetPixelControl (s3, MIXSEL_FRGDMIX); \
    _s3SetMix(s3, FSS_BITBLT | s3alu[alu], BSS_BITBLT | s3alu[alu]); \
    _s3SetWriteMask(s3, mask);  \
    DRAW_DEBUG((DEBUG_SET,"  done")); \
}

#define _s3Blt(s3,_sx,_sy,_dx,_dy,_w,_h,_dir) { \
    DRAW_DEBUG ((DEBUG_RENDER, "blt %d,%d -> %d,%d %dx%d 0x%x", \
		_sx,_sy,_dx,_dy,_w,_h,_dir)); \
    _s3WaitSlots(s3,4); \
    _s3SetCur(s3,_sx,_sy); \
    _s3SetStep(s3,_dx,_dy); \
    _s3SetPcnt(s3,(_w)-1,(_h)-1); \
    _s3SetCmd (s3, CMD_BITBLT | (_dir) | DRAW | WRTDATA); \
    DRAW_DEBUG((DEBUG_RENDER,"  done")); \
}
    
#define _s3SetOpaquePlaneBlt(s3,alu,mask,_fg,_bg) {\
    DRAW_DEBUG ((DEBUG_SET,"set opaque plane blt 0x%x 0x%x %d 0x%x", \
		_fg, _bg, alu, mask)); \
    /* _s3WaitSlots(s3, 5); */ \
    _s3WaitIdleEmpty (s3); \
    _s3SetFg(s3,_fg); \
    _s3SetBg(s3,_bg); \
    _s3SetWriteMask(s3,mask); \
    _s3SetMix(s3,FSS_FRGDCOL|s3alu[alu], BSS_BKGDCOL|s3alu[alu]); \
    _s3SetPixelControl(s3,MIXSEL_EXPPC); \
    DRAW_DEBUG((DEBUG_SET,"  done")); \
}
		       
#define _s3SetTransparentPlaneBlt(s3,alu,mask,_fg) {\
    DRAW_DEBUG ((DEBUG_SET,"set transparent plane blt 0x%x %d 0x%x", \
		_fg, alu, mask)); \
    /*_s3WaitSlots(s3, 4);  */ \
    _s3WaitIdleEmpty (s3); \
    _s3SetFg(s3,_fg); \
    _s3SetWriteMask(s3,mask); \
    _s3SetMix(s3,FSS_FRGDCOL|s3alu[alu], BSS_BKGDCOL|MIX_DST); \
    _s3SetPixelControl(s3,MIXSEL_EXPPC); \
    DRAW_DEBUG((DEBUG_SET,"  done")); \
}
		       
/* Across the plane blt */
#define _s3PlaneBlt(s3,x,y,w,h) {\
    DRAW_DEBUG ((DEBUG_RENDER, "plane blt %d,%d %dx%d", x,y,w,h)); \
    _s3WaitSlots(s3, 4); \
    _s3SetPixelControl(s3,MIXSEL_EXPPC); \
    _s3SetCur(s3, x, y); \
    _s3SetPcnt (s3, (w)-1, (h)-1); \
    _s3SetCmd (s3, \
	       CMD_RECT|    /* Fill rectangle */ \
	       BYTSEQ|	    /* LSB byte order */ \
	       _32BIT|	    /* 32 bit data on 32 bit boundaries */ \
	       PCDATA|	    /* Data from CPU */ \
	       INC_X|INC_Y| /* X and Y both increasing */ \
	       DRAW|	    /* Draw, not move */ \
	       PLANAR|	    /* multi pixel */ \
	       WRTDATA); \
    DRAW_DEBUG((DEBUG_RENDER,"  done")); \
}

#define _s3SetClip(s3,pbox) {\
    DRAW_DEBUG ((DEBUG_SET, "set clip %dx%d -> %dx%d ", \
		pbox->x1, pbox->y1, pbox->x2, pbox->y2)); \
    _s3WaitSlots(s3, 2); \
    _s3SetScissorsTl(s3,(pbox)->x1, (pbox)->y1); \
    _s3SetScissorsBr(s3,(pbox)->x2 - 1, (pbox)->y2 - 1); \
    DRAW_DEBUG((DEBUG_SET,"  done")); \
}

#define _s3ResetClip(s3,pScreen) { \
    DRAW_DEBUG ((DEBUG_SET, "reset clip")); \
    _s3WaitSlots(s3, 2); \
    _s3SetScissorsTl(s3,0,0); \
    _s3SetScissorsBr(s3,pScreen->width - 1, pScreen->height - 1); \
    DRAW_DEBUG((DEBUG_SET,"  done")); \
}

RegionPtr
s3CopyArea(DrawablePtr pSrcDrawable, DrawablePtr pDstDrawable, GCPtr pGC,
	   int srcx, int srcy, int width, int height, int dstx, int dsty);
    
RegionPtr
s3CopyPlane(DrawablePtr pSrcDrawable, DrawablePtr pDstDrawable, GCPtr pGC,
	    int srcx, int srcy, int width, int height, 
	    int dstx, int dsty, unsigned long bitPlane);
    
void
s3PushPixels (GCPtr pGC, PixmapPtr pBitmap,
	      DrawablePtr pDrawable,
	      int w, int h, int x, int y);

void
s3FillBoxSolid (DrawablePtr pDrawable, int nBox, BoxPtr pBox, 
		unsigned long pixel, int alu, unsigned long planemask);
    
void
s3FillBoxPattern (DrawablePtr pDrawable, int nBox, BoxPtr pBox, 
		  int alu, unsigned long planemask, s3PatternPtr pPattern);

void
s3PolyFillRect (DrawablePtr pDrawable, GCPtr pGC, 
		int nrectFill, xRectangle *prectInit);

void
s3FillSpans (DrawablePtr pDrawable, GCPtr pGC, int n, 
	     DDXPointPtr ppt, int *pwidth, int fSorted);
    
void
s3PolyFillArcSolid (DrawablePtr pDraw, GCPtr pGC, int narcs, xArc *parcs);

void
s3FillPoly (DrawablePtr pDrawable, GCPtr pGC, int shape, 
	    int mode, int count, DDXPointPtr ptsIn);

void
s3PolyGlyphBlt (DrawablePtr pDrawable, 
		GCPtr pGC, 
		int xInit, int y, 
		unsigned int nglyphInit,
		CharInfoPtr *ppciInit, 
		pointer pglyphBase);

void
s3ImageGlyphBlt (DrawablePtr pDrawable, 
		GCPtr pGC, 
		int x, int y, 
		unsigned int nglyph, 
		CharInfoPtr *ppci, 
		pointer pglyphBase);

void
s3ImageTEGlyphBlt (DrawablePtr pDrawable, GCPtr pGC,
		   int xInit, int y,
		   unsigned int nglyphInit,
		   CharInfoPtr *ppciInit,
		   pointer pglyphBase);

void
s3PolyTEGlyphBlt (DrawablePtr pDrawable, GCPtr pGC, 
		  int x, int y, 
		  unsigned int nglyph, CharInfoPtr *ppci,
		  pointer pglyphBase);

void
s3Polylines (DrawablePtr pDrawable, GCPtr pGC,
	     int mode, int nptInit, DDXPointPtr pptInit);

void
s3PolySegment (DrawablePtr pDrawable, GCPtr pGC, 
	       int nsegInit, xSegment *pSegInit);

void
s3FillBoxSolid (DrawablePtr pDrawable, int nBox, BoxPtr pBox, 
		unsigned long pixel, int alu, unsigned long planemask);

void	s3ValidateGC (GCPtr pGC, Mask changes, DrawablePtr pDrawable);

void
s3CheckGCFill (GCPtr pGC);

void
s3MoveGCFill (GCPtr pGC);

void
s3SyncProc (DrawablePtr pDrawable);

#endif
