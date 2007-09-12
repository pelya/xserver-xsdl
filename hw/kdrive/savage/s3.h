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

#ifndef _S3_H_
#define _S3_H_

#include "kdrive.h"
#include "s3reg.h"

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
#if 0
    VOL16	pix_cntl;		/* 8140 */
    VOL16	mult_misc2;		/* 8142 */
#else
    VOL32	pix_cntl_mult_misc2;	/* 8140 */
#endif
    VOL32	mult_misc_read_sel;	/* 8144 */
    VOL32	alt_pcnt;		/* 8148 min_axis_pcnt, maj_axis_pcnt */
    VOL8	_pad3a[0x1c];		/* 814c */
    VOL32	global_bitmap_1;	/* 8168 */
    VOL32	global_bitmap_2;	/* 816c */
    VOL32	primary_bitmap_1;	/* 8170 */
    VOL32	primary_bitmap_2;	/* 8174 */
    VOL32	secondary_bitmap_1;	/* 8178 */
    VOL32	secondary_bitmap_2;	/* 817c */
    VOL32	primary_stream_control;	/* 8180 */
    VOL32	chroma_key_control;	/* 8184 */
    VOL32	genlocking_control;	/* 8188 */
    VOL8	_pad3b[0x4];		/* 818c */
    VOL32	secondary_stream_control;   /* 8190 */
    VOL32	chroma_key_upper_bound;	/* 8194 */
    VOL32	secondary_stream_h_scale;   /* 8198 */
    VOL32	color_adjustment;	/* 819c */
    VOL32	blend_control;		/* 81a0 */
    VOL8	_pad3c[0x1c];		/* 81a4 */
    VOL32	primary_stream_addr_0;	/* 81c0 */
    VOL32	primary_stream_addr_1;	/* 81c4 */
    VOL32	primary_stream_stride;	/* 81c8 */
    VOL32       secondary_stream_mbuf;	/* 81cc */
    VOL32       secondary_stream_addr_0;/* 81d0 */
    VOL32       secondary_stream_addr_1;/* 81d4 */
    VOL32       secondary_stream_stride;/* 81d8 */
    VOL8	_pad81dc[4];		/* 81dc */
    VOL32       secondary_stream_vscale;/* 81e0 */
    VOL32       secondary_stream_vinit;	/* 81e4 */
    VOL32       secondary_stream_scount;/* 81e8 */
    VOL32       streams_fifo;		/* 81ec */
    VOL32       primary_stream_xy;	/* 81f0 */
    VOL32       primary_stream_size;	/* 81f4 */
    VOL32	secondary_stream_xy;	/* 81f8 */
    VOL32	secondary_stream_size;	/* 81fc */
    VOL8	_pad8200[0xe8];		/* 8200 */
    VOL32	cur_y;			/* 82e8 */
    VOL8	_pad4[0x14];		/* 82ec */
    VOL32	primary_stream_mem;	/* 8300 */
    VOL32	secondary_stream_mem;	/* 8304 */
    VOL8	_pad8308[0xD2];		/* 8308 */
    VOL8	input_status_1;		/* 83da */
    VOL8	_pad83db[0x131];	/* 83db */
    VOL32	adv_func_cntl;		/* 850c */
    VOL8	_pad8510[0x5dd8];	/* 8510 */
    VOL32	pix_trans;		/* e2e8 */
    VOL8	_pade2ec[0x3a92c];	/*  e2ec */
    VOL32	cmd_overflow_buf_ptr;	/* 48c18 */
    VOL8	_pad48c1c[0x8];		/* 48c1c */
    VOL32	bci_power_management;	/* 48c24 */
    VOL8	_pad48c28[0x38];	/* 48c28 */
    VOL32	alt_status_0;		/* 48c60 */
    VOL32	alt_status_1;		/* 48c64 */
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

/* Multifunction Control Misc 8144 */
#define MISC_DST_BA_0	(0x0 << 0)
#define MISC_DST_BA_1	(0x1 << 0)
#define MISC_DST_BA_2	(0x2 << 0)
#define MISC_DST_BA_3	(0x3 << 0)
#define MISC_SRC_BA_0	(0x0 << 2)
#define MISC_SRC_BA_1	(0x1 << 2)
#define MISC_SRC_BA_2	(0x2 << 2)
#define MISC_SRC_BA_3	(0x3 << 2)
#define MISC_RSF	(1 << 4)
#define MISC_EXT_CLIP	(1 << 5)
#define MISC_SRC_NE	(1 << 7)
#define MISC_ENB_CMP	(1 << 8)
#define MISC_32B	(1 << 9)
#define MISC_DC		(1 << 11)
#define MISC_INDEX_E	(0xe << 12)

#define S3_SAVAGE4_SLOTS    0x0001ffff
#define S3_SAVAGE4_2DI	    0x00800000

#define _s3WaitLoop(s3,mask,value){ \
    int	__loop = 1000000; \
    while (((s3)->alt_status_0 & (mask)) != (value)) \
	if (--__loop == 0) { \
	    ErrorF ("savage wait loop failed 0x%x\n", s3->alt_status_0); \
	    break; \
	} \
}

#define S3_SAVAGE4_ROOM	    10

#define _s3WaitSlots(s3,n) { \
    int __loop = 1000000; \
    while (((s3)->alt_status_0 & S3_SAVAGE4_SLOTS) >= S3_SAVAGE4_ROOM-(n)) \
	if (--__loop == 0) { \
	    ErrorF ("savage wait loop failed 0x%x\n", s3->alt_status_0); \
	    break; \
	} \
}
    
