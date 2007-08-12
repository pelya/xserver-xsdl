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

#ifndef _S3_H_
#define _S3_H_

#include "kdrive.h"

#define PLATFORM 300

#define DRAW_DEBUG(a)

#define DEBUG_S3INIT	(DEBUG_ACCEL)
#define DEBUG_CRTC	(DEBUG_ACCEL+1)
#define DEBUG_PATTERN	(DEBUG_ACCEL+2)
#define DEBUG_RECT	(DEBUG_ACCEL+3)
#define DEBUG_PAINT_WINDOW	(DEBUG_ACCEL+4)
#define DEBUG_SET	(DEBUG_ACCEL+5)
#define DEBUG_RENDER	(DEBUG_ACCEL+6)
#define DEBUG_REGISTERS	(DEBUG_ACCEL+7)
#define DEBUG_ARCS	(DEBUG_ACCEL+8)
#define DEBUG_TEXT	(DEBUG_ACCEL+9)
#define DEBUG_POLYGON	(DEBUG_ACCEL+10)
#define DEBUG_CLIP	(DEBUG_ACCEL+11)

/* VESA Approved Register Definitions */

/*
 *  Linear Addressing		000 0000 - 0ff ffff (16m)
 *  Image data transfer		100 0000 - 100 7fff (32k)
 *  PCI config			100 8000 - 100 8043
 *  Packed enhanced regs	100 8100 - 100 814a
 *  Streams regs		100 8180 - 100 81ff
 *  Current Y pos		100 82e8
 *  CRT VGA 3b? regs		100 83b0 - 100 83bf
 *  CRT VGA 3c? regs		100 83c0 - 100 83cf
 *  CRT VGA 3d? regs		100 83d0 - 100 83df
 *  Subsystem status (42e8h)	100 8504
 *  Advanced function (42e8h)	100 850c
 *  Enhanced regs		100 86e8 - 100 eeea
 *  Local peripheral bus	100 ff00 - 100 ff5c
 *
 * We don't care about the image transfer or PCI regs, so
 * this structure starts at the packed enhanced regs
 */
    
typedef volatile CARD32 VOL32;
typedef volatile CARD16 VOL16;
typedef volatile CARD8 VOL8;

typedef volatile struct _s3 {
    VOL32	alt_curxy;		/* 8100 */
    VOL32	_pad0;			/* 8104 */
    VOL32	alt_step;		/* 8108 */
    VOL32	_pad1;			/* 810c */
    VOL32	err_term;		/* 8110 */
    VOL32	_pad2;			/* 8114 */
    VOL32	cmd_gp_stat; 		/* 8118 */
    VOL32	short_stroke;		/* 811c */
    VOL32	bg;			/* 8120 */
    VOL32	fg;			/* 8124 */
    VOL32	write_mask;		/* 8128 */
    VOL32	read_mask;		/* 812c */
    VOL32	color_cmp;		/* 8130 */
    VOL32	alt_mix;		/* 8134 */
    VOL32	scissors_tl;		/* 8138 */
    VOL32	scissors_br;		/* 813c */
    VOL32	pix_cntl_mult_misc2;	/* 8140 */
    VOL32	mult_misc_read_sel;	/* 8144 */
    VOL32	alt_pcnt;		/* 8148 min_axis_pcnt, maj_axis_pcnt */
    VOL8	_pad3[0x19c];		/* 814c */
    VOL16	cur_y;			/* 82e8 */
    VOL8	_pad4[0xc6];		/* 82ea */
    
    VOL8	crt_vga_3b0;		/* 83b0 */
    VOL8	crt_vga_3b1;		/* 83b1 */
    VOL8	crt_vga_3b2;		/* 83b2 */
    VOL8	crt_vga_3b3;		/* 83b3 */
    VOL8	crt_vga_3b4;		/* 83b4 */
    VOL8	crt_vga_3b5;		/* 83b5 */
    VOL8	crt_vga_3b6;		/* 83b6 */
    VOL8	crt_vga_3b7;    	/* 83b7 */
    VOL8	crt_vga_3b8;    	/* 83b8 */
    VOL8	crt_vga_3b9;    	/* 83b9 */
    VOL8	crt_vga_3ba;		/* 83ba */
    VOL8	crt_vga_3bb;		/* 83bb */
    VOL8	crt_vga_3bc;		/* 83bc */
    VOL8	crt_vga_3bd;		/* 83bd */
    VOL8	crt_vga_3be;		/* 83be */
    VOL8	crt_vga_3bf;		/* 83bf */
    
    VOL8	crt_vga_3c0;		/* 83c0 */
    VOL8	crt_vga_3c1;		/* 83c1 */
    VOL8	crt_vga_3c2;		/* 83c2 */
    VOL8	crt_vga_3c3;		/* 83c3 */
    VOL8	crt_vga_3c4;		/* 83c4 */
    VOL8	crt_vga_3c5;		/* 83c5 */
    VOL8	crt_vga_dac_ad_mk;	/* 83c6 */
    VOL8	crt_vga_dac_rd_ad;    	/* 83c7 */
    VOL8	crt_vga_dac_wt_ad;    	/* 83c8 */
    VOL8	crt_vga_dac_data;    	/* 83c9 */
    VOL8	crt_vga_3ca;		/* 83ca */
    VOL8	crt_vga_3cb;		/* 83cb */
    VOL8	crt_vga_3cc;		/* 83cc */
    VOL8	crt_vga_3cd;		/* 83cd */
    VOL8	crt_vga_3ce;		/* 83ce */
    VOL8	crt_vga_3cf;		/* 83cf */
    
    VOL8	crt_vga_3d0;		/* 83d0 */
    VOL8	crt_vga_3d1;		/* 83d1 */
    VOL8	crt_vga_3d2;		/* 83d2 */
    VOL8	crt_vga_3d3;		/* 83d3 */
    VOL8	crt_vga_3d4;		/* 83d4 */
    VOL8	crt_vga_3d5;		/* 83d5 */
    VOL8	crt_vga_3d6;		/* 83d6 */
    VOL8	crt_vga_3d7;    	/* 83d7 */
    VOL8	crt_vga_3d8;    	/* 83d8 */
    VOL8	crt_vga_3d9;    	/* 83d9 */
    VOL8	crt_vga_status_1;    	/* 83da */
    VOL8	crt_vga_3db;		/* 83db */
    VOL8	crt_vga_3dc;		/* 83dc */
    VOL8	crt_vga_3dd;		/* 83dd */
    VOL8	crt_vga_3de;		/* 83de */
    VOL8	crt_vga_3df;		/* 83df */
    
    VOL8	_pad5[0x124];		/* 83e0 */
    VOL16	subsys_status;		/* 8504 */
    VOL8	_pad6[0x6];		/* 8506 */
    VOL16	adv_control;		/* 850c */
    VOL8	_pad7[0x1da];		/* 850e */
    VOL16	cur_x;			/* 86e8 */
    VOL8	_pad8[0x3fe];		/* 86ea */
    VOL16	desty_axstp;		/* 8ae8 */
    VOL8	_pad9[0x3fe];		/* 8aea */
    VOL16	destx_diastp;		/* 8ee8 */
    VOL8	_pad10[0x3fe];		/* 8eea */
    VOL16	enh_err_term;		/* 92e8 */
    VOL8	_pad11[0x3fe];		/* 92ea */
    VOL16	maj_axis_pcnt;		/* 96e8 */
    VOL8	_pad12[0x3fe];		/* 96ea */
    VOL16	enh_cmd_gp_stat;    	/* 9ae8 */
    VOL8	_pad13[0x3fe];		/* 9aea */
    VOL16	enh_short_stroke;    	/* 9ee8 */
    VOL8	_pad14[0x3fe];		/* 9eea */
    VOL16	enh_bg;			/* a2e8 */
    VOL8	_pad15[0x3fe];		/* a2ea */
    VOL16	enh_fg;			/* a6e8 */
    VOL8	_pad16[0x3fe];		/* a6ea */
    VOL16	enh_wrt_mask;		/* aae8 */
    VOL8	_pad17[0x3fe];		/* aaea */
    VOL16	enh_rd_mask;	    	/* aee8 */
    VOL8	_pad18[0x3fe];		/* aeea */
    VOL16	enh_color_cmp;		/* b2e8 */
    VOL8	_pad19[0x3fe];		/* b2ea */
    VOL16	enh_bg_mix;    		/* b6e8 */
    VOL8	_pad20[0x3fe];		/* b6ea */
    VOL16	enh_fg_mix;    		/* bae8 */
    VOL8	_pad21[0x3fe];		/* baea */
    VOL16	enh_rd_reg_dt;		/* bee8 */
    VOL8	_pad22[0x23fe];		/* beea */
    VOL32	pix_trans;		/* e2e8 */
} S3, *S3Ptr;

