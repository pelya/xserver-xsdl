/*
 * $Id$
 *
 * Copyright © 2003 Eric Anholt
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Eric Anholt not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Eric Anholt makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * ERIC ANHOLT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ERIC ANHOLT BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
/* $Header$ */

/* The Radeon register definitions are almost all the same for r128 */
#define RADEON_REG_RBBM_STATUS			0x0e40
# define RADEON_RBBM_FIFOCNT_MASK		0x007f
# define RADEON_RBBM_ACTIVE			(1 << 31)
#define RADEON_REG_SRC_PITCH_OFFSET		0x1428
#define RADEON_REG_DST_PITCH_OFFSET		0x142c
#define RADEON_REG_SRC_Y_X			0x1434
#define RADEON_REG_DST_Y_X			0x1438
#define RADEON_REG_DST_HEIGHT_WIDTH		0x143c
#define RADEON_REG_DP_GUI_MASTER_CNTL		0x146c
#define RADEON_REG_DP_BRUSH_FRGD_CLR		0x147c
# define RADEON_GMC_SRC_PITCH_OFFSET_CNTL	(1    <<  0)
# define RADEON_GMC_DST_PITCH_OFFSET_CNTL	(1    <<  1)
# define RADEON_GMC_BRUSH_SOLID_COLOR		(13   <<  4)
# define RADEON_GMC_BRUSH_NONE			(15   <<  4)
# define RADEON_GMC_SRC_DATATYPE_COLOR		(3    << 12)
# define RADEON_DP_SRC_SOURCE_MEMORY		(2    << 24)
# define RADEON_GMC_CLR_CMP_CNTL_DIS		(1    << 28)
# define RADEON_GMC_AUX_CLIP_DIS		(1    << 29)
#define RADEON_REG_DP_CNTL			0x16c0
# define RADEON_DST_X_LEFT_TO_RIGHT		(1 <<  0)
# define RADEON_DST_Y_TOP_TO_BOTTOM		(1 <<  1)
#define RADEON_REG_DST_WIDTH_HEIGHT		0x1598
#define RADEON_REG_AUX_SC_CNTL			0x1660
#define RADEON_REG_DP_WRITE_MASK		0x16cc
#define RADEON_REG_DEFAULT_OFFSET		0x16e0
#define RADEON_REG_DEFAULT_PITCH		0x16e4
#define RADEON_REG_DEFAULT_SC_BOTTOM_RIGHT	0x16e8
# define RADEON_DEFAULT_SC_RIGHT_MAX		(0x1fff <<  0)
# define RADEON_DEFAULT_SC_BOTTOM_MAX		(0x1fff << 16)
#define RADEON_REG_SC_TOP_LEFT                  0x16ec
#define RADEON_REG_SC_BOTTOM_RIGHT		0x16f0
#define RADEON_REG_RB2D_DSTCACHE_CTLSTAT	0x342c
# define RADEON_RB2D_DC_FLUSH			(3 << 0)
# define RADEON_RB2D_DC_FREE			(3 << 2)
# define RADEON_RB2D_DC_FLUSH_ALL		0xf
# define RADEON_RB2D_DC_BUSY			(1 << 31)

#define R128_REG_PC_NGUI_CTLSTAT		0x0184
# define R128_PC_BUSY				(1 << 31)
#define R128_REG_GUI_STAT			0x1740
# define R128_GUI_ACTIVE			(1 << 31)
