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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "ati.h"
#include "ati_reg.h"
#include "ati_dma.h"
#include "ati_draw.h"

extern ATIScreenInfo *accel_atis;
static Bool is_transform[2];
static PictTransform *transform[2];

struct blendinfo {
	Bool dst_alpha;
	Bool src_alpha;
	CARD32 blend_cntl;
};

static struct blendinfo RadeonBlendOp[] = {
	/* Clear */
	{0, 0, RADEON_SBLEND_GL_ZERO	      | RADEON_DBLEND_GL_ZERO},
	/* Src */
	{0, 0, RADEON_SBLEND_GL_ONE	      | RADEON_DBLEND_GL_ZERO},
	/* Dst */
	{0, 0, RADEON_SBLEND_GL_ZERO	      | RADEON_DBLEND_GL_ONE},
	/* Over */
	{0, 1, RADEON_SBLEND_GL_ONE	      | RADEON_DBLEND_GL_INV_SRC_ALPHA},
	/* OverReverse */
	{1, 0, RADEON_SBLEND_GL_INV_DST_ALPHA | RADEON_DBLEND_GL_ONE},
	/* In */
	{1, 0, RADEON_SBLEND_GL_DST_ALPHA     | RADEON_DBLEND_GL_ZERO},
	/* InReverse */
	{0, 1, RADEON_SBLEND_GL_ZERO	      | RADEON_DBLEND_GL_SRC_ALPHA},
	/* Out */
	{1, 0, RADEON_SBLEND_GL_INV_DST_ALPHA | RADEON_DBLEND_GL_ZERO},
	/* OutReverse */
	{0, 1, RADEON_SBLEND_GL_ZERO	      | RADEON_DBLEND_GL_INV_SRC_ALPHA},
	/* Atop */
	{1, 1, RADEON_SBLEND_GL_DST_ALPHA     | RADEON_DBLEND_GL_INV_SRC_ALPHA},
	/* AtopReverse */
	{1, 1, RADEON_SBLEND_GL_INV_DST_ALPHA | RADEON_DBLEND_GL_SRC_ALPHA},
	/* Xor */
	{1, 1, RADEON_SBLEND_GL_INV_DST_ALPHA | RADEON_DBLEND_GL_INV_SRC_ALPHA},
	/* Add */
	{0, 0, RADEON_SBLEND_GL_ONE	      | RADEON_DBLEND_GL_ONE},
};

struct formatinfo {
	int fmt;
	Bool byte_swap;
	CARD32 card_fmt;
};

/* Note on texture formats:
 * TXFORMAT_Y8 expands to (Y,Y,Y,1).  TXFORMAT_I8 expands to (I,I,I,I)
 */
static struct formatinfo R100TexFormats[] = {
	{PICT_a8r8g8b8,	0, RADEON_TXFORMAT_ARGB8888 | RADEON_TXFORMAT_ALPHA_IN_MAP},
	{PICT_x8r8g8b8,	0, RADEON_TXFORMAT_ARGB8888},
	{PICT_a8b8g8r8,	1, RADEON_TXFORMAT_RGBA8888 | RADEON_TXFORMAT_ALPHA_IN_MAP},
	{PICT_x8b8g8r8,	1, RADEON_TXFORMAT_RGBA8888},
	{PICT_r5g6b5,	0, RADEON_TXFORMAT_RGB565},
	{PICT_a1r5g5b5,	0, RADEON_TXFORMAT_ARGB1555 | RADEON_TXFORMAT_ALPHA_IN_MAP},
	{PICT_x1r5g5b5,	0, RADEON_TXFORMAT_ARGB1555},
	{PICT_a8,	0, RADEON_TXFORMAT_I8 | RADEON_TXFORMAT_ALPHA_IN_MAP},
};