#define VGA_STATUS_1_DTM    0x01
#define VGA_STATUS_1_VSY    0x08

#define	DAC_MASK	0x03c6
#define	DAC_R_INDEX	0x03c7
#define	DAC_W_INDEX	0x03c8
#define	DAC_DATA	0x03c9
#define	DISP_STAT	0x02e8
#define	H_TOTAL		0x02e8
#define	H_DISP		0x06e8
#define	H_SYNC_STRT	0x0ae8
#define	H_SYNC_WID	0x0ee8
#define	V_TOTAL		0x12e8
#define	V_DISP		0x16e8
#define	V_SYNC_STRT	0x1ae8
#define	V_SYNC_WID	0x1ee8
#define	DISP_CNTL	0x22e8
#define	ADVFUNC_CNTL	0x4ae8
#define	SUBSYS_STAT	0x42e8
#define	SUBSYS_CNTL	0x42e8
#define	ROM_PAGE_SEL	0x46e8
#define	CUR_Y		0x82e8
#define	CUR_X		0x86e8
#define	DESTY_AXSTP	0x8ae8
#define	DESTX_DIASTP	0x8ee8
#define	ERR_TERM	0x92e8
#define	MAJ_AXIS_PCNT	0x96e8
#define	GP_STAT		0x9ae8
#define	CMD		0x9ae8
#define	SHORT_STROKE	0x9ee8
#define	BKGD_COLOR	0xa2e8
#define	FRGD_COLOR	0xa6e8
#define	WRT_MASK	0xaae8
#define	RD_MASK		0xaee8
#define	COLOR_CMP	0xb2e8
#define	BKGD_MIX	0xb6e8
#define	FRGD_MIX	0xbae8
#define	MULTIFUNC_CNTL	0xbee8
#define	MIN_AXIS_PCNT	0x0000
#define	SCISSORS_T	0x1000
#define	SCISSORS_L	0x2000
#define	SCISSORS_B	0x3000
#define	SCISSORS_R	0x4000
#define	MEM_CNTL	0x5000
#define	PATTERN_L	0x8000
#define	PATTERN_H	0x9000
#define	PIX_CNTL	0xa000
#define CONTROL_MISC2	0xd000
#define	PIX_TRANS	0xe2e8

/* Advanced Function Control Regsiter */
#define	CLKSEL		0x0004
#define	DISABPASSTHRU	0x0001

/* Graphics Processor Status Register */

#define GPNSLOT		13

#define GPBUSY_1	0x0080
#define GPBUSY_2	0x0040
#define GPBUSY_3	0x0020
#define GPBUSY_4	0x0010
#define GPBUSY_5	0x0008
#define GPBUSY_6	0x0004
#define GPBUSY_7	0x0002
#define GPBUSY_8	0x0001
#define GPBUSY_9	0x8000
#define GPBUSY_10	0x4000
#define GPBUSY_11	0x2000
#define GPBUSY_12	0x1000
#define GPBUSY_13	0x0800

#define GPEMPTY		0x0400
#define	GPBUSY		0x0200
#define	DATDRDY		0x0100

/* Command Register */
#define	CMD_NOP		0x0000
#define	CMD_LINE	0x2000
#define	CMD_RECT	0x4000
#define	CMD_RECTV1	0x6000
#define	CMD_RECTV2	0x8000
#define	CMD_LINEAF	0xa000
#define	CMD_BITBLT	0xc000
#define CMD_PATBLT	0xe000
#define	CMD_OP_MSK	0xe000
#define	BYTSEQ		0x1000
#define _32BITNOPAD	0x0600
#define _32BIT		0x0400
#define	_16BIT		0x0200
#define _8BIT		0x0000
#define	PCDATA		0x0100
#define	INC_Y		0x0080
#define	YMAJAXIS	0x0040
#define	INC_X		0x0020
#define	DRAW		0x0010
#define	LINETYPE	0x0008
#define	LASTPIX		0x0004	    /* Draw last pixel in line */
#define	PLANAR		0x0002
#define	WRTDATA		0x0001

/* Background Mix Register */
#define	BSS_BKGDCOL	0x0000
#define	BSS_FRGDCOL	0x0020
#define	BSS_PCDATA	0x0040
#define	BSS_BITBLT	0x0060

/* Foreground Mix Register */
#define	FSS_BKGDCOL	0x0000
#define	FSS_FRGDCOL	0x0020
#define	FSS_PCDATA	0x0040
#define	FSS_BITBLT	0x0060

/* The Mixes */
#define	MIX_MASK			0x001f

#define	MIX_NOT_DST			0x0000
#define	MIX_0				0x0001
#define	MIX_1				0x0002
#define	MIX_DST				0x0003
#define	MIX_NOT_SRC			0x0004
#define	MIX_XOR				0x0005
#define	MIX_XNOR			0x0006
#define	MIX_SRC				0x0007
#define	MIX_NAND			0x0008
#define	MIX_NOT_SRC_OR_DST		0x0009
#define	MIX_SRC_OR_NOT_DST		0x000a
#define	MIX_OR				0x000b
#define	MIX_AND				0x000c
#define	MIX_SRC_AND_NOT_DST		0x000d
#define	MIX_NOT_SRC_AND_DST		0x000e
#define	MIX_NOR				0x000f

