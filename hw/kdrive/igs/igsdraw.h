/*
 * Copyright © 2000 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _IGSDRAW_H_
#define _IGSDRAW_H_

extern CARD8	igsPatRop[];

#define SetupIgs(s) KdScreenPriv(s); \
		    igsCardInfo(pScreenPriv); \
		    Cop5xxx *cop = igsc->cop; \
		    int	    cop_stride = pScreenPriv->screen->fb[0].pixelStride

#define _igsWaitLoop(cop,mask,value) { \
    int __loop = 1000000; \
    while (((cop)->control & (mask)) != (value)) { \
	if (--__loop <= 0) { \
	    FatalError("Warning: igsWaitLoop 0x%x 0x%x failed\n", mask, value); \
	} \
    } \
}

#define _igsWaitDone(cop)   _igsWaitLoop(cop, \
					 (IGS_CONTROL_BUSY| \
					  IGS_CONTROL_MALLWBEPTZ), \
					 0)

#if 1
#define _igsWaitFull(cop)   _igsWaitLoop(cop, \
					 IGS_CONTROL_CMDFF, \
					 0)
#else
#define _igsWaitFull(cop)   _igsWaitDone(cop)
#endif

#define _igsWaitIdleEmpty(cop)	_igsWaitDone(cop)
#define _igsWaitHostBltAck(cop)	_igsWaitLoop(cop, \
					     (IGS_CONTROL_HBACKZ| \
					      IGS_CONTROL_CMDFF), \
					     0)
					      
#define _igsReset(cop)		((cop)->control = 0)

#define IgsInvertBits32(v) { \
    v = ((v & 0x55555555) << 1) | ((v >> 1) & 0x55555555); \
    v = ((v & 0x33333333) << 2) | ((v >> 2) & 0x33333333); \
    v = ((v & 0x0f0f0f0f) << 4) | ((v >> 4) & 0x0f0f0f0f); \
}

#define IgsInvertBits16(v) { \
    v = ((v & 0x5555) << 1) | ((v >> 1) & 0x5555); \
    v = ((v & 0x3333) << 2) | ((v >> 2) & 0x3333); \
    v = ((v & 0x0f0f) << 4) | ((v >> 4) & 0x0f0f); \
}

#define IgsInvertBits8(v) { \
    v = ((v & 0x55) << 1) | ((v >> 1) & 0x55); \
    v = ((v & 0x33) << 2) | ((v >> 2) & 0x33); \
    v = ((v & 0x0f) << 4) | ((v >> 4) & 0x0f); \
}

#define IgsByteSwap32(x)    	((x) = (((x) >> 24) | \
					(((x) >> 8) & 0xff00) | \
					(((x) << 8) & 0xff0000) | \
					((x) << 24)))

#define IgsByteSwap16(x)    	((x) = ((x) << 8) | ((x) >> 8))

#define igsPatternDimOk(d)  ((d) <= IGS_PATTERN_WIDTH && (((d) & ((d) - 1)) == 0))

#if BITMAP_BIT_ORDER == LSBFirst
#define IgsAdjustBits32(b)	IgsInvertBits32(b)
#define IgsAdjustBits16(x)	IgsInvertBits16(x)
#else
#define IgsAdjustBits32(x)	IgsByteSwap32(x)
#define IgsAdjustBits16(x)	IgsByteSwap16(x)
#endif

#define _igsSetSolidRect(cop,alu,pm,pix,cmd) {\
    _igsWaitFull(cop); \
    (cop)->mix = IGS_MAKE_MIX(alu,alu); \
    (cop)->fg = (pix); \
    (cmd) = (IGS_DRAW_T_B | \
	     IGS_DRAW_L_R | \
	     IGS_DRAW_ALL | \
	     IGS_PIXEL_FG | \
	     IGS_HBLT_DISABLE | \
	     IGS_SRC2_NORMAL | \
	     IGS_STEP_PXBLT | \
	     IGS_FGS_FG | \
	     IGS_BGS_BG); \
}

#define _igsSetTiledRect(cop,alu,pm,base,cmd) {\
    _igsWaitFull(cop); \
    (cop)->mix = IGS_MAKE_MIX(alu,alu); \
    (cop)->src1_stride = IGS_PATTERN_WIDTH - 1; \
    (cop)->src1_start = (base); \
    (cmd) = (IGS_DRAW_T_B | \
	     IGS_DRAW_L_R | \
	     IGS_DRAW_ALL | \
	     IGS_PIXEL_TILE | \
	     IGS_HBLT_DISABLE | \
	     IGS_SRC2_NORMAL | \
	     IGS_STEP_PXBLT | \
	     IGS_FGS_SRC | \
	     IGS_BGS_BG); \
}

#define _igsSetStippledRect(cop,alu,pm,pix,base,cmd) {\
    _igsWaitFull(cop); \
    (cop)->mix = IGS_MAKE_MIX(alu,alu); \
    (cop)->src1_start = (base); \
    (cop)->fg = (pix); \
    (cmd) = (IGS_DRAW_T_B | \
	     IGS_DRAW_L_R | \
	     IGS_DRAW_ALL | \
	     IGS_PIXEL_STIP_TRANS | \
	     IGS_HBLT_DISABLE | \
	     IGS_SRC2_NORMAL | \
	     IGS_STEP_PXBLT | \
	     IGS_FGS_FG | \
	     IGS_BGS_BG); \
}

#define _igsSetOpaqueStippledRect(cop,alu,pm,_fg,_bg,base,cmd) {\
    _igsWaitFull(cop); \
    (cop)->mix = IGS_MAKE_MIX(alu,alu); \
    (cop)->src1_start = (base); \
    (cop)->fg = (_fg); \
    (cop)->bg = (_bg); \
    (cmd) = (IGS_DRAW_T_B | \
	     IGS_DRAW_L_R | \
	     IGS_DRAW_ALL | \
	     IGS_PIXEL_STIP_OPAQUE | \
	     IGS_HBLT_DISABLE | \
	     IGS_SRC2_NORMAL | \
	     IGS_STEP_PXBLT | \
	     IGS_FGS_FG | \
	     IGS_BGS_BG); \
}

#define _igsRect(cop,x,y,w,h,cmd) { \
    _igsWaitFull(cop); \
    (cop)->dst_start = (x) + (y) * (cop_stride); \
    (cop)->dim = IGS_MAKE_DIM(w-1,h-1); \
    (cop)->operation = (cmd); \
}

#define _igsPatRect(cop,x,y,w,h,cmd) { \
    _igsWaitFull(cop); \
    (cop)->dst_start = (x) + (y) * (cop_stride); \
    (cop)->rotate = IGS_MAKE_ROTATE(x&7,y&7); \
    (cop)->dim = IGS_MAKE_DIM(w-1,h-1); \
    (cop)->operation = (cmd); \
}

#define _igsSetBlt(cop,alu,pm,backwards,upsidedown,cmd) { \
    _igsWaitFull(cop); \
    (cop)->mix = IGS_MAKE_MIX(alu,alu); \
    (cop)->src1_stride = cop_stride - 1; \
    (cmd) = (IGS_DRAW_ALL | \
	     IGS_PIXEL_FG | \
	     IGS_HBLT_DISABLE | \
	     IGS_SRC2_NORMAL | \
	     IGS_STEP_PXBLT | \
	     IGS_FGS_SRC | \
	     IGS_BGS_BG); \
    if (backwards) (cmd) |= IGS_DRAW_R_L; \
    if (upsidedown) (cmd) |= IGS_DRAW_B_T; \
}

#if 0
#define _igsPreparePlaneBlt(cop) { \
    _igsReset(cop); \
    (cop)->dst_stride = cop_stride - 1; \
    (cop)->src1_stride = cop_stride - 1; \
    (cop)->src2_stride = cop_stride - 1; \
    (cop)->format = IGS_FORMAT_16BPP;  \
    (cop)->src1_start = 0; \
    (cop)->src2_start = 0; \
}
#else
#define _igsPreparePlaneBlt(cop)
#endif
    
#define _igsSetTransparentPlaneBlt(cop,alu,pm,fg_pix,cmd) { \
    _igsWaitIdleEmpty(cop); \
    _igsPreparePlaneBlt(cop); \
    (cop)->mix = IGS_MAKE_MIX(igsPatRop[alu],igsPatRop[alu]); \
    (cop)->fg = (fg_pix); \
    (cmd) = (IGS_DRAW_T_B | \
	     IGS_DRAW_L_R | \
	     IGS_DRAW_ALL | \
	     IGS_PIXEL_FG | \
	     IGS_HBLT_WRITE_2 | \
	     IGS_SRC2_MONO_TRANS | \
	     IGS_STEP_TERNARY_PXBLT | \
	     IGS_FGS_FG | \
	     IGS_BGS_BG); \
}

#define _igsSetOpaquePlaneBlt(cop,alu,pm,fg_pix,bg_pix,cmd) { \
    _igsWaitIdleEmpty(cop); \
    _igsPreparePlaneBlt(cop); \
    (cop)->mix = IGS_MAKE_MIX(igsPatRop[alu],igsPatRop[alu]); \
    (cop)->fg = (fg_pix); \
    (cop)->bg = (bg_pix); \
    (cmd) = (IGS_DRAW_T_B | \
	     IGS_DRAW_L_R | \
	     IGS_DRAW_ALL | \
	     IGS_PIXEL_FG | \
	     IGS_HBLT_WRITE_2 | \
	     IGS_SRC2_MONO_OPAQUE | \
	     IGS_STEP_TERNARY_PXBLT | \
	     IGS_FGS_FG | \
	     IGS_BGS_BG); \
}

#define _igsPlaneBlt(cop,x,y,w,h,cmd) { \
/*    _igsWaitFull(cop); */ \
    (cop)->dst_start = (x) + (y) * (cop_stride); \
    (cop)->dim = IGS_MAKE_DIM((w)-1,(h)-1); \
    (cop)->operation = (cmd); \
/*    _igsWaitHostBltAck(cop); */ \
}
    
#define _igsBlt(cop,sx,sy,dx,dy,w,h,cmd) { \
    _igsWaitFull(cop); \
    (cop)->dst_start = (dx) + (dy) * cop_stride; \
    (cop)->src1_start = (sx) + (sy) * cop_stride; \
    (cop)->src1_stride = cop_stride - 1; \
    (cop)->dim = IGS_MAKE_DIM(w-1,h-1); \
    (cop)->operation = (cmd); \
}

#define sourceInvarient(alu)	(((alu) & 3) == (((alu) >> 2) & 3))

#endif