static struct formatinfo R200TexFormats[] = {
	{PICT_a8r8g8b8,	0, R200_TXFORMAT_ARGB8888 | R200_TXFORMAT_ALPHA_IN_MAP},
	{PICT_x8r8g8b8,	0, R200_TXFORMAT_ARGB8888},
	{PICT_a8r8g8b8,	1, R200_TXFORMAT_RGBA8888 | R200_TXFORMAT_ALPHA_IN_MAP},
	{PICT_x8r8g8b8,	1, R200_TXFORMAT_RGBA8888},
	{PICT_r5g6b5,	0, R200_TXFORMAT_RGB565},
	{PICT_a1r5g5b5,	0, R200_TXFORMAT_ARGB1555 | R200_TXFORMAT_ALPHA_IN_MAP},
	{PICT_x1r5g5b5,	0, R200_TXFORMAT_ARGB1555},
	{PICT_a8,	0, R200_TXFORMAT_I8 | R200_TXFORMAT_ALPHA_IN_MAP},
};

/* Common Radeon setup code */

static Bool
RadeonGetDestFormat(PicturePtr pDstPicture, CARD32 *dst_format)
{
	switch (pDstPicture->format) {
	case PICT_a8r8g8b8:
	case PICT_x8r8g8b8:
		*dst_format = RADEON_COLOR_FORMAT_ARGB8888;
		break;
	case PICT_r5g6b5:
		*dst_format = RADEON_COLOR_FORMAT_RGB565;
		break;
	case PICT_a1r5g5b5:
	case PICT_x1r5g5b5:
		*dst_format = RADEON_COLOR_FORMAT_ARGB1555;
		break;
	case PICT_a8:
		*dst_format = RADEON_COLOR_FORMAT_RGB8;
		break;
	default:
		ATI_FALLBACK(("Unsupported dest format 0x%x\n",
		    pDstPicture->format));
	}

	return TRUE;
}

/* R100-specific code */

static Bool
R100CheckCompositeTexture(PicturePtr pPict, int unit)
{
	int w = pPict->pDrawable->width;
	int h = pPict->pDrawable->height;
	int i;

	if ((w > 0x7ff) || (h > 0x7ff))
		ATI_FALLBACK(("Picture w/h too large (%dx%d)\n", w, h));

	for (i = 0; i < sizeof(R100TexFormats) / sizeof(R100TexFormats[0]); i++)
	{
		if (R100TexFormats[i].fmt == pPict->format)
			break;
	}
	if (i == sizeof(R100TexFormats) / sizeof(R100TexFormats[0]))
		ATI_FALLBACK(("Unsupported picture format 0x%x\n",
		    pPict->format));

	if (pPict->repeat && ((w & (w - 1)) != 0 || (h & (h - 1)) != 0))
		ATI_FALLBACK(("NPOT repeat unsupported (%dx%d)\n", w, h));

	return TRUE;
}

