/*
 * Copyright © 1999 Keith Packard
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

#ifndef _SISDRAW_H_
#define _SISDRAW_H_

#define SetupSis(s)  KdScreenPriv(s); \
		    sisCardInfo(pScreenPriv); \
		    SisPtr sis = sisc->sis

#define SIS_CMD_BITBLT			(0)
#define SIS_CMD_COLOR_EXPAND		(1)
#define SIS_CMD_ENH_COLOR_EXPAND    	(2)
#define SIS_CMD_MULTI_SCANLINE		(3)
#define SIS_CMD_LINE			(4)
#define SIS_CMD_TRAPEZOID    		(5)
#define SIS_CMD_TRANSPARENT_BITBLT    	(6)
					 
#define SIS_CMD_SRC_SCREEN		(0 << 4)
#define SIS_CMD_SRC_CPU			(1 << 4)

#define SIS_CMD_PAT_FG			(0 << 6)
#define SIS_CMD_PAT_PATTERN    		(1 << 6)
#define SIS_CMD_PAT_MONO		(2 << 6)
					 
/* 8->15 rop */

#define SIS_CMD_DEC_X			(0 << 16)
#define SIS_CMD_INC_X			(1 << 16)

#define SIS_CMD_DEC_Y			(0 << 17)
#define SIS_CMD_INC_Y			(1 << 17)
					 
#define SIS_CMD_RECT_CLIP_DISABLE	(0 << 18)
#define SIS_CMD_RECT_CLIP_ENABLE	(1 << 18)

#define SIS_CMD_OPAQUE			(0 << 20)
#define SIS_CMD_TRANSPARENT		(1 << 20)

#define SIS_CMD_RECT_CLIP_MERGE		(0 << 26)
#define SIS_CMD_RECT_CLIP_DONT_MERGE	(1 << 26)

#define SIS_STAT_2D_IDLE	    	(1 << 31)
#define SIS_STAT_3D_IDLE		(1 << 30)
#define SIS_STAT_EMPTY			(1 << 29)
#define SIS_STAT_CPU_BITBLT		(0xf << 24)
#define SIS_STAT_ENH_COLOR_EXPAND	(1 << 23)
#define SIS_STAT_AVAIL			(0x1fff)

extern CARD8	sisPatRop[16];
extern CARD8	sisBltRop[16];
					 
#define _sisSetSolidRect(sis,pix,alu,cmd) {\
    (sis)->u.general.pattern_fg = (pix); \
    (cmd) = (SIS_CMD_BITBLT | \
	     SIS_CMD_SRC_SCREEN | \
	     SIS_CMD_PAT_FG | \
	     (sisPatRop[alu] << 8) | \
	     SIS_CMD_INC_X | \
	     SIS_CMD_INC_Y | \
	     SIS_CMD_RECT_CLIP_DISABLE | \
	     SIS_CMD_OPAQUE | \
	     SIS_CMD_RECT_CLIP_DONT_MERGE); \
}

#define _sisClip(sis,x1,y1,x2,y2) { \
    (sis)->u.general.clip_left = (x1); \
    (sis)->u.general.clip_top = (y1); \
    (sis)->u.general.clip_right = (x2); \
    (sis)->u.general.clip_bottom = (y2); \
}

#define _sisRect(sis,x,y,w,h,cmd) { \
    (sis)->u.general.dst_x = (x); \
    (sis)->u.general.dst_y = (y); \
    (sis)->u.general.rect_width = (w); \
    (sis)->u.general.rect_height = (h); \
    (sis)->u.general.command = (cmd); \
}

#define _sisSetTransparentPlaneBlt(sis, alu, fg, cmd) { \
    (sis)->u.general.src_fg = (fg); \
    (cmd) = (SIS_CMD_ENH_COLOR_EXPAND | \
	     SIS_CMD_SRC_CPU | \
	     SIS_CMD_PAT_FG | \
	     (sisBltRop[alu] << 8) | \
	     SIS_CMD_INC_X | \
	     SIS_CMD_INC_Y | \
	     SIS_CMD_RECT_CLIP_DISABLE | \
	     SIS_CMD_TRANSPARENT | \
	     SIS_CMD_RECT_CLIP_DONT_MERGE); \
}

#define _sisSetOpaquePlaneBlt(sis, alu, fg, bg, cmd) { \
    (sis)->u.general.src_fg = (fg); \
    (sis)->u.general.src_bg = (bg); \
    (cmd) = (SIS_CMD_ENH_COLOR_EXPAND | \
	     SIS_CMD_SRC_CPU | \
	     SIS_CMD_PAT_FG | \
	     (sisBltRop[alu] << 8) | \
	     SIS_CMD_INC_X | \
	     SIS_CMD_INC_Y | \
	     SIS_CMD_RECT_CLIP_DISABLE | \
	     SIS_CMD_OPAQUE | \
	     SIS_CMD_RECT_CLIP_DONT_MERGE); \
}

#define _sisPlaneBlt(sis,x,y,w,h,cmd) _sisSolidRect(sis,x,y,w,h,cmd)

#define _sisSetBlt(sis,alu,cmd) { \
    (sis)->u.general.src_base = (sis)->u.general.dst_base; \
    (sis)->u.general.src_pitch = (sis)->u.general.dst_pitch; \
    (cmd) = (SIS_CMD_RECT_CLIP_DONT_MERGE |\
	     (sisBltRop[alu] << 8) |\
	     SIS_CMD_PAT_FG |\
	     SIS_CMD_SRC_SCREEN |\
	     SIS_CMD_BITBLT); \
}

#define _sisBlt(sis,sx,sy,dx,dy,w,h,cmd) { \
    (sis)->u.general.src_x = (sx); \
    (sis)->u.general.src_y = (sy); \
    (sis)->u.general.dst_x = (dx); \
    (sis)->u.general.dst_y = (dy); \
    (sis)->u.general.rect_width = (w); \
    (sis)->u.general.rect_height = (h); \
    (sis)->u.general.command = (cmd); \
}
		
#define SIS_IE	(SIS_STAT_2D_IDLE|SIS_STAT_EMPTY)

#define _sisWaitIdleEmpty(sis) \
    while (((sis)->u.general.status & SIS_IE) != SIS_IE)

/*
 * Ok, so the Sis530 is broken -- it expects bitmaps to come MSB bit order,
 * but it's willing to take them in LSB byte order.  These macros
 * flip bits around without flipping bytes.  Instead of using a table
 * and burning memory bandwidth, do them in place with the CPU.
 */

/* The MIPS compiler automatically places these constants in registers */
#define SisInvertBits32(v) { \
    v = ((v & 0x55555555) << 1) | ((v >> 1) & 0x55555555); \
    v = ((v & 0x33333333) << 2) | ((v >> 2) & 0x33333333); \
    v = ((v & 0x0f0f0f0f) << 4) | ((v >> 4) & 0x0f0f0f0f); \
}

#define SisInvertBits16(v) { \
    v = ((v & 0x5555) << 1) | ((v >> 1) & 0x5555); \
    v = ((v & 0x3333) << 2) | ((v >> 2) & 0x3333); \
    v = ((v & 0x0f0f) << 4) | ((v >> 4) & 0x0f0f); \
}

#define SisInvertBits8(v) { \
    v = ((v & 0x55) << 1) | ((v >> 1) & 0x55); \
    v = ((v & 0x33) << 2) | ((v >> 2) & 0x33); \
    v = ((v & 0x0f) << 4) | ((v >> 4) & 0x0f); \
}

#endif