#define	MIX_MIN				0x0010
#define	MIX_DST_MINUS_SRC		0x0011
#define	MIX_SRC_MINUS_DST		0x0012
#define	MIX_PLUS			0x0013
#define	MIX_MAX				0x0014
#define	MIX_HALF__DST_MINUS_SRC		0x0015
#define	MIX_HALF__SRC_MINUS_DST		0x0016
#define	MIX_AVERAGE			0x0017
#define	MIX_DST_MINUS_SRC_SAT		0x0018
#define	MIX_SRC_MINUS_DST_SAT		0x001a
#define	MIX_HALF__DST_MINUS_SRC_SAT	0x001c
#define	MIX_HALF__SRC_MINUS_DST_SAT	0x001e
#define	MIX_AVERAGE_SAT			0x001f

/* Pixel Control Register */
#define	MIXSEL_FRGDMIX	0x0000
#define	MIXSEL_PATT	0x0040
#define	MIXSEL_EXPPC	0x0080
#define	MIXSEL_EXPBLT	0x00c0
#define COLCMPOP_F	0x0000
#define COLCMPOP_T	0x0008
#define COLCMPOP_GE	0x0010
#define COLCMPOP_LT	0x0018
#define COLCMPOP_NE	0x0020
#define COLCMPOP_EQ	0x0028
#define COLCMPOP_LE	0x0030
#define COLCMPOP_GT	0x0038
#define	PLANEMODE	0x0004

#define FIFO_SLOTS	13

#define GPSLOT(n)   (1 << ((n) > 8 ? (15 - ((n) - 9)) : (8 - (n))))

/* Wait for n slots to become available */
#if 0
#define _s3WaitSlots(s3,n) { \
    DRAW_DEBUG ((DEBUG_CRTC, "_s3WaitSlots 0x%x %d", (s3)->cmd_gp_stat, n)); \
    while (((s3)->cmd_gp_stat & GPSLOT(n)) != 0); \
    DRAW_DEBUG ((DEBUG_CRTC, "  s3 0x%x %d slots ready", (s3)->cmd_gp_stat, n)); \
}
#else
/* let PCI retries solve this problem */
#define _s3WaitSlots(s3,n)
#endif

/* Wait until queue is empty */
#define _s3WaitEmpty(s3) { \
    DRAW_DEBUG ((DEBUG_CRTC, "_s3WaitEmpty 0x%x", (s3)->cmd_gp_stat)); \
    while (!((s3)->cmd_gp_stat & GPEMPTY)) ; \
    DRAW_DEBUG ((DEBUG_CRTC, "  s3 empty")); \
}

/* Wait until GP is idle and queue is empty */
#define	_s3WaitIdleEmpty(s3) { \
    DRAW_DEBUG ((DEBUG_CRTC, "_s3WaitIdleEmpty 0x%x", (s3)->cmd_gp_stat)); \
    while (((s3)->cmd_gp_stat & (GPBUSY|GPEMPTY)) != GPEMPTY) ; \
    DRAW_DEBUG ((DEBUG_CRTC, "  s3 idle empty")); \
}

/* Wait until GP is idle */
#define _s3WaitIdle(s3) { \
    DRAW_DEBUG ((DEBUG_CRTC, "_s3WaitIdle 0x%x", (s3)->cmd_gp_stat)); \
    while ((s3)->cmd_gp_stat & GPBUSY) ; \
    DRAW_DEBUG ((DEBUG_CRTC, "  s3 idle")); \
}

typedef struct _s3Cursor {
    int		width, height;
    int		xhot, yhot;
    Bool	has_cursor;
    CursorPtr	pCursor;
    Pixel	source, mask;
} S3Cursor;

typedef struct _s3PatternCache {
    int		id;
    int		x, y;
} S3PatternCache;

typedef struct _s3Patterns {
    S3PatternCache	*cache;
    int			ncache;
    int			last_used;
    int			last_id;
} S3Patterns;