static Bool
R100TextureSetup(PicturePtr pPict, PixmapPtr pPix, int unit)
{
	ATIScreenInfo *atis = accel_atis;
	KdScreenPriv(pPix->drawable.pScreen);
	CARD32 txformat, txoffset, txpitch;
	int w = pPict->pDrawable->width;
	int h = pPict->pDrawable->height;
	int i;
	RING_LOCALS;

	txpitch = pPix->devKind;
	txoffset = ((CARD8 *)pPix->devPrivate.ptr -
	    pScreenPriv->screen->memory_base);

	for (i = 0; i < sizeof(R100TexFormats) / sizeof(R100TexFormats[0]); i++)
	{
		if (R100TexFormats[i].fmt == pPict->format)
			break;
	}
	txformat = R100TexFormats[i].card_fmt;
	if (R100TexFormats[i].byte_swap)
		txoffset |= RADEON_TXO_ENDIAN_BYTE_SWAP;

	if (pPict->repeat) {
		txformat |= ATILog2(w) << RADEON_TXFORMAT_WIDTH_SHIFT;
		txformat |= ATILog2(h) << RADEON_TXFORMAT_HEIGHT_SHIFT;
	} else 
		txformat |= RADEON_TXFORMAT_NON_POWER2;
	txformat |= unit << 24; /* RADEON_TXFORMAT_ST_ROUTE_STQX */

 
	if ((txoffset & 0x1f) != 0)
		ATI_FALLBACK(("Bad texture offset 0x%x\n", txoffset));
	if ((txpitch & 0x1f) != 0)
		ATI_FALLBACK(("Bad texture pitch 0x%x\n", txpitch));

	/* RADEON_REG_PP_TXFILTER_0,
	 * RADEON_REG_PP_TXFORMAT_0,
	 * RADEON_REG_PP_TXOFFSET_0
	 */
	BEGIN_DMA(4);
	OUT_RING(DMA_PACKET0(RADEON_REG_PP_TXFILTER_0 + 0x18 * unit, 3));
	OUT_RING(0);
	OUT_RING(txformat);
	OUT_RING(txoffset);
	END_DMA();

	/* RADEON_REG_PP_TEX_SIZE_0,
	 * RADEON_REG_PP_TEX_PITCH_0
	 */
	BEGIN_DMA(3);
	OUT_RING(DMA_PACKET0(RADEON_REG_PP_TEX_SIZE_0 + 0x8 * unit, 2));
	OUT_RING((pPix->drawable.width - 1) |
	    ((pPix->drawable.height - 1) << RADEON_TEX_VSIZE_SHIFT));
	OUT_RING(txpitch - 32);
	END_DMA();

	if (pPict->transform != 0) {
		is_transform[unit] = TRUE;
		transform[unit] = pPict->transform;
	} else {
		is_transform[unit] = FALSE;
	}

	return TRUE;
}

Bool
R100CheckComposite(int op, PicturePtr pSrcPicture, PicturePtr pMaskPicture,
    PicturePtr pDstPicture)
{
	CARD32 tmp1;

	/* Check for unsupported compositing operations. */
	if (op >= sizeof(RadeonBlendOp) / sizeof(RadeonBlendOp[0]))
		ATI_FALLBACK(("Unsupported Composite op 0x%x\n", op));
	if (pMaskPicture != NULL && pMaskPicture->componentAlpha &&
	    RadeonBlendOp[op].src_alpha)
		ATI_FALLBACK(("Component alpha not supported with source "
		    "alpha blending.\n"));
	if (pDstPicture->pDrawable->width >= (1 << 11) ||
	    pDstPicture->pDrawable->height >= (1 << 11))
		ATI_FALLBACK(("Dest w/h too large (%d,%d).\n",
		    pDstPicture->pDrawable->width,
		    pDstPicture->pDrawable->height));

	if (!R100CheckCompositeTexture(pSrcPicture, 0))
		return FALSE;
	if (pMaskPicture != NULL && !R100CheckCompositeTexture(pMaskPicture, 1))
		return FALSE;

	if (!RadeonGetDestFormat(pDstPicture, &tmp1))
		return FALSE;

	return TRUE;
}

