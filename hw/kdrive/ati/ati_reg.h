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
#define RADEON_REG_BUS_CNTL			0x0030
# define RADEON_BUS_MASTER_DIS			(1 << 6)
#define RADEON_GEN_INT_CNTL			0x0040
#define RADEON_REG_AGP_BASE			0x0170
#define RADEON_REG_AGP_CNTL			0x0174
# define RADEON_AGP_APER_SIZE_256MB		(0x00 << 0)
# define RADEON_AGP_APER_SIZE_128MB		(0x20 << 0)
# define RADEON_AGP_APER_SIZE_64MB		(0x30 << 0)
# define RADEON_AGP_APER_SIZE_32MB		(0x38 << 0)
# define RADEON_AGP_APER_SIZE_16MB		(0x3c << 0)
# define RADEON_AGP_APER_SIZE_8MB		(0x3e << 0)
# define RADEON_AGP_APER_SIZE_4MB		(0x3f << 0)
# define RADEON_AGP_APER_SIZE_MASK		(0x3f << 0)
#define RADEON_REG_RBBM_STATUS			0x0e40
# define RADEON_RBBM_FIFOCNT_MASK		0x007f
# define RADEON_RBBM_ACTIVE			(1 << 31)
#define RADEON_REG_CP_CSQ_CNTL			0x0740
# define RADEON_CSQ_PRIBM_INDBM			(4    << 28)
#define RADEON_REG_SRC_PITCH_OFFSET		0x1428
#define RADEON_REG_DST_PITCH_OFFSET		0x142c
#define RADEON_REG_SRC_Y_X			0x1434
#define RADEON_REG_DST_Y_X			0x1438
#define RADEON_REG_DST_HEIGHT_WIDTH		0x143c
#define RADEON_REG_DP_GUI_MASTER_CNTL		0x146c
# define RADEON_GMC_SRC_PITCH_OFFSET_CNTL	(1    <<  0)
# define RADEON_GMC_DST_PITCH_OFFSET_CNTL	(1    <<  1)
# define RADEON_GMC_BRUSH_SOLID_COLOR		(13   <<  4)
# define RADEON_GMC_BRUSH_NONE			(15   <<  4)
# define RADEON_GMC_SRC_DATATYPE_COLOR		(3    << 12)
# define RADEON_DP_SRC_SOURCE_MEMORY		(2    << 24)
# define RADEON_GMC_3D_FCN_EN			(1    << 27)
# define RADEON_GMC_CLR_CMP_CNTL_DIS		(1    << 28)
# define RADEON_GMC_AUX_CLIP_DIS		(1    << 29)
#define RADEON_REG_DP_BRUSH_FRGD_CLR		0x147c
#define RADEON_REG_DST_WIDTH_HEIGHT		0x1598
#define RADEON_REG_CLR_CMP_CNTL			0x15c0
#define RADEON_REG_AUX_SC_CNTL			0x1660
#define RADEON_REG_DP_CNTL			0x16c0
# define RADEON_DST_X_LEFT_TO_RIGHT		(1 <<  0)
# define RADEON_DST_Y_TOP_TO_BOTTOM		(1 <<  1)
#define RADEON_REG_DP_MIX			0x16c8
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

#define RADEON_CP_PACKET0			0x00000000
#define RADEON_CP_PACKET1			0x40000000
#define RADEON_CP_PACKET2			0x80000000

#define R128_REG_PC_NGUI_CTLSTAT		0x0184
# define R128_PC_BUSY				(1 << 31)
#define R128_REG_PCI_GART_PAGE			0x017c
#define R128_REG_PC_NGUI_CTLSTAT		0x0184
#define R128_REG_BM_CHUNK_0_VAL			0x0a18
# define R128_BM_PTR_FORCE_TO_PCI		(1 << 21)
# define R128_BM_PM4_RD_FORCE_TO_PCI		(1 << 22)
# define R128_BM_GLOBAL_FORCE_TO_PCI		(1 << 23)
#define R128_REG_GUI_STAT			0x1740
# define R128_GUI_ACTIVE			(1 << 31)

#define R128_REG_TEX_CNTL			0x1800
#define R128_REG_SCALE_SRC_HEIGHT_WIDTH		0x1994
#define R128_REG_SCALE_OFFSET_0			0x1998
#define R128_REG_SCALE_PITCH			0x199c
#define R128_REG_SCALE_X_INC			0x19a0
#define R128_REG_SCALE_Y_INC			0x19a4
#define R128_REG_SCALE_HACC			0x19a8
#define R128_REG_SCALE_VACC			0x19ac
#define R128_REG_SCALE_DST_X_Y			0x19b0
#define R128_REG_SCALE_DST_HEIGHT_WIDTH		0x19b4

