/*
 * $XFree86$
 *
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

#define _igsWaitFull(cop)   _igsWaitLoop(cop, \
					 IGS_CONTROL_CMDFF, \
					 0)

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

#define IgsAdjustBits32(b)  IgsInvertBits32(b)

#define _igsSetSolidRect(cop,alu,pm,pix,cmd) {\
    _igsWaitFull(cop); \
    (cop)->mix = IGS_MAKE_MIX(alu,alu); \
    (cop)->fg = (pix); \
    (cop)->planemask = (pm); \
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

#define _igsRect(cop,x,y,w,h,cmd) { \
    _igsWaitFull(cop); \
    (cop)->dst_start = (x) + (y) * (cop_stride); \
    (cop)->dim = IGS_MAKE_DIM(w-1,h-1); \
    (cop)->operation = (cmd); \
}

#define _igsSetBlt(cop,alu,pm,backwards,upsidedown,cmd) { \
    _igsWaitFull(cop); \
    (cop)->mix = IGS_MAKE_MIX(alu,GXnoop); \
    (cop)->planemask = (pm); \
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
    (cop)->mix = IGS_MAKE_MIX(igsPatRop[alu],igsPatRop[GXnoop]); \
    (cop)->fg = (fg_pix); \
    (cop)->planemask = (pm); \
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
    (cop)->planemask = (pm); \
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