Bool
R100PrepareComposite(int op, PicturePtr pSrcPicture, PicturePtr pMaskPicture,
    PicturePtr pDstPicture, PixmapPtr pSrc, PixmapPtr pMask, PixmapPtr pDst)
{
	KdScreenPriv(pDst->drawable.pScreen);
	ATIScreenInfo(pScreenPriv);
	CARD32 dst_format, dst_offset, dst_pitch;
	CARD32 pp_cntl, blendcntl, cblend, ablend;
	int pixel_shift;
	RING_LOCALS;

	accel_atis = atis;

	RadeonGetDestFormat(pDstPicture, &dst_format);
	pixel_shift = pDst->drawable.bitsPerPixel >> 4;

	dst_offset = ((CARD8 *)pDst->devPrivate.ptr -
	    pScreenPriv->screen->memory_base);
	dst_pitch = pDst->devKind;
	if ((dst_offset & 0x0f) != 0)
		ATI_FALLBACK(("Bad destination offset 0x%x\n", dst_offset));
	if (((dst_pitch >> pixel_shift) & 0x7) != 0)
		ATI_FALLBACK(("Bad destination pitch 0x%x\n", dst_pitch));

	if (!R100TextureSetup(pSrcPicture, pSrc, 0))
		return FALSE;
	pp_cntl = RADEON_TEX_0_ENABLE | RADEON_TEX_BLEND_0_ENABLE;

	if (pMask != NULL) {
		if (!R100TextureSetup(pMaskPicture, pMask, 1))
			return FALSE;
		pp_cntl |= RADEON_TEX_1_ENABLE;
	} else {
		is_transform[1] = FALSE;
	}

	BEGIN_DMA(14);
	OUT_REG(ATI_REG_WAIT_UNTIL,
		RADEON_WAIT_HOST_IDLECLEAN | RADEON_WAIT_2D_IDLECLEAN);

	/* RADEON_REG_PP_CNTL,
	 * RADEON_REG_RB3D_CNTL, 
	 * RADEON_REG_RB3D_COLOROFFSET
	 */
	OUT_RING(DMA_PACKET0(RADEON_REG_PP_CNTL, 3));
	OUT_RING(pp_cntl);
	OUT_RING(dst_format | RADEON_ALPHA_BLEND_ENABLE);
	OUT_RING(dst_offset);

	OUT_REG(RADEON_REG_RB3D_COLORPITCH, dst_pitch >> pixel_shift);

	/* IN operator: Multiply src by mask components or mask alpha.
	 * BLEND_CTL_ADD is A * B + C.
	 * If a picture is a8, we have to explicitly zero its color values.
	 * If the destination is a8, we have to route the alpha to red, I think.
	 */
	cblend = RADEON_BLEND_CTL_ADD | RADEON_CLAMP_TX |
	    RADEON_COLOR_ARG_C_ZERO;
	ablend = RADEON_BLEND_CTL_ADD | RADEON_CLAMP_TX |
	    RADEON_ALPHA_ARG_C_ZERO;

	if (pDstPicture->format == PICT_a8)
		cblend |= RADEON_COLOR_ARG_A_T0_ALPHA;
	else if (pSrcPicture->format == PICT_a8)
		cblend |= RADEON_COLOR_ARG_A_ZERO;
	else
		cblend |= RADEON_COLOR_ARG_A_T0_COLOR;
	ablend |= RADEON_ALPHA_ARG_A_T0_ALPHA;

	if (pMask) {
		if (pMaskPicture->componentAlpha &&
		    pDstPicture->format != PICT_a8)
			cblend |= RADEON_COLOR_ARG_B_T1_COLOR;
		else
			cblend |= RADEON_COLOR_ARG_B_T1_ALPHA;
		ablend |= RADEON_ALPHA_ARG_B_T1_ALPHA;
	} else {
		cblend |= RADEON_COLOR_ARG_B_ZERO | RADEON_COMP_ARG_B;
		ablend |= RADEON_ALPHA_ARG_B_ZERO | RADEON_COMP_ARG_B;
	}

	OUT_REG(RADEON_REG_PP_TXCBLEND_0, cblend);
	OUT_REG(RADEON_REG_PP_TXABLEND_0, ablend);

	/* Op operator. */
	blendcntl = RadeonBlendOp[op].blend_cntl;
	if (PICT_FORMAT_A(pDstPicture->format) == 0 &&
	    RadeonBlendOp[op].dst_alpha) {
		if ((blendcntl & RADEON_SBLEND_MASK) ==
		    RADEON_SBLEND_GL_DST_ALPHA)
			blendcntl = (blendcntl & ~RADEON_SBLEND_MASK) |
			    RADEON_SBLEND_GL_ONE;
		else if ((blendcntl & RADEON_SBLEND_MASK) ==
		    RADEON_SBLEND_GL_INV_DST_ALPHA)
			blendcntl = (blendcntl & ~RADEON_SBLEND_MASK) |
			    RADEON_SBLEND_GL_ZERO;
	}
	OUT_REG(RADEON_REG_RB3D_BLENDCNTL, blendcntl);
	END_DMA();

	return TRUE;
}