typedef struct _crtc {
    CARD8   h_total_0_7;			    /* CR0 */
    CARD8   h_display_end_0_7;			    /* CR1 */
    CARD8   h_blank_start_0_7;			    /* CR2 */
    union {
	struct {
	    CARD8   _h_blank_end_0_4  	    : 5;
	    CARD8   _display_skew	    : 2;
	    CARD8			    : 1;
	} _h_blank_end_s;
	CARD8   __h_blank_end;			    /* CR3 */
    } _h_blank_end_u;
#define h_blank_end_0_4 _h_blank_end_u._h_blank_end_s._h_blank_end_0_4
#define display_skew	_h_blank_end_u._h_blank_end_s._display_skew
#define _h_blank_end	_h_blank_end_u.__h_blank_end
    
    CARD8   h_sync_start_0_7;			    /* CR4 */
    
    union {
	struct {
	    CARD8   _h_sync_end_0_4    	    : 5;
	    CARD8   _horizontal_skew	    : 2;
	    CARD8   _h_blank_end_5	    : 1;
	} _h_sync_end_s;
        CARD8   __h_sync_end;			    /* CR5 */
    } _h_sync_end_u;

#define h_sync_end_0_4	_h_sync_end_u._h_sync_end_s._h_sync_end_0_4
#define horizontal_skew	_h_sync_end_u._h_sync_end_s._horizontal_skew
#define h_blank_end_5	_h_sync_end_u._h_sync_end_s._h_blank_end_5
#define _h_sync_end	_h_sync_end_u.__h_sync_end
    
    CARD8   v_total_0_7;			    /* CR6 */
    
    union {
	struct {
	    CARD8   _v_total_8		    : 1;
	    CARD8   _v_display_end_8	    : 1;
	    CARD8   _v_retrace_start_8	    : 1;
	    CARD8   _v_blank_start_8	    : 1;
	    CARD8   _line_compare_8	    : 1;
	    CARD8   _v_total_9		    : 1;
	    CARD8   _v_display_end_9	    : 1;
	    CARD8   _v_retrace_start_9	    : 1;
	} _crtc_overflow_s;
	CARD8	_crtc_overflow;			    /* CR7 */
    } _crtc_overflow_u;

#define v_total_8	    _crtc_overflow_u._crtc_overflow_s._v_total_8
#define v_display_end_8	    _crtc_overflow_u._crtc_overflow_s._v_display_end_8
#define v_retrace_start_8   _crtc_overflow_u._crtc_overflow_s._v_retrace_start_8
#define v_blank_start_8	    _crtc_overflow_u._crtc_overflow_s._v_blank_start_8
#define line_compare_8	    _crtc_overflow_u._crtc_overflow_s._line_compare_8
#define v_total_9	    _crtc_overflow_u._crtc_overflow_s._v_total_9
#define v_display_end_9	    _crtc_overflow_u._crtc_overflow_s._v_display_end_9
#define v_retrace_start_9   _crtc_overflow_u._crtc_overflow_s._v_retrace_start_9
#define crtc_overflow	    _crtc_overflow_u._crtc_overflow

    CARD8   preset_row_scan;			    /* CR8 (unused) */

    union {
	struct {
	    CARD8   _max_scan_line	    : 5;
	    CARD8   _v_blank_start_9	    : 1;
	    CARD8   _line_compare_9	    : 1;
	    CARD8   _double_scan		    : 1;
	} _max_scan_line_s;
	CARD8   __max_scan_line;		    /* CR9 */
    } _max_scan_line_u;

#define max_scan_line	_max_scan_line_u._max_scan_line_s._max_scan_line
#define v_blank_start_9	_max_scan_line_u._max_scan_line_s._v_blank_start_9
#define line_compare_9	_max_scan_line_u._max_scan_line_s._line_compare_9
#define double_scan	_max_scan_line_u._max_scan_line_s._double_scan
#define _max_scan_line	_max_scan_line_u.__max_scan_line
    
    CARD8   cursor_start;
    CARD8   cursor_end;

    CARD8   start_address_8_15;			    /* CRC */
    CARD8   start_address_0_7;			    /* CRD */

    CARD8   cursor_loc_high;
    CARD8   cursor_loc_low;

    CARD8   v_retrace_start_0_7;		    /* CR10 */
    union {
	struct {
	    CARD8   _v_retrace_end_0_3	    : 4;
	    CARD8   _clear_v_retrace_int	    : 1;
	    CARD8   _disable_v_retrace_int   : 1;
	    CARD8   _refresh_cycle_select    : 1;
	    CARD8   _lock_crtc		    : 1;
	} _v_retrace_end_s;
	CARD8   __v_retrace_end;		    /* CR11 */
    } _v_retrace_end_u;
    
#define v_retrace_end_0_3	_v_retrace_end_u._v_retrace_end_s._v_retrace_end_0_3
#define clear_v_retrace_int	_v_retrace_end_u._v_retrace_end_s._clear_v_retrace_int
#define disable_v_retrace_int	_v_retrace_end_u._v_retrace_end_s._disable_v_retrace_int
#define refresh_cycle_select	_v_retrace_end_u._v_retrace_end_s._refresh_cycle_select
#define lock_crtc		_v_retrace_end_u._v_retrace_end_s._lock_crtc
#define _v_retrace_end		_v_retrace_end_u.__v_retrace_end
    
    CARD8   v_display_end_0_7;			    /* CR12 */
    
    CARD8   screen_off_0_7;			    /* CR13 */
    
    union {
	struct {
	    CARD8   _underline_location	    : 5;
	    CARD8   _count_by_four	    : 1;
	    CARD8   _doubleword_mode	    : 1;
	    CARD8			    : 1;
	} _underline_location_s;
	CARD8   __underline_location;		    /* CR14 (unused) */
    } _underline_location_u;

#define underline_location  _underline_location_u._underline_location_s._underline_location
#define count_by_four	    _underline_location_u._underline_location_s._count_by_four
#define doubleword_mode	    _underline_location_u._underline_location_s._doubleword_mode
#define _underline_location _underline_location_u.__underline_location
    
    CARD8   v_blank_start_0_7;			    /* CR15 */
    CARD8   v_blank_end_0_7;			    /* CR16 */

    union {
	struct {
	    CARD8   _two_bk_cga		    : 1;
	    CARD8   _four_bk_cga	    : 1;
	    CARD8   _v_total_double	    : 1;
	    CARD8   _word_mode		    : 1;
	    CARD8			    : 1;
	    CARD8   _address_wrap	    : 1;
	    CARD8   _byte_mode		    : 1;
	    CARD8   _hardware_reset	    : 1;
	} _crtc_mode_s;
	CARD8   _crtc_mode;			    /* CR17 (unused) */
    } _crtc_mode_u;

    CARD8   line_compare_0_7;			    /* CR18 (unused) */
    
    union {
	struct {
	    CARD8   _enable_base_offset	    : 1;
	    CARD8   _enable_two_page	    : 1;
	    CARD8   _enable_vga_16_bit	    : 1;
	    CARD8   _enhanced_mode_mapping   : 1;
	    CARD8   _old_display_start	    : 2;
	    CARD8   _enable_high_speed_text  : 1;
	    CARD8			    : 1;
	} _memory_configuration_s;
	CARD8	_memory_configuration;		    /* CR31 (unused) */
    } _memory_configuration_u;

#define memory_configuration	_memory_configuration_u._memory_configuration
#define enable_base_offset	_memory_configuration_u._memory_configuration_s._enable_base_offset
#define enable_two_page		_memory_configuration_u._memory_configuration_s._enable_two_page
#define enable_vga_16_bit	_memory_configuration_u._memory_configuration_s._enable_vga_16_bit
#define enhanved_mode_mapping	_memory_configuration_u._memory_configuration_s._enhanced_mode_mapping
#define old_display_start	_memory_configuration_u._memory_configuration_s._old_display_start
#define enable_high_speed_text	_memory_configuration_u._memory_configuration_s._enable_high_speed_text
    
    union {
	struct {
	    CARD8   _alt_refresh_count	    : 2;
	    CARD8   _enable_alt_refresh	    : 1;
	    CARD8   _enable_top_memory	    : 1;
	    CARD8   _enable_256_or_more	    : 1;
	    CARD8   _high_speed_text	    : 1;
	    CARD8			    : 1;
	    CARD8   _pci_burst_disabled	    : 1;
	} _misc_1_s;
	CARD8	_misc_1;				    /* CR3A */
    } _misc_1_u;
#define misc_1	_misc_1_u._misc_1
#define alt_refresh_count   _misc_1_u._misc_1_s._alt_refresh_count
#define enable_alt_refresh  _misc_1_u._misc_1_s._enable_alt_refresh
#define enable_top_memory   _misc_1_u._misc_1_s._enable_top_memory
#define enable_256_or_more  _misc_1_u._misc_1_s._enable_256_or_more
#define high_speed_text	    _misc_1_u._misc_1_s._high_speed_text
#define pci_burst_disabled  _misc_1_u._misc_1_s._pci_burst_disabled
    
    CARD8   h_start_fifo_fetch_0_7;		    /* CR3B */
    
    union {
	struct {
	    CARD8			    : 5;
	    CARD8 interlace		    : 1;
	    CARD8			    : 2;
	} _mode_control_s;
	CARD8	_mode_control;			    /* CR42 */
    } _mode_control_u;
    
#define mode_control	_mode_control_u._mode_control
    
    union {
	struct {
	    CARD8			    : 2;
	    CARD8   _old_screen_off_8	    : 1;
	    CARD8			    : 4;
	    CARD8   h_counter_double_mode   : 1;
	} _extended_mode_s;
	CARD8	_extended_mode;			    /* CR43 (unused) */
    } _extended_mode_u;
    
#define extended_mode	    _extended_mode_u._extended_mode
#define old_screen_off_8    _extended_mode_u._extended_mode_s._old_screen_off_8
    
    union {
	struct {
	    CARD8   _hardware_cursor_enable : 1;
	    CARD8			    : 3;
	    CARD8   _hardware_cursor_right  : 1;
	    CARD8			    : 3;
	} _hardware_cursor_mode_s;
	CARD8	_hardware_cursor_mode;		    /* CR45 */
    } _hardware_cursor_mode_u;

#define hardware_cursor_mode	_hardware_cursor_mode_u._hardware_cursor_mode
#define hardware_cursor_enable	_hardware_cursor_mode_u._hardware_cursor_mode_s._hardware_cursor_enable

    CARD8   cursor_address_8_15;		    /* CR4C */
    CARD8   cursor_address_0_7;			    /* CR4D */
    
    union {
	struct {
	    CARD8   _ge_screen_width_2	    : 1;
	    CARD8			    : 3;
	    CARD8   _pixel_length	    : 2;
	    CARD8   _ge_screen_width_0_1	    : 2;
	} _extended_system_control_1_s;
	CARD8	_extended_system_control_1;	    /* CR50 */
    } _extended_system_control_1_u;
#define ge_screen_width_2	    _extended_system_control_1_u._extended_system_control_1_s._ge_screen_width_2
#define pixel_length		    _extended_system_control_1_u._extended_system_control_1_s._pixel_length
#define ge_screen_width_0_1	    _extended_system_control_1_u._extended_system_control_1_s._ge_screen_width_0_1
#define extended_system_control_1   _extended_system_control_1_u._extended_system_control_1

    union {
	struct {
	    CARD8			    : 4;
	    CARD8   _screen_off_8_9	    : 2;
	    CARD8			    : 2;
	} _extended_system_control_2_s;
	CARD8	_extended_system_control_2;	    /* CR51 */
    } _extended_system_control_2_u;
#define extended_system_control_2   _extended_system_control_2_u._extended_system_control_2
#define screen_off_8_9		    _extended_system_control_2_u._extended_system_control_2_s._screen_off_8_9

    union {
	struct {
	    CARD8			    : 1;
	    CARD8   big_endian_linear	    : 2;
	    CARD8   mmio_select		    : 2;
	    CARD8   mmio_window		    : 1;
	    CARD8   swap_nibbles	    : 1;
	    CARD8			    : 1;
	} _extended_memory_control_s;
	CARD8	_extended_memory_control;	    /* CR53 */
    } _extended_memory_control_u;
#define extended_memory_control	_extended_memory_control_u._extended_memory_control
    
    union {
	struct {
	    CARD8			    : 2;
	    CARD8   _enable_gir		    : 1;
	    CARD8			    : 1;
	    CARD8   _hardware_cursor_ms_x11 : 1;
	    CARD8			    : 2;
	    CARD8   _tri_state_off_vclk	    : 1;
	} _extended_ramdac_control_s;
	CARD8	_extended_ramdac_control;	    /* CR55 */
    } _extended_ramdac_control_u;
#define extended_ramdac_control	_extended_ramdac_control_u._extended_ramdac_control
#define hardware_cursor_ms_x11	_extended_ramdac_control_u._extended_ramdac_control_s._hardware_cursor_ms_x11

    
    union {
	struct {
	    CARD8   _h_total_8		    : 1;
	    CARD8   _h_display_end_8	    : 1;
	    CARD8   _h_blank_start_8	    : 1;
	    CARD8   _h_blank_extend	    : 1;    /* extend h_blank by 64 */
	    CARD8   _h_sync_start_8	    : 1;
	    CARD8   _h_sync_extend 	    : 1;    /* extend h_sync by 32 */
	    CARD8   _h_start_fifo_fetch_8    : 1;
	    CARD8			    : 1;
	} _extended_horizontal_overflow_s;
	CARD8	_extended_horizontal_overflow;	    /* CR5D */
    } _extended_horizontal_overflow_u;
#define extended_horizontal_overflow	_extended_horizontal_overflow_u._extended_horizontal_overflow
#define h_total_8			_extended_horizontal_overflow_u._extended_horizontal_overflow_s._h_total_8
#define h_display_end_8			_extended_horizontal_overflow_u._extended_horizontal_overflow_s._h_display_end_8
#define h_blank_start_8			_extended_horizontal_overflow_u._extended_horizontal_overflow_s._h_blank_start_8
#define h_blank_extend			_extended_horizontal_overflow_u._extended_horizontal_overflow_s._h_blank_extend
#define h_sync_start_8			_extended_horizontal_overflow_u._extended_horizontal_overflow_s._h_sync_start_8
#define h_sync_extend			_extended_horizontal_overflow_u._extended_horizontal_overflow_s._h_sync_extend
#define h_start_fifo_fetch_8		_extended_horizontal_overflow_u._extended_horizontal_overflow_s._h_start_fifo_fetch_8
    
    
    union {
	struct {
	    CARD8   _v_total_10		    : 1;
	    CARD8   _v_display_end_10	    : 1;
	    CARD8   _v_blank_start_10	    : 1;
	    CARD8			    : 1;
	    CARD8   _v_retrace_start_10	    : 1;
	    CARD8			    : 1;
	    CARD8   _line_compare_10	    : 1;
	    CARD8			    : 1;
	} _extended_vertical_overflow_s;
	CARD8	_extended_vertical_overflow;	    /* CR5E */
    } _extended_vertical_overflow_u;
#define extended_vertical_overflow  _extended_vertical_overflow_u._extended_vertical_overflow
#define v_total_10		    _extended_vertical_overflow_u._extended_vertical_overflow_s._v_total_10
#define v_display_end_10	    _extended_vertical_overflow_u._extended_vertical_overflow_s._v_display_end_10
#define v_blank_start_10	    _extended_vertical_overflow_u._extended_vertical_overflow_s._v_blank_start_10
#define v_retrace_start_10	    _extended_vertical_overflow_u._extended_vertical_overflow_s._v_retrace_start_10
#define line_compare_10		    _extended_vertical_overflow_u._extended_vertical_overflow_s._line_compare_10
    
    
    CARD8   l_parm_0_7;				    /* CR62 (undocumented) */
    
    union {
	struct {
	    CARD8			    : 3;
	    CARD8   _delay_blank		    : 2;
	    CARD8			    : 3;
	} _extended_misc_control_s;
	CARD8	_extended_misc_control;		    /* CR65 */
    } _extended_misc_control_u;
#define extended_misc_control	_extended_misc_control_u._extended_misc_control
#define delay_blank		_extended_misc_control_u._extended_misc_control_s._delay_blank
    
    union {
	struct {
	    CARD8   _v_clock_phase	    : 1;
	    CARD8			    : 3;
	    CARD8   _color_mode		    : 4;
	} _extended_misc_control_2_s;
	CARD8	_extended_misc_control_2;	    /* CR67 */
    } _extended_misc_control_2_u;
    
#define v_clock_phase		_extended_misc_control_2_u._extended_misc_control_2_s._v_clock_phase
#define color_mode		_extended_misc_control_2_u._extended_misc_control_2_s._color_mode
#define extended_misc_control_2	_extended_misc_control_2_u._extended_misc_control_2


    union {
	struct {
	    CARD8   cas_oe_str		    : 2;
	    CARD8   ras_low		    : 1;
	    CARD8   ras_precharge	    : 1;
	    CARD8			    : 3;
	    CARD8   _memory_bus_size	    : 1;    /* 0 = 32, 1 = 32/64 */
	} _configuration_3_s;
	CARD8	_configuration_3;		    /* CR68 */
    } _configuration_3_u;
#define configuration_3    _configuration_3_u._configuration_3
#define memory_bus_size	    _configuration_3_u._configuration_3_s._memory_bus_size
    
    union {
	struct {
	    CARD8   _start_address_16_19    : 4;
	    CARD8			    : 4;
	} _extended_system_control_3_s;
	CARD8   _extended_system_control_3;	    /* CR69 */
    } _extended_system_control_3_u;
#define extended_system_control_3   _extended_system_control_3_u._extended_system_control_3
#define start_address_16_19	    _extended_system_control_3_u._extended_system_control_3_s._start_address_16_19
    
    CARD8   extended_bios_5;			    /* CR6D */

    union {
	struct {
	    CARD8   dot_clock_vclki	    : 1;    /* testing only */
	    CARD8   vclki_with_vafc	    : 1;    /* feature connector */
	    CARD8			    : 1;
	    CARD8   bpp_24_mode		    : 1;    /* 24 bpp mode */
	    CARD8   alt_color_mode	    : 4;    /* feature connector mode */
	} _extended_sequencer_b_s;
	CARD8   _extended_sequencer_b;		    /* SRB */
    } _extended_sequencer_b_u;
    
#define extended_sequencer_b	_extended_sequencer_b_u._extended_sequencer_b
    
    union extended_sequencer_d_u {
	struct {
	    CARD8   enable_feature  : 1;
	    CARD8   lpb_feature	    : 1;
	    CARD8		    : 2;
	    CARD8   _hsync_control  : 2;
	    CARD8   _vsync_control  : 2;
	} _extended_sequencer_d_s;
	CARD8	_extended_sequencer_d;
    } _extended_sequencer_d_u;

#define extended_sequencer_d	_extended_sequencer_d_u._extended_sequencer_d
#define hsync_control		_extended_sequencer_d_u._extended_sequencer_d_s._hsync_control
#define vsync_control		_extended_sequencer_d_u._extended_sequencer_d_s._vsync_control
    
    union {
	struct {
	    CARD8   _dclk_pll_n		    : 5;
	    CARD8   _dclk_pll_r		    : 2;
	    CARD8			    : 1;
	} _dclk_value_low_s;
	CARD8	_dclk_value_low;		    /* SR12 */
    } _dclk_value_low_u;
    
#define dclk_value_low	_dclk_value_low_u._dclk_value_low
#define dclk_pll_n_trio	_dclk_value_low_u._dclk_value_low_s._dclk_pll_n
#define dclk_pll_r_trio	_dclk_value_low_u._dclk_value_low_s._dclk_pll_r
    
    union {
	struct {
	    CARD8   _dclk_pll_m		    : 7;
	    CARD8			    : 1;
	} _dclk_value_high_s;
	CARD8	_dclk_value_high;		    /* SR13 */
    } _dclk_value_high_u;

#define dclk_value_high	_dclk_value_high_u._dclk_value_high
#define dclk_pll_m_trio	_dclk_value_high_u._dclk_value_high_s._dclk_pll_m
    
    union {
	struct {
	    CARD8   _mfrq_en		    : 1;
	    CARD8   _dfrq_en		    : 1;
	    CARD8   _mclk_out		    : 1;
	    CARD8   _vclk_out		    : 1;
	    CARD8   _dclk_over_2	    : 1;
	    CARD8   _clk_load		    : 1;
	    CARD8   _dclk_invert    	    : 1;
	    CARD8   _ena_2_cycle_write	    : 1;
	} _control_2_s;
	CARD8	_control_2;			    /* SR15 */
    } _control_2_u;

#define control_2	    _control_2_u._control_2
#define mfrq_en		    _control_2_u._control_2_s._mfrq_en
#define dfrq_en		    _control_2_u._control_2_s._dfrq_en
#define mclk_out	    _control_2_u._control_2_s._mclk_out
#define vclk_out	    _control_2_u._control_2_s._vclk_out
#define dclk_over_2	    _control_2_u._control_2_s._dclk_over_2
#define clk_load	    _control_2_u._control_2_s._clk_load
#define dclk_invert	    _control_2_u._control_2_s._dclk_invert
#define ena_2_cycle_write   _control_2_u._control_2_s._ena_2_cycle_write
    
    union {
	struct {
	    CARD8			    : 5;
	    CARD8   _dac_power_down	    : 1;
	    CARD8   _lut_write_control	    : 1;
	    CARD8   _enable_clock_double    : 1;
	} _ramdac_control_s;
	CARD8	_ramdac_control;		    /* SR18 */
    }  _ramdac_control_u;
    
#define ramdac_control	    _ramdac_control_u._ramdac_control
#define enable_clock_double _ramdac_control_u._ramdac_control_s._enable_clock_double
    
    union {
	struct {
	    CARD8   _dclk_pll_n		    : 6;
	    CARD8   _dclk_pll_r		    : 2;
	} _dclk_value_low_s;
	CARD8	_dclk_value_low;		    /* SR36 */
    } _dclk_value_low_savage_u;
    
#define dclk_value_low_savage	_dclk_value_low_savage_u._dclk_value_low
#define dclk_pll_n_savage_0_5	_dclk_value_low_savage_u._dclk_value_low_s._dclk_pll_n
#define dclk_pll_r_savage_0_1	_dclk_value_low_savage_u._dclk_value_low_s._dclk_pll_r
    
    CARD8   dclk_pll_m0_savage_0_7;		    /* SR37 */
    CARD8   dclk_pll_m1_savage_0_7;		    /* SR38 */
    
	struct {
	    CARD8   _dclk_pll_m		    : 8;
	} _dclk_value_high_s_savage;
    
    union {
	struct {
	    CARD8   _dclk_select	    : 1;
	    CARD8			    : 1;
	    CARD8   _dclk_pll_r_2	    : 1;
	    CARD8   _dclk_pll_m_8	    : 1;
	    CARD8   _dclk_pll_n_6	    : 1;
	    CARD8   _pce		    : 1;
	    CARD8   _ccg		    : 1;
	    CARD8   _csp		    : 1;
	} _extended_seq_39_s;
	CARD8	_extended_seq_39;		    /* SR39 */
    } _extended_seq_39_u;
    
#define extended_seq_39		_extended_seq_39_u._extended_seq_39
#define dclk_pll_select_savage	_extended_seq_39_u._extended_seq_39_s._dclk_select
#define dclk_pll_r_savage_2	_extended_seq_39_u._extended_seq_39_s._dclk_pll_r_2
#define dclk_pll_m_savage_8	_extended_seq_39_u._extended_seq_39_s._dclk_pll_m_8
#define dclk_pll_n_savage_6	_extended_seq_39_u._extended_seq_39_s._dclk_pll_n_6
    
    /* computed values */
    CARD16	    ge_screen_pitch;
    CARD8	    bits_per_pixel;
    CARD8	    depth;
    CARD8	    double_pixel_mode;
    CARD16	    pixel_width;
} S3Crtc;