#define _s3WaitEmpty(s3)	_s3WaitLoop(s3,S3_SAVAGE4_SLOTS, 0)
#define _s3WaitIdleEmpty(s3)	_s3WaitLoop(s3,S3_SAVAGE4_SLOTS|S3_SAVAGE4_2DI, S3_SAVAGE4_2DI)
#define _s3WaitIdle(s3)		_s3WaitLoop(s3,S3_SAVAGE4_2DI, S3_SAVAGE4_2DI)

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

#define S3_CLOCK_REF	14318	/* KHz */

#define S3_CLOCK(m,n,r)	((S3_CLOCK_REF * ((m) + 2)) / (((n) + 2) * (1 << (r))))

#define S3_MAX_CLOCK	150000	/* KHz */

typedef struct _s3Timing {
    /* label */
    int		horizontal;
    int		vertical;
    int		rate;
    /* horizontal timing */
    int		hfp;	    /* front porch */
    int		hbp;	    /* back porch */
    int		hblank;	    /* blanking */
    /* vertical timing */
    int		vfp;	    /* front porch */
    int		vbp;	    /* back porch */
    int		vblank;	    /* blanking */
    /* clock values */
    int		dac_m;
    int		dac_n;
    int		dac_r;
} S3Timing;

#define S3_TEXT_SAVE	(64*1024)

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
    CARD32		global_bitmap_1;
    CARD32		global_bitmap_2;
    CARD32		adv_func_cntl;
    CARD32		primary_bitmap_1;
    CARD32		primary_bitmap_2;
    CARD32		secondary_bitmap_1;
    CARD32		secondary_bitmap_2;
    CARD32		primary_stream_control;
    CARD32		blend_control;
    CARD32		primary_stream_addr_0;
    CARD32		primary_stream_addr_1;
    CARD32		primary_stream_stride;
    CARD32		primary_stream_xy;
    CARD32		primary_stream_size;
    CARD32		primary_stream_mem;
    CARD32		secondary_stream_xy;
    CARD32		secondary_stream_size;
    CARD32		streams_fifo;
    CARD8		text_save[S3_TEXT_SAVE];
} S3Save;

typedef struct _s3CardInfo {
    S3Ptr	s3;		    /* pointer to register structure */
    int		memory;		    /* amount of memory */
    CARD8	*frameBuffer;	    /* pointer to frame buffer */
    CARD8	*registers;	    /* pointer to register map */
    S3Vga	s3vga;
    S3Save	save;
    Bool	need_sync;
    Bool	bios_initialized;   /* whether the bios has been run */
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
    CARD32	chroma_key;
} S3FBInfo;
    
typedef struct _s3ScreenInfo {
    CARD8	*cursor_base;	    /* pointer to cursor area */
    S3Cursor	cursor;
    Bool	managing_border;
    Bool	use_streams;
    int		primary_depth;
    int		current_ma;
    CARD32	border_pixel;
    S3FBInfo	fb[KD_MAX_FB];
    RegionRec	region[KD_MAX_FB];
    int		fbmap[KD_MAX_FB+1];   /* map from fb to stream */
} S3ScreenInfo;

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

void	s3DumbCopyWindow (WindowPtr pWin, DDXPointRec ptOldOrg, RegionPtr prgnSrc);
    
Bool	s3DrawInit (ScreenPtr pScreen);
void	s3DrawEnable (ScreenPtr pScreen);
void	s3DrawSync (ScreenPtr pScreen);
void	s3DrawDisable (ScreenPtr pScreen);
void	s3DrawFini (ScreenPtr pScreen);

void	s3GetColors (ScreenPtr pScreen, int fb, int ndef, xColorItem *pdefs);
void	s3PutColors (ScreenPtr pScreen, int fb, int ndef, xColorItem *pdefs);

void	S3InitCard (KdCardAttr *attr);

void	s3GetClock (int target, int *Mp, int *Np, int *Rp, int maxM, int maxN, int maxR, int minVco);

extern KdCardFuncs  s3Funcs;

/*
 * Wait for the begining of the retrace interval
 */

#define S3_RETRACE_LOOP_CHECK if (++_loop_count > 300000) {\
    DRAW_DEBUG ((DEBUG_FAILURE, "S3 wait loop failed at %s:%d", \
		__FILE__, __LINE__)); \
    break; \
}

#define DRAW_DEBUG(a)

#define _s3WaitVRetrace(s3vga) { \
    int _loop_count; \
    _loop_count = 0; \
    while (s3GetImm(s3vga, s3_vertical_sync_active) != 0) S3_RETRACE_LOOP_CHECK; \
    _loop_count = 0; \
    while (s3GetImm(s3vga, s3_vertical_sync_active) == 0) S3_RETRACE_LOOP_CHECK; \
}
#define _s3WaitVRetraceFast(s3) { \
    int _loop_count; \
    _loop_count = 0; \
    while (s3->input_status_1 & 8) S3_RETRACE_LOOP_CHECK; \
    _loop_count = 0; \
    while ((s3->input_status_1 & 8) == 0) S3_RETRACE_LOOP_CHECK; \
}
/*
 * Wait for the begining of the retrace interval
 */
#define _s3WaitVRetraceEnd(s3vga) { \
    int _loop_count; \
    _loop_count = 0; \
    while (s3GetImm(s3vga, s3_vertical_sync_active) == 0) S3_RETRACE_LOOP_CHECK; \
    _loop_count = 0; \
    while (s3GetImm(s3vga, s3_vertical_sync_active) != 0) S3_RETRACE_LOOP_CHECK; \
}

#define S3_CURSOR_WIDTH	    64
#define S3_CURSOR_HEIGHT    64
#define S3_CURSOR_SIZE	    ((S3_CURSOR_WIDTH * S3_CURSOR_HEIGHT + 7) / 8)

#define S3_TILE_SIZE	    8

#endif /* _S3_H_ */