static Bool
R200CheckCompositeTexture(PicturePtr pPict, int unit)
{
	int w = pPict->pDrawable->width;
	int h = pPict->pDrawable->height;
	int i;

	if ((w > 0x7ff) || (h > 0x7ff))
		ATI_FALLBACK(("Picture w/h too large (%dx%d)\n", w, h));

	for (i = 0; i < sizeof(R200TexFormats) / sizeof(R200TexFormats[0]); i++)
	{
		if (R200TexFormats[i].fmt == pPict->format)
			break;
	}
	if (i == sizeof(R200TexFormats) / sizeof(R200TexFormats[0]))
		ATI_FALLBACK(("Unsupported picture format 0x%x\n",
		    pPict->format));

	if (pPict->repeat && ((w & (w - 1)) != 0 || (h & (h - 1)) != 0))
		ATI_FALLBACK(("NPOT repeat unsupported (%dx%d)\n", w, h));

	return TRUE;
}
static Bool
R200TextureSetup(PicturePtr pPict, PixmapPtr pPix, int unit)
{
	ATIScreenInfo *atis = accel_atis;
	KdScreenPriv(pPix->drawable.pScreen);
	CARD32 txformat, txoffset, txpitch;
	int w = pPict->pDrawable->width;
	int h = pPict->pDrawable->height;
	int i;
	RING_LOCALS;

	txpitch = pPix->devKind;
	txoffset = ((CARD8 *)pPix->devPrivate.ptr -
	    pScreenPriv->screen->memory_base);

	for (i = 0; i < sizeof(R200TexFormats) / sizeof(R200TexFormats[0]); i++)
	{
		if (R200TexFormats[i].fmt == pPict->format)
			break;
	}
	txformat = R200TexFormats[i].card_fmt;
	if (R100TexFormats[i].byte_swap)
		txoffset |= RADEON_TXO_ENDIAN_BYTE_SWAP;

	if (pPict->repeat) {
		txformat |= ATILog2(w) << R200_TXFORMAT_WIDTH_SHIFT;
		txformat |= ATILog2(h) << R200_TXFORMAT_HEIGHT_SHIFT;
	} else 
		txformat |= R200_TXFORMAT_NON_POWER2;
	txformat |= unit << R200_TXFORMAT_ST_ROUTE_SHIFT;

	if ((txoffset & 0x1f) != 0)
		ATI_FALLBACK(("Bad texture offset 0x%x\n", txoffset));
	if ((txpitch & 0x1f) != 0)
		ATI_FALLBACK(("Bad texture pitch 0x%x\n", txpitch));

	/* R200_REG_PP_TXFILTER_0,
	 * R200_REG_PP_TXFORMAT_0,
	 * R200_REG_PP_TXFORMAT_X_0,
	 * R200_REG_PP_TXSIZE_0,
	 * R200_REG_PP_TXPITCH_0
	 */
	BEGIN_DMA(6);
	OUT_RING(DMA_PACKET0(R200_REG_PP_TXFILTER_0 + 0x20 * unit, 5));
	OUT_RING(0);
	OUT_RING(txformat);
	OUT_RING(0);
	OUT_RING((pPix->drawable.width - 1) |
	    ((pPix->drawable.height - 1) << RADEON_TEX_VSIZE_SHIFT)); /* XXX */
	OUT_RING(txpitch - 32); /* XXX */
	END_DMA();

	BEGIN_DMA(2);
	OUT_REG(R200_PP_TXOFFSET_0 + 0x18 * unit, txoffset);
	END_DMA();

	if (pPict->transform != 0) {
		is_transform[unit] = TRUE;
		transform[unit] = pPict->transform;
	} else {
		is_transform[unit] = FALSE;
	}

	return TRUE;
}