#define crtc_v_total(crtc)    ((crtc)->v_total_0_7 | \
			       ((crtc)->v_total_8 << 8) | \
			       ((crtc)->v_total_9 << 9) | \
			       ((crtc)->v_total_10 << 10))
    
#define crtc_set_v_total(crtc,v) { \
    ((crtc))->v_total_0_7 = (v); \
    ((crtc))->v_total_8 = (v) >> 8; \
    ((crtc))->v_total_9 = (v) >> 9; \
    ((crtc))->v_total_10 = (v) >> 10; \
}
    
#define crtc_v_display_end(crtc) ((crtc)->v_display_end_0_7 | \
				  ((crtc)->v_display_end_8 << 8) | \
				  ((crtc)->v_display_end_9 << 9) | \
				  ((crtc)->v_display_end_10 << 10))

#define crtc_set_v_display_end(crtc,v) {\
    ((crtc))->v_display_end_0_7 = (v); \
    ((crtc))->v_display_end_8 = (v) >> 8; \
    ((crtc))->v_display_end_9 = (v) >> 9; \
    ((crtc))->v_display_end_10 = (v) >> 10; \
}

#define crtc_v_retrace_start(crtc)  ((crtc)->v_retrace_start_0_7 | \
				     ((crtc)->v_retrace_start_8 << 8) | \
				     ((crtc)->v_retrace_start_9 << 9) | \
				     ((crtc)->v_retrace_start_10 << 10))