#define R128_REG_SCALE_3D_CNTL			0x1a00
# define R128_SCALE_DITHER_ERR_DIFF		(0  <<  1)
# define R128_SCALE_DITHER_TABLE		(1  <<  1)
# define R128_TEX_CACHE_SIZE_FULL		(0  <<  2)
# define R128_TEX_CACHE_SIZE_HALF		(1  <<  2)
# define R128_DITHER_INIT_CURR			(0  <<  3)
# define R128_DITHER_INIT_RESET			(1  <<  3)
# define R128_ROUND_24BIT			(1  <<  4)
# define R128_TEX_CACHE_DISABLE			(1  <<  5)
# define R128_SCALE_3D_NOOP			(0  <<  6)
# define R128_SCALE_3D_SCALE			(1  <<  6)
# define R128_SCALE_3D_TEXMAP_SHADE		(2  <<  6)
# define R128_SCALE_PIX_BLEND			(0  <<  8)
# define R128_SCALE_PIX_REPLICATE		(1  <<  8)
# define R128_TEX_CACHE_SPLIT			(1  <<  9)
# define R128_APPLE_YUV_MODE			(1  << 10)
# define R128_TEX_CACHE_PALLETE_MODE		(1  << 11)
# define R128_ALPHA_COMB_ADD_CLAMP		(0  << 12)
# define R128_ALPHA_COMB_ADD_NCLAMP		(1  << 12)
# define R128_ALPHA_COMB_SUB_DST_SRC_CLAMP	(2  << 12)
# define R128_ALPHA_COMB_SUB_DST_SRC_NCLAMP	(3  << 12)
# define R128_FOG_TABLE				(1  << 14)
# define R128_SIGNED_DST_CLAMP			(1  << 15)
# define R128_ALPHA_BLEND_SRC_ZERO		(0  << 16)
# define R128_ALPHA_BLEND_SRC_ONE		(1  << 16)
# define R128_ALPHA_BLEND_SRC_SRCCOLOR		(2  << 16)
# define R128_ALPHA_BLEND_SRC_INVSRCCOLOR	(3  << 16)
# define R128_ALPHA_BLEND_SRC_SRCALPHA		(4  << 16)
# define R128_ALPHA_BLEND_SRC_INVSRCALPHA	(5  << 16)
# define R128_ALPHA_BLEND_SRC_DSTALPHA		(6  << 16)
# define R128_ALPHA_BLEND_SRC_INVDSTALPHA	(7  << 16)
# define R128_ALPHA_BLEND_SRC_DSTCOLOR		(8  << 16)
# define R128_ALPHA_BLEND_SRC_INVDSTCOLOR	(9  << 16)
# define R128_ALPHA_BLEND_SRC_SAT		(10 << 16)
# define R128_ALPHA_BLEND_SRC_BLEND		(11 << 16)
# define R128_ALPHA_BLEND_SRC_INVBLEND		(12 << 16)
# define R128_ALPHA_BLEND_DST_ZERO		(0  << 20)
# define R128_ALPHA_BLEND_DST_ONE		(1  << 20)
# define R128_ALPHA_BLEND_DST_SRCCOLOR		(2  << 20)
# define R128_ALPHA_BLEND_DST_INVSRCCOLOR	(3  << 20)
# define R128_ALPHA_BLEND_DST_SRCALPHA		(4  << 20)
# define R128_ALPHA_BLEND_DST_INVSRCALPHA	(5  << 20)
# define R128_ALPHA_BLEND_DST_DSTALPHA		(6  << 20)
# define R128_ALPHA_BLEND_DST_INVDSTALPHA	(7  << 20)
# define R128_ALPHA_BLEND_DST_DSTCOLOR		(8  << 20)
# define R128_ALPHA_BLEND_DST_INVDSTCOLOR	(9  << 20)
# define R128_ALPHA_TEST_NEVER			(0  << 24)
# define R128_ALPHA_TEST_LESS			(1  << 24)
# define R128_ALPHA_TEST_LESSEQUAL		(2  << 24)
# define R128_ALPHA_TEST_EQUAL			(3  << 24)
# define R128_ALPHA_TEST_GREATEREQUAL		(4  << 24)
# define R128_ALPHA_TEST_GREATER		(5  << 24)
# define R128_ALPHA_TEST_NEQUAL			(6  << 24)
# define R128_ALPHA_TEST_ALWAYS			(7  << 24)
# define R128_COMPOSITE_SHADOW_CMP_EQUAL	(0  << 28)
# define R128_COMPOSITE_SHADOW_CMP_NEQUAL	(1  << 28)
# define R128_COMPOSITE_SHADOW			(1  << 29)
# define R128_TEX_MAP_ALPHA_IN_TEXTURE		(1  << 30)
# define R128_TEX_CACHE_LINE_SIZE_8QW		(0  << 31)
# define R128_TEX_CACHE_LINE_SIZE_4QW		(1  << 31)

#define R128_REG_SCALE_3D_DATATYPE		0x1a20

#define R128_REG_TEX_CNTL_C			0x1c9c
# define R128_TEX_ALPHA_EN			(1 << 9)
# define R128_TEX_CACHE_FLUSH			(1 << 23)

#define R128_REG_PRIM_TEX_CNTL_C		0x1cb0
#define R128_REG_PRIM_TEXTURE_COMBINE_CNTL_C	0x1cb4

#define R128_DATATYPE_C8			2
#define R128_DATATYPE_ARGB_1555			3
#define R128_DATATYPE_RGB_565			4
#define R128_DATATYPE_ARGB_8888			6
#define R128_DATATYPE_RGB_332			7
#define R128_DATATYPE_Y8			8
#define R128_DATATYPE_RGB_8			9
#define R128_DATATYPE_VYUY_422			11
#define R128_DATATYPE_YVYU_422			12
#define R128_DATATYPE_AYUV_444			14
#define R128_DATATYPE_ARGB_4444			15

#define R128_PM4_NONPM4				(0  << 28)
#define R128_PM4_192PIO				(1  << 28)
#define R128_PM4_192BM				(2  << 28)
#define R128_PM4_128PIO_64INDBM			(3  << 28)
#define R128_PM4_128BM_64INDBM			(4  << 28)
#define R128_PM4_64PIO_128INDBM			(5  << 28)
#define R128_PM4_64BM_128INDBM			(6  << 28)
#define R128_PM4_64PIO_64VCBM_64INDBM		(7  << 28)
#define R128_PM4_64BM_64VCBM_64INDBM		(8  << 28)
#define R128_PM4_64PIO_64VCPIO_64INDPIO		(15 << 28)
