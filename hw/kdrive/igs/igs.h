/*
 * $XFree86$
 *
 * Copyright © 1999 SuSE, Inc.
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

#ifndef _IGS_H_
#define _IGS_H_

#include "kdrive.h"

extern KdCardFuncs  igsFuncs;

/*
 * FB		0x00000000
 * VGA		0x00800000
 * Blt window	0x008a0000
 * Coprocessor	0x008bf000
 */

#define IGS_COP_DATA	    0x008a0000
#define IGS_COP_DATA_LEN    0x00010000
#define IGS_COP_OFFSET	    0x008bf000

typedef volatile CARD8	VOL8;
typedef volatile CARD16	VOL16;
typedef volatile CARD32	VOL32;

typedef struct _Cop5xxx {
    VOL8	pad000[0x11];		    /* 0x000 */
    
    VOL8	control;		    /* 0x011 */
#define IGS_CONTROL_HBLTW_RDYZ	    0x01
#define IGS_CONTROL_MALLWBEPTZ	    0x02
#define IGS_CONTROL_CMDFF	    0x04
#define IGS_CONTROL_SOP		    0x08
#define IGS_CONTROL_OPS		    0x10
#define IGS_CONTROL_TER		    0x20
#define IGS_CONTROL_HBACKZ	    0x40
#define IGS_CONTROL_BUSY	    0x80
    
    VOL8	pad012[0x06];		    /* 0x012 */
    
    VOL32	src1_stride;		    /* 0x018 */
    
    VOL32	format;			    /* 0x01c */

#define IGS_FORMAT_8BPP	    0
#define IGS_FORMAT_16BPP    1
#define IGS_FORMAT_24BPP    2
#define IGS_FORMAT_32BPP    3

    VOL32	bres_error;		    /* 0x020 */
    VOL32	bres_k1;		    /* 0x024 */
    VOL32	bres_k2;		    /* 0x028 */
    VOL8	pad02c[0x1c];		    /* 0x02c */
    
    VOL32	mix;			    /* 0x048 */
#define IGS_MIX_FG	0x00ff
#define IGS_MIX_BG	0xff00
#define IGS_MAKE_MIX(fg,bg)	((fg) | ((bg) << 8))

#define IGS_MIX_ZERO		    0x0
#define IGS_MIX_SRC_AND_DST	    0x1
#define IGS_MIX_SRC_AND_NOT_DST	    0x2
#define IGS_MIX_SRC		    0x3
#define IGS_MIX_NOT_SRC_AND_DST	    0x4
#define IGS_MIX_DST		    0x5
#define IGS_MIX_SRC_XOR_DST	    0x6
#define IGS_MIX_SRC_OR_DST	    0x7
#define IGS_MIX_NOT_SRC_AND_NOT_DST 0x8
#define IGS_MIX_SRC_XOR_NOT_DST	    0x9
#define IGS_MIX_NOT_DST		    0xa
#define IGS_MIX_SRC_OR_NOT_DST	    0xb
#define IGS_MIX_NOT_SRC		    0xc
#define IGS_MIX_NOT_SRC_OR_DST	    0xd
#define IGS_MIX_NOT_SRC_OR_NOT_DST  0xe
#define IGS_MIX_ONE		    0xf
    
    VOL32	colorComp;		    /* 0x04c */
    VOL32	planemask;		    /* 0x050 */
    
    VOL8	pad054[0x4];		    /* 0x054 */
    
    VOL32	fg;			    /* 0x058 */
    VOL32	bg;			    /* 0x05c */
    VOL32	dim;			    /* 0x060 */
#define IGS_MAKE_DIM(w,h)   ((w) | ((h) << 16))
    VOL8	pad5[0x0c];		    /* 0x064 */
    
    VOL32	src1_base_address;	    /* 0x070 */
    VOL8	pad074[0x04];		    /* 0x074 */
    
    VOL16	dst_x_rotate;		    /* 0x078 */
    VOL16	pat_y_rotate;		    /* 0x07a */
    VOL32	operation;		    /* 0x07c */

/* OCT[2:0] */
#define IGS_DRAW_X_MAJOR	0x00000000
#define IGS_DRAW_Y_MAJOR	0x00000001
#define IGS_DRAW_T_B		0x00000000
#define IGS_DRAW_B_T		0x00000002
#define IGS_DRAW_L_R		0x00000000
#define IGS_DRAW_R_L		0x00000004
    
/* Draw_Mode[1:0] */
#define IGS_DRAW_ALL		0x00000000
#define IGS_DRAW_NOT_FIRST	0x00000010
#define IGS_DRAW_NOT_LAST	0x00000020
#define IGS_DRAW_NOT_FIRST_LAST	0x00000030

/* TRPS[1:0] */
#define IGS_TRANS_SRC1		0x00000000
#define IGS_TRANS_SRC2		0x00000100
#define IGS_TRANS_DST		0x00000200
/* TRPS2 Transparent Invert */
#define IGS_TRANS_INVERT	0x00000400
/* TRPS3, Transparent Enable */
#define IGS_TRANS_ENABLE	0x00000800

/* PPS[3:0], Pattern Pixel Select */
#define IGS_PIXEL_TEXT_OPAQUE	0x00001000
#define IGS_PIXEL_STIP_OPAQUE	0x00002000
#define IGS_PIXEL_LINE_OPAQUE	0x00003000
#define IGS_PIXEL_TEXT_TRANS	0x00005000
#define IGS_PIXEL_STIP_TRANS	0x00006000
#define IGS_PIXEL_LINE_TRANS	0x00007000
#define IGS_PIXEL_FG		0x00008000
#define IGS_PIXEL_TILE		0x00009000
    
/* HostBltEnable[1:0] */
#define IGS_HBLT_DISABLE	0x00000000
#define IGS_HBLT_READ		0x00010000
#define IGS_HBLT_WRITE_1	0x00020000
#define IGS_HBLT_WRITE_2	0x00030000

/* Src2MapSelect[2:0], Src2 map select mode */
#define IGS_SRC2_NORMAL		0x00000000
#define IGS_SRC2_MONO_OPAQUE	0x00100000
#define IGS_SRC2_FG		0x00200000
#define IGS_SRC2_MONO_TRANS	0x00500000

/* StepFunction[3:0], Step function select */
#define IGS_STEP_DRAW_AND_STEP	0x04000000
#define IGS_STEP_LINE_DRAW	0x05000000
#define IGS_STEP_PXBLT		0x08000000
#define IGS_STEP_INVERT_PXBLT	0x09000000
#define IGS_STEP_TERNARY_PXBLT	0x0b000000

/* FGS */
#define IGS_FGS_FG		0x00000000
#define IGS_FGS_SRC		0x20000000
    
/* BGS */
#define IGS_BGS_BG		0x00000000
#define IGS_BGS_SRC		0x80000000
    VOL8	pad080[0x91];		    /* 0x080 */
    
    VOL8	debug_control_1;	    /* 0x111 */
    VOL8	debug_control_2;	    /* 0x112 */
    VOL8	pad113[0x05];		    /* 0x113 */
    
    VOL32	src2_stride;		    /* 0x118 */
    VOL8	pad11c[0x14];		    /* 0x11c */
    
    VOL32	extension;		    /* 0x130 */

#define IGS_BURST_ENABLE    0x01
#define IGS_STYLE_LINE	    0x02
#define IGS_ADDITIONAL_WAIT 0x04
#define	IGS_BLOCK_COP_REG   0x08
#define IGS_TURBO_MONO	    0x10
#define IGS_SELECT_SAMPLE   0x40
#define IGS_MDSBL_RD_B_WR   0x80
#define IGS_WRMRSTZ	    0x100
#define IGS_TEST_MTST	    0x200

    VOL8	style_line_roll_over;	    /* 0x134 */
    VOL8	style_line_inc;		    /* 0x135 */
    VOL8	style_line_pattern;	    /* 0x136 */
    VOL8	style_line_accumulator;	    /* 0x137 */
    VOL8	style_line_pattern_index;   /* 0x138 */
    VOL8	pad139[0x3];		    /* 0x139 */

    VOL16	mono_burst_total;	    /* 0x13c */
    VOL8	pad13e[0x12];		    /* 0x13e */
    
    VOL8	pat_x_rotate;		    /* 0x150 */
    VOL8	pad151[0x1f];		    /* 0x151 */

    VOL32	src1_start;		    /* 0x170 */
    VOL32	src2_start;		    /* 0x174 */
    VOL32	dst_start;		    /* 0x178 */
    VOL8	pad17c[0x9c];		    /* 0x17c */
    
    VOL16	dst_stride;		    /* 0x218 */
} Cop5xxx;

typedef struct _igsCardInfo {
    Cop5xxx	*cop;
    VOL32	*copData;
    Bool	need_sync;
    CARD8	*frameBuffer;
} IgsCardInfo;
    
#define getIgsCardInfo(kd)  ((IgsCardInfo *) ((kd)->card->driver))
#define igsCardInfo(kd)	    IgsCardInfo	*igsc = getIgsCardInfo(kd)

Bool
igsDrawInit (ScreenPtr pScreen);

void
igsDrawEnable (ScreenPtr pScreen);

void
igsDrawDisable (ScreenPtr pScreen);

void
igsDrawSync (ScreenPtr pScreen);

void
igsDrawFini (ScreenPtr pScreen);

    
#endif /* _IGS_H_ */