Bool
R200CheckComposite(int op, PicturePtr pSrcPicture, PicturePtr pMaskPicture,
    PicturePtr pDstPicture)
{
	CARD32 tmp1;

	/* Check for unsupported compositing operations. */
	if (op >= sizeof(RadeonBlendOp) / sizeof(RadeonBlendOp[0]))
		ATI_FALLBACK(("Unsupported Composite op 0x%x\n", op));
	if (pMaskPicture != NULL && pMaskPicture->componentAlpha &&
	    RadeonBlendOp[op].src_alpha)
		ATI_FALLBACK(("Component alpha not supported with source "
		    "alpha blending.\n"));

	if (!R200CheckCompositeTexture(pSrcPicture, 0))
		return FALSE;
	if (pMaskPicture != NULL && !R200CheckCompositeTexture(pMaskPicture, 1))
		return FALSE;

	if (!RadeonGetDestFormat(pDstPicture, &tmp1))
		return FALSE;

	return TRUE;
}

Bool
R200PrepareComposite(int op, PicturePtr pSrcPicture, PicturePtr pMaskPicture,
    PicturePtr pDstPicture, PixmapPtr pSrc, PixmapPtr pMask, PixmapPtr pDst)
{
	KdScreenPriv(pDst->drawable.pScreen);
	ATIScreenInfo(pScreenPriv);
	CARD32 dst_format, dst_offset, dst_pitch;
	CARD32 pp_cntl, blendcntl, cblend, ablend;
	int pixel_shift;
	RING_LOCALS;

	RadeonGetDestFormat(pDstPicture, &dst_format);
	pixel_shift = pDst->drawable.bitsPerPixel >> 4;

	accel_atis = atis;

	dst_offset = ((CARD8 *)pDst->devPrivate.ptr -
	    pScreenPriv->screen->memory_base);
	dst_pitch = pDst->devKind;
	if ((dst_offset & 0x0f) != 0)
		ATI_FALLBACK(("Bad destination offset 0x%x\n", dst_offset));
	if (((dst_pitch >> pixel_shift) & 0x7) != 0)
		ATI_FALLBACK(("Bad destination pitch 0x%x\n", dst_pitch));

	if (!R200TextureSetup(pSrcPicture, pSrc, 0))
		return FALSE;
	pp_cntl = RADEON_TEX_0_ENABLE | RADEON_TEX_BLEND_0_ENABLE;

	if (pMask != NULL) {
		if (!R200TextureSetup(pMaskPicture, pMask, 1))
			return FALSE;
		pp_cntl |= RADEON_TEX_1_ENABLE;
	} else {
		is_transform[1] = FALSE;
	}

	BEGIN_DMA(34);
	OUT_REG(ATI_REG_WAIT_UNTIL,
		RADEON_WAIT_HOST_IDLECLEAN | RADEON_WAIT_2D_IDLECLEAN);

	/* RADEON_REG_PP_CNTL,
	 * RADEON_REG_RB3D_CNTL, 
	 * RADEON_REG_RB3D_COLOROFFSET
	 */
	OUT_RING(DMA_PACKET0(RADEON_REG_PP_CNTL, 3));
	OUT_RING(pp_cntl);
	OUT_RING(dst_format | RADEON_ALPHA_BLEND_ENABLE);
	OUT_RING(dst_offset);

	OUT_REG(RADEON_REG_RB3D_COLORPITCH, dst_pitch >> pixel_shift);

	/* IN operator: Multiply src by mask components or mask alpha.
	 * BLEND_CTL_ADD is A * B + C.
	 * If a picture is a8, we have to explicitly zero its color values.
	 * If the destination is a8, we have to route the alpha to red, I think.
	 */
	cblend = R200_TXC_OP_MADD | R200_TXC_ARG_C_ZERO;
	ablend = R200_TXA_OP_MADD | R200_TXA_ARG_C_ZERO;

	if (pDstPicture->format == PICT_a8)
		cblend |= R200_TXC_ARG_A_R0_ALPHA;
	else if (pSrcPicture->format == PICT_a8)
		cblend |= R200_TXC_ARG_A_ZERO;
	else
		cblend |= R200_TXC_ARG_A_R0_COLOR;
	ablend |= R200_TXA_ARG_B_R0_ALPHA;

	if (pMask) {
		if (pMaskPicture->componentAlpha &&
		    pDstPicture->format != PICT_a8)
			cblend |= R200_TXC_ARG_B_R1_COLOR;
		else
			cblend |= R200_TXC_ARG_B_R1_ALPHA;
		ablend |= R200_TXA_ARG_B_R1_ALPHA;
	} else {
		cblend |= R200_TXC_ARG_B_ZERO | R200_TXC_COMP_ARG_B;
		ablend |= R200_TXA_ARG_B_ZERO | R200_TXA_COMP_ARG_B;
	}

	OUT_REG(R200_REG_PP_TXCBLEND_0, cblend);
	OUT_REG(R200_REG_PP_TXABLEND_0, ablend);
	OUT_REG(R200_REG_PP_TXCBLEND2_0, 0);
	OUT_REG(R200_REG_PP_TXABLEND2_0, 0);

	/* Op operator. */
	blendcntl = RadeonBlendOp[op].blend_cntl;
	if (PICT_FORMAT_A(pDstPicture->format) == 0 &&
	    RadeonBlendOp[op].dst_alpha) {
		blendcntl &= ~RADEON_SBLEND_MASK;
		if ((blendcntl & RADEON_SBLEND_MASK) ==
		    RADEON_SBLEND_GL_DST_ALPHA)
			blendcntl |= RADEON_SBLEND_GL_ONE;
		else if ((blendcntl & RADEON_SBLEND_MASK) ==
		    RADEON_SBLEND_GL_INV_DST_ALPHA)
			blendcntl |= RADEON_SBLEND_GL_ZERO;
	}
	OUT_REG(RADEON_REG_RB3D_BLENDCNTL, blendcntl);
	END_DMA();

	return TRUE;
}