#define crtc_set_v_retrace_start(crtc,v) {\
    ((crtc))->v_retrace_start_0_7 = (v); \
    ((crtc))->v_retrace_start_8 = (v) >> 8; \
    ((crtc))->v_retrace_start_9 = (v) >> 9; \
    ((crtc))->v_retrace_start_10 = (v) >> 10; \
}

#define crtc_v_blank_start(crtc)  ((crtc)->v_blank_start_0_7 | \
				   ((crtc)->v_blank_start_8 << 8) | \
				   ((crtc)->v_blank_start_9 << 9) | \
				   ((crtc)->v_blank_start_10 << 10))

#define crtc_set_v_blank_start(crtc,v) {\
    ((crtc))->v_blank_start_0_7 = (v); \
    ((crtc))->v_blank_start_8 = (v) >> 8; \
    ((crtc))->v_blank_start_9 = (v) >> 9; \
    ((crtc))->v_blank_start_10 = (v) >> 10; \
}

#define crtc_h_total(crtc) ((crtc)->h_total_0_7 | \
			    ((crtc)->h_total_8 << 8))

#define crtc_set_h_total(crtc,v) {\
    ((crtc))->h_total_0_7 = (v); \
    ((crtc))->h_total_8 = (v) >> 8; \
}

#define crtc_h_display_end(crtc) ((crtc)->h_display_end_0_7 | \
				  ((crtc)->h_display_end_8 << 8))

#define crtc_set_h_display_end(crtc,v) {\
    ((crtc))->h_display_end_0_7 = (v); \
    ((crtc))->h_display_end_8 = (v) >> 8; \
}

#define crtc_h_blank_start(crtc) ((crtc)->h_blank_start_0_7 | \
				  ((crtc)->h_blank_start_8 << 8))

#define crtc_set_h_blank_start(crtc,v) {\
    ((crtc))->h_blank_start_0_7 = (v); \
    ((crtc))->h_blank_start_8 = (v) >> 8; \
}

#define crtc_h_blank_end(crtc)  ((crtc)->h_blank_end_0_4 | \
				 ((crtc)->h_blank_end_5 << 5))

#define crtc_set_h_blank_end(crtc,v) {\
    ((crtc))->h_blank_end_0_4 = (v); \
    ((crtc))->h_blank_end_5 = (v) >> 5; \
}

#define crtc_h_sync_start(crtc) ((crtc)->h_sync_start_0_7 | \
				 ((crtc)->h_sync_start_8 << 8))

#define crtc_set_h_sync_start(crtc,v) {\
    ((crtc))->h_sync_start_0_7 = (v); \
    ((crtc))->h_sync_start_8 = (v) >> 8; \
}

#define crtc_h_sync_end(crtc) ((crtc)->h_sync_end_0_4)

#define crtc_set_h_sync_end(crtc,v) {\
    ((crtc))->h_sync_end_0_4 = (v); \
}

#define crtc_screen_off(crtc)    ((crtc)->screen_off_0_7 | \
				  (((crtc)->screen_off_8_9 ? \
				    ((crtc))->screen_off_8_9 : \
				    ((crtc))->old_screen_off_8) << 8))

#define crtc_set_screen_off(crtc,v) {\
    ((crtc))->screen_off_0_7 = (v); \
    ((crtc))->old_screen_off_8 = 0; \
    ((crtc))->screen_off_8_9 = (v) >> 8; \
}

#define crtc_ge_screen_width(crtc) ((crtc)->ge_screen_width_0_1 | \
				    ((crtc)->ge_screen_width_2 << 2))

#define crtc_set_ge_screen_width(crtc,v) { \
    (crtc)->ge_screen_width_0_1 = (v); \
    (crtc)->ge_screen_width_2 = (v) >> 2; \
}

#define crtc_h_start_fifo_fetch(crtc) ((crtc)->h_start_fifo_fetch_0_7 | \
				       ((crtc)->h_start_fifo_fetch_8 << 8))

#define crtc_set_h_start_fifo_fetch(crtc,v) {\
    (crtc)->h_start_fifo_fetch_0_7 = (v); \
    (crtc)->h_start_fifo_fetch_8 = (v) >> 8; \
}

#define crtc_start_address(crtc)    ((crtc)->start_address_0_7 | \
				     ((crtc)->start_address_8_15 << 8) | \
				     ((crtc)->start_address_16_19 << 16))

#define crtc_set_start_address(crtc,v) {\
    (crtc)->start_address_0_7 = (v); \
    (crtc)->start_address_8_15 = (v) >> 8; \
    (crtc)->start_address_16_19 = (v) >> 16; \
}

#define crtc_line_compare(crtc)    ((crtc)->line_compare_0_7 | \
				    ((crtc)->line_compare_8 << 8) | \
				    ((crtc)->line_compare_9 << 9) | \
				    ((crtc)->line_compare_10 << 10))