union intfloat {
	float f;
	CARD32 i;
};

struct blend_vertex {
	union intfloat x, y;
	union intfloat s0, t0;
	union intfloat s1, t1;
};

#define VTX_DWORD_COUNT 6

#define VTX_OUT(vtx)		\
do {				\
	OUT_RING(vtx.x.i);	\
	OUT_RING(vtx.y.i);	\
	OUT_RING(vtx.s0.i);	\
	OUT_RING(vtx.t0.i);	\
	OUT_RING(vtx.s1.i);	\
	OUT_RING(vtx.t1.i);	\
} while (0)

void
RadeonComposite(int srcX, int srcY, int maskX, int maskY, int dstX, int dstY,
    int w, int h)
{
	ATIScreenInfo *atis = accel_atis;
	ATICardInfo *atic = atis->atic;
	struct blend_vertex vtx[4];
	int srcXend, srcYend, maskXend, maskYend;
	RING_LOCALS;

	/*ErrorF("RadeonComposite (%d,%d) (%d,%d) (%d,%d) (%d,%d)\n",
	    srcX, srcY, maskX, maskY,dstX, dstY, w, h);*/

	if (is_transform[0]) {
		PictVector v;

		v.vector[0] = IntToxFixed(srcX);
		v.vector[1] = IntToxFixed(srcY);
		v.vector[3] = xFixed1;
		PictureTransformPoint(transform[0], &v);
		srcX = xFixedToInt(v.vector[0]);
		srcY = xFixedToInt(v.vector[1]);
		v.vector[0] = IntToxFixed(srcX + w);
		v.vector[1] = IntToxFixed(srcY + h);
		v.vector[3] = xFixed1;
		PictureTransformPoint(transform[0], &v);
		srcXend = xFixedToInt(v.vector[0]);
		srcYend = xFixedToInt(v.vector[1]);
	} else {
		srcXend = srcX + w;
		srcYend = srcY + h;
	}
	if (is_transform[1]) {
		PictVector v;

		v.vector[0] = IntToxFixed(maskX);
		v.vector[1] = IntToxFixed(maskY);
		v.vector[3] = xFixed1;
		PictureTransformPoint(transform[1], &v);
		maskX = xFixedToInt(v.vector[0]);
		maskY = xFixedToInt(v.vector[1]);
		v.vector[0] = IntToxFixed(maskX + w);
		v.vector[1] = IntToxFixed(maskY + h);
		v.vector[3] = xFixed1;
		PictureTransformPoint(transform[1], &v);
		maskXend = xFixedToInt(v.vector[0]);
		maskYend = xFixedToInt(v.vector[1]);
	} else {
		maskXend = maskX + w;
		maskYend = maskY + h;
	}

	vtx[0].x.f = dstX;
	vtx[0].y.f = dstY;
	vtx[0].s0.f = srcX;
	vtx[0].t0.f = srcY;
	vtx[0].s1.f = maskX;
	vtx[0].t1.f = maskY;

	vtx[1].x.f = dstX;
	vtx[1].y.f = dstY + h;
	vtx[1].s0.f = srcX;
	vtx[1].t0.f = srcYend;
	vtx[1].s1.f = maskX;
	vtx[1].t1.f = maskYend;

	vtx[2].x.f = dstX + w;
	vtx[2].y.f = dstY + h;
	vtx[2].s0.f = srcXend;
	vtx[2].t0.f = srcYend;
	vtx[2].s1.f = maskXend;
	vtx[2].t1.f = maskYend;

	vtx[3].x.f = dstX + w;
	vtx[3].y.f = dstY;
	vtx[3].s0.f = srcXend;
	vtx[3].t0.f = srcY;
	vtx[3].s1.f = maskXend;
	vtx[3].t1.f = maskY;

	if (atic->is_r100) {
		BEGIN_DMA(4 * VTX_DWORD_COUNT + 3);
		OUT_RING(DMA_PACKET3(RADEON_CP_PACKET3_3D_DRAW_IMMD,
		    4 * VTX_DWORD_COUNT + 2));
		OUT_RING(RADEON_CP_VC_FRMT_XY |
		    RADEON_CP_VC_FRMT_ST0 |
		    RADEON_CP_VC_FRMT_ST1);
		OUT_RING(RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_FAN |
		    RADEON_CP_VC_CNTL_PRIM_WALK_RING |
		    RADEON_CP_VC_CNTL_MAOS_ENABLE |
		    RADEON_CP_VC_CNTL_VTX_FMT_RADEON_MODE |
		    (4 << RADEON_CP_VC_CNTL_NUM_SHIFT));
	} else {
		BEGIN_DMA(4 * VTX_DWORD_COUNT + 2);
		OUT_RING(DMA_PACKET3(R200_CP_PACKET3_3D_DRAW_IMMD_2,
		    4 * VTX_DWORD_COUNT + 1));
		OUT_RING(RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_FAN |
		    RADEON_CP_VC_CNTL_PRIM_WALK_RING |
		    (4 << RADEON_CP_VC_CNTL_NUM_SHIFT));
	}

	VTX_OUT(vtx[0]);
	VTX_OUT(vtx[1]);
	VTX_OUT(vtx[2]);
	VTX_OUT(vtx[3]);

	END_DMA();
}

void
RadeonDoneComposite(void)
{
}