#define crtc_set_line_compare(crtc,v) { \
    ((crtc))->line_compare_0_7 = (v); \
    ((crtc))->line_compare_8 = (v) >> 8; \
    ((crtc))->line_compare_9 = (v) >> 9; \
    ((crtc))->line_compare_10 = (v) >> 10; \
}
    

#define GetCrtc(s3,i)	_s3ReadIndexRegister (&(s3)->crt_vga_3d4, (i))
#define PutCrtc(s3,i,v)	_s3WriteIndexRegister (&(s3)->crt_vga_3d4, (i), (v))

#define GetSrtc(s3,i)	_s3ReadIndexRegister (&(s3)->crt_vga_3c4, (i))
#define PutSrtc(s3,i,v)	_s3WriteIndexRegister (&(s3)->crt_vga_3c4, (i), (v))

#define S3_CLOCK_REF	14318	/* KHz */

#define S3_CLOCK(m,n,r)	(S3_CLOCK_REF * ((m) + 2) / (((n) + 2) * (1 << (r))))

#if PLATFORM == 200
#define S3_MAX_CLOCK	80000	/* KHz */
#endif
#if PLATFORM == 300
#define S3_MAX_CLOCK	135000	/* KHz */
#endif

typedef struct _s3Save {
    CARD8		cursor_fg;
    CARD8		cursor_bg;
    CARD8		lock1;
    CARD8		lock2;
    CARD8		locksrtc;
    CARD8		clock_mode;
    CARD32		alt_mix;
    CARD32		write_mask;
    CARD32		fg;
    CARD32		bg;
    S3Crtc		crtc;
} S3Save;

typedef struct _s3CardInfo {
    S3Ptr	s3;		    /* pointer to register structure */
    int		memory;		    /* amount of memory */
    CARD8	*frameBuffer;	    /* pointer to frame buffer */
    CARD8	*registers;	    /* pointer to register map */
    S3Save	save;
    Bool	savage;
    Bool	need_sync;
} S3CardInfo;

typedef struct _s3FbInfo {
    CARD8	*offscreen;	    /* pointer to offscreen area */
    int		offscreen_y;	    /* top y coordinate of offscreen area */
    int		offscreen_x;	    /* top x coordinate of offscreen area */
    int		offscreen_width;    /* width of offscreen area */
    int		offscreen_height;   /* height of offscreen area */
    S3Patterns	patterns;
    CARD32	bitmap_offset;
    int		accel_stride;
    int		accel_bpp;
} S3FBInfo;
    
typedef struct _s3ScreenInfo {
    CARD8	*cursor_base;	    /* pointer to cursor area */
    S3Cursor	cursor;
    S3FBInfo	fb[1];
} S3ScreenInfo;

#define LockS3(s3c)
#define UnlockS3(s3c)

#define getS3CardInfo(kd)   ((S3CardInfo *) ((kd)->card->driver))
#define s3CardInfo(kd)	    S3CardInfo *s3c = getS3CardInfo(kd)

#define getS3ScreenInfo(kd) ((S3ScreenInfo *) ((kd)->screen->driver))
#define s3ScreenInfo(kd)    S3ScreenInfo *s3s = getS3ScreenInfo(kd)

Bool	s3CardInit (KdCardInfo *);
Bool	s3ScreenInit (KdScreenInfo *);
Bool	s3Enable (ScreenPtr pScreen);
void	s3Disable (ScreenPtr pScreen);
void	s3Fini (ScreenPtr pScreen);

Bool	s3CursorInit (ScreenPtr pScreen);
void	s3CursorEnable (ScreenPtr pScreen);
void	s3CursorDisable (ScreenPtr pScreen);
void	s3CursorFini (ScreenPtr pScreen);
void	s3RecolorCursor (ScreenPtr pScreen, int ndef, xColorItem *pdefs);

Bool	s3DrawInit (ScreenPtr pScreen);
void	s3DrawEnable (ScreenPtr pScreen);
void	s3DrawSync (ScreenPtr pScreen);
void	s3DrawDisable (ScreenPtr pScreen);
void	s3DrawFini (ScreenPtr pScreen);

void	s3GetColors (ScreenPtr pScreen, int fb, int ndef, xColorItem *pdefs);
void	s3PutColors (ScreenPtr pScreen, int fb, int ndef, xColorItem *pdefs);

void	S3InitCard (KdCardAttr *attr);

void	s3GetClock (int target, int *Mp, int *Np, int *Rp, int maxM, int maxN, int maxR);

CARD8	_s3ReadIndexRegister (VOL8 *base, CARD8 index);
void	_s3WriteIndexRegister (VOL8 *base, CARD8 index, CARD8 value);

extern KdCardFuncs  s3Funcs;

/*
 * Wait for the begining of the retrace interval
 */

#define S3_RETRACE_LOOP_CHECK if (++_loop_count > 300000) {\
    DRAW_DEBUG ((DEBUG_FAILURE, "S3 wait loop failed at %s:%d", \
		__FILE__, __LINE__)); \
    break; \
}

#define _s3WaitVRetrace(s3) { \
    VOL8  *_status = &s3->crt_vga_status_1; \
    int _loop_count; \
    DRAW_DEBUG ((DEBUG_CRTC, "_s3WaitVRetrace 0x%x", *_status)); \
    _loop_count = 0; \
    while ((*_status & VGA_STATUS_1_VSY) != 0) S3_RETRACE_LOOP_CHECK; \
    _loop_count = 0; \
    while ((*_status & VGA_STATUS_1_VSY) == 0) S3_RETRACE_LOOP_CHECK; \
}
/*
 * Wait for the begining of the retrace interval
 */
#define _s3WaitVRetraceEnd(s3) { \
    VOL8  *_status = &s3->crt_vga_status_1; \
    int _loop_count; \
    DRAW_DEBUG ((DEBUG_CRTC, "_s3WaitVRetraceEnd 0x%x", *_status)); \
    _loop_count = 0; \
    while ((*_status & VGA_STATUS_1_VSY) == 0) S3_RETRACE_LOOP_CHECK; \
    _loop_count = 0; \
    while ((*_status & VGA_STATUS_1_VSY) != 0) S3_RETRACE_LOOP_CHECK; \
}

/*
 * This extension register must contain a magic bit pattern to enable
 * the remaining extended registers
 */

#define _s3UnlockExt(s3)    _s3WriteIndexRegister (&s3->crt_vga_3d4, 0x39, 0xa0)
#define _s3LockExt(s3)	    _s3WriteIndexRegister (&s3->crt_vga_3d4, 0x39, 0x00)

#define S3_CURSOR_WIDTH	    64
#define S3_CURSOR_HEIGHT    64
#define S3_CURSOR_SIZE	    ((S3_CURSOR_WIDTH * S3_CURSOR_HEIGHT + 7) / 8)

#define S3_TILE_SIZE	    8

#endif /* _S3_H_ */
