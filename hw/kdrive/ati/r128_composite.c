/*
 * Copyright © 2003 Eric Anholt, Anders Carlsson
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

#include "ati.h"
#include "ati_reg.h"
#include "ati_dma.h"
#include "ati_draw.h"

extern ATIScreenInfo *accel_atis;
extern CARD8 ATIBltRop[16];

static int src_pitch;
static int src_offset;
static int src_bpp;
int widths[2] = {1,1};
int heights[2] = {1,1};
Bool is_repeat;

struct blendinfo {
	Bool dst_alpha;
	Bool src_alpha;
	CARD32 blendctl;
};

static struct blendinfo R128BlendOp[] = {
	/* Clear */
	{0, 0, R128_SBLEND_ZERO		 | R128_DBLEND_ZERO},
	/* Src */
	{0, 0, R128_SBLEND_ONE		 | R128_DBLEND_ZERO},
	/* Dst */
	{0, 0, R128_SBLEND_ZERO		 | R128_DBLEND_ONE},
	/* Over */
	{0, 1, R128_SBLEND_ONE		 | R128_DBLEND_INV_SRC_ALPHA},
	/* OverReverse */
	{1, 0, R128_SBLEND_INV_DST_ALPHA | R128_DBLEND_ONE},
	/* In */
	{1, 0, R128_SBLEND_DST_ALPHA	 | R128_DBLEND_ZERO},
	/* InReverse */
	{0, 1, R128_SBLEND_ZERO		 | R128_DBLEND_SRC_ALPHA},
	/* Out */
	{1, 0, R128_SBLEND_INV_DST_ALPHA | R128_DBLEND_ZERO},
	/* OutReverse */
	{0, 1, R128_SBLEND_ZERO		 | R128_DBLEND_INV_SRC_ALPHA},
	/* Atop */
	{1, 1, R128_SBLEND_DST_ALPHA	 | R128_DBLEND_INV_SRC_ALPHA},
	/* AtopReverse */
	{1, 1, R128_SBLEND_INV_DST_ALPHA | R128_DBLEND_SRC_ALPHA},
	/* Xor */
	{1, 1, R128_SBLEND_INV_DST_ALPHA | R128_DBLEND_INV_SRC_ALPHA},
	/* Add */
	{0, 0, R128_SBLEND_ONE		 | R128_DBLEND_ONE},
};

static Bool
R128GetDatatypePict(CARD32 format, CARD32 *type)
{
	switch (format) {
	case PICT_a1r5g5b5:
		*type = R128_DATATYPE_ARGB1555;
		return TRUE;
	case PICT_r5g6b5:
		*type = R128_DATATYPE_RGB565;
		return TRUE;
	case PICT_a8r8g8b8:
		*type = R128_DATATYPE_ARGB8888;
		return TRUE;
	default:
		return FALSE;
	}

}

Bool
R128PrepareBlend(int op, PicturePtr pSrcPicture, PicturePtr pDstPicture,
    PixmapPtr pSrc, PixmapPtr pDst)
{
	KdScreenPriv(pDst->drawable.pScreen);
	ATIScreenInfo(pScreenPriv);
	CARD32 dstDatatype, srcDatatype;
	RING_LOCALS;
	CARD32 xinc, yinc, dst_pitch_offset;

	accel_atis = atis;

	src_offset = (CARD8 *)pSrc->devPrivate.ptr -
	    pScreenPriv->screen->memory_base;
	src_pitch = pSrc->devKind;
	src_bpp = pSrc->drawable.bitsPerPixel;
	is_repeat = pSrcPicture->repeat;

	if (op >= sizeof(R128BlendOp)/sizeof(R128BlendOp[0]))
		ATI_FALLBACK(("Unsupported op 0x%x\n", op));
	if (pSrcPicture->repeat && (pSrc->drawable.width != 1 ||
	    pSrc->drawable.height != 1))
		ATI_FALLBACK(("repeat unsupported\n"));
	if (pSrcPicture->transform != NULL)
		ATI_FALLBACK(("transform unsupported\n"));
	if (!R128GetDatatypePict(pDstPicture->format, &dstDatatype))
		ATI_FALLBACK(("Unsupported dest format 0x%x\n",
		    pDstPicture->format));
	if (!R128GetDatatypePict(pSrcPicture->format, &srcDatatype))
		ATI_FALLBACK(("Unsupported src format 0x%x\n",
		    pSrcPicture->format));
	if (src_pitch % src_bpp != 0)
		ATI_FALLBACK(("Bad src pitch 0x%x\n", src_pitch));
	if (!ATIGetPixmapOffsetPitch(pDst, &dst_pitch_offset))
		return FALSE;

	if (is_repeat) {
		xinc = 0;
		yinc = 0;
	} else {
		xinc = 65536;
		yinc = 65536;
	}

	BEGIN_DMA(18);
	OUT_REG(ATI_REG_DST_PITCH_OFFSET, dst_pitch_offset);
	OUT_REG(ATI_REG_DP_GUI_MASTER_CNTL,
	    ATI_GMC_DST_PITCH_OFFSET_CNTL |
	    ATI_GMC_BRUSH_SOLID_COLOR |
	    (dstDatatype << 8) |
	    ATI_GMC_SRC_DATATYPE_COLOR |
	    (ATIBltRop[GXcopy] << 16) |
	    ATI_DP_SRC_SOURCE_MEMORY |
	    R128_GMC_3D_FCN_EN |
	    ATI_GMC_CLR_CMP_CNTL_DIS |
	    R128_GMC_AUX_CLIP_DIS);
	OUT_REG(ATI_REG_DP_CNTL, 
	    ATI_DST_X_LEFT_TO_RIGHT | ATI_DST_Y_TOP_TO_BOTTOM );
	OUT_REG(R128_REG_SCALE_3D_CNTL,
	    R128_SCALE_3D_SCALE |
	    R128_SCALE_PIX_REPLICATE |
	    R128BlendOp[op].blendctl |
	    R128_TEX_MAP_ALPHA_IN_TEXTURE);
	OUT_REG(R128_REG_TEX_CNTL_C, R128_ALPHA_ENABLE | R128_TEX_CACHE_FLUSH);
	OUT_REG(R128_REG_SCALE_3D_DATATYPE, srcDatatype);

	/* R128_REG_SCALE_PITCH,
	 * R128_REG_SCALE_X_INC,
	 * R128_REG_SCALE_Y_INC,
	 * R128_REG_SCALE_HACC
	 * R128_REG_SCALE_VACC */
	OUT_RING(DMA_PACKET0(R128_REG_SCALE_PITCH, 5));
	OUT_RING(src_pitch / src_bpp);
	OUT_RING(xinc);
	OUT_RING(yinc);
	OUT_RING(0x0);
	OUT_RING(0x0);
	END_DMA();

	return TRUE;
}

void
R128Blend(int srcX, int srcY, int dstX, int dstY, int width, int height)
{
	ATIScreenInfo *atis = accel_atis;
	RING_LOCALS;

	if (is_repeat) {
		srcX = 0;
		srcY = 0;
	}

	BEGIN_DMA(6);
	/* R128_REG_SCALE_SRC_HEIGHT_WIDTH,
	 * R128_REG_SCALE_OFFSET_0
	 */
	OUT_RING(DMA_PACKET0(R128_REG_SCALE_SRC_HEIGHT_WIDTH, 2));
	OUT_RING((height << 16) | width);
	OUT_RING(src_offset + srcY * src_pitch + srcX * (src_bpp >> 3));
	/* R128_REG_SCALE_DST_X_Y
	 * R128_REG_SCALE_DST_HEIGHT_WIDTH
	 */
	OUT_RING(DMA_PACKET0(R128_REG_SCALE_DST_X_Y, 2));
	OUT_RING((dstX << 16) | dstY);
	OUT_RING((height << 16) | width);
	END_DMA();
}

void
R128DoneBlend(void)
{
}

static Bool
R128CheckCompositeTexture(PicturePtr pPict)
{
	int w = pPict->pDrawable->width;
	int h = pPict->pDrawable->height;

	if (w > (1 << 10) || h > (1 << 10))
		ATI_FALLBACK(("Picture w/h too large (%dx%d)\n", w, h));
	if (pPict->repeat && ((w & (w - 1)) != 0 || (h & (h - 1)) != 0))
		ATI_FALLBACK(("NPOT repeat unsupported (%dx%d)\n", w, h));

	switch (pPict->format) {
	case PICT_a8:
	case PICT_a1r5g5b5:
	case PICT_a4r4g4b4:
	case PICT_r5g6b5:
	case PICT_a8r8g8b8:
		break;
	default:
		ATI_FALLBACK(("Unsupported picture format 0x%x\n",
		    pPict->format));
	}

	return TRUE;
}

Bool
R128CheckComposite(int op, PicturePtr pSrcPicture, PicturePtr pMaskPicture,
    PicturePtr pDstPicture)
{
	CARD32 dstDatatype;

	if (op >= sizeof(R128BlendOp)/sizeof(R128BlendOp[0]))
		ATI_FALLBACK(("Unsupported op 0x%x\n", op));
	if (op == PictOpIn || op == PictOpOut || op == PictOpAtopReverse ||
	    op == PictOpXor)
	if (op != PictOpOver)
		ATI_FALLBACK(("Something's wacky with these ops\n"));
	if (pSrcPicture->transform)
		ATI_FALLBACK(("Source transform unsupported.\n"));
	if (pMaskPicture && pMaskPicture->transform)
		ATI_FALLBACK(("Mask transform unsupported.\n"));
	if (pDstPicture->format == PICT_a8) {
		if (R128BlendOp[op].src_alpha || R128BlendOp[op].dst_alpha ||
		    pMaskPicture != NULL)
			ATI_FALLBACK(("alpha blending unsupported with "
			    "A8 dst?\n"));
	} else if (!R128GetDatatypePict(pDstPicture->format, &dstDatatype)) {
		ATI_FALLBACK(("Unsupported dest format 0x%x\n",
		    pDstPicture->format));
	}
	if (pMaskPicture != NULL && pMaskPicture->componentAlpha &&
	    R128BlendOp[op].src_alpha)
		ATI_FALLBACK(("Component alpha not supported with source alpha "
		    "blending.\n"));

	if (!R128CheckCompositeTexture(pSrcPicture))
		return FALSE;
	if (pMaskPicture != NULL && !R128CheckCompositeTexture(pMaskPicture))
		return FALSE;

	return TRUE;
}

static Bool
R128TextureSetup(PicturePtr pPict, PixmapPtr pPix, int unit, CARD32 *txsize,
    CARD32 *tex_cntl_c)
{
	int w = pPict->pDrawable->width;
	int h = pPict->pDrawable->height;
	int bytepp, shift, l2w, l2h, l2p;
	int pitch;

	pitch = pPix->devKind;
	if ((pitch & (pitch - 1)) != 0)
		ATI_FALLBACK(("NPOT pitch 0x%x unsupported\n", pitch));

	switch (pPict->format) {
	case PICT_a8:
		/* DATATYPE_RGB8 appears to expand the value into the alpha
		 * channel like we want.  We then blank out the R,G,B channels
		 * as necessary using the combiners.
		 */
		*tex_cntl_c = R128_DATATYPE_RGB8 << R128_TEX_DATATYPE_SHIFT;
		bytepp = 1;
		break;
	case PICT_a1r5g5b5:
		*tex_cntl_c = R128_DATATYPE_ARGB1555 << R128_TEX_DATATYPE_SHIFT;
		bytepp = 2;
		break;
	case PICT_a4r4g4b4:
		*tex_cntl_c = R128_DATATYPE_ARGB4444 << R128_TEX_DATATYPE_SHIFT;
		bytepp = 2;
		break;
	case PICT_r5g6b5:
		*tex_cntl_c = R128_DATATYPE_RGB565 << R128_TEX_DATATYPE_SHIFT;
		bytepp = 2;
		break;
	case PICT_a8r8g8b8:
		*tex_cntl_c = R128_DATATYPE_ARGB8888 << R128_TEX_DATATYPE_SHIFT;
		bytepp = 4;
		break;
	default:
		return FALSE;
	}

	*tex_cntl_c |= R128_MIP_MAP_DISABLE;

	if (unit == 0)
		shift = 0;
	else {
		shift = 16;
		*tex_cntl_c |= R128_SEC_SELECT_SEC_ST;
	}

	if (w == 1)
		l2w = 0;
	else
		l2w = ATILog2(w - 1) + 1;
	if (h == 1)
		l2h = 0;
	else
		l2h = ATILog2(h - 1) + 1;
	l2p = ATILog2(pPix->devKind / bytepp);

	if (pPict->repeat && w == 1 && h == 1)
		l2p = 0;
	else if (pPict->repeat && l2p != l2w)
		ATI_FALLBACK(("Repeat not supported for pitch != width\n"));
	l2w = l2p;

	widths[unit] = 1 << l2w;
	heights[unit] = 1 << l2h;
	*txsize |= l2p << (R128_TEX_PITCH_SHIFT + shift);
	*txsize |= ((l2w > l2h) ? l2w : l2h) << (R128_TEX_SIZE_SHIFT + shift);
	*txsize |= l2h << (R128_TEX_HEIGHT_SHIFT + shift);

	return TRUE;
}

Bool
R128PrepareComposite(int op, PicturePtr pSrcPicture, PicturePtr pMaskPicture,
    PicturePtr pDstPicture, PixmapPtr pSrc, PixmapPtr pMask, PixmapPtr pDst)
{
	KdScreenPriv(pDst->drawable.pScreen);
	ATIScreenInfo(pScreenPriv);
	CARD32 txsize = 0, prim_tex_cntl_c, sec_tex_cntl_c = 0, dstDatatype;
	CARD32 dst_pitch_offset, color_factor, in_color_factor;
	int i;
	RING_LOCALS;

	accel_atis = atis;

	if (pDstPicture->format == PICT_a8)
		dstDatatype = R128_DATATYPE_Y8;
	else
		R128GetDatatypePict(pDstPicture->format, &dstDatatype);

	if (!R128TextureSetup(pSrcPicture, pSrc, 0, &txsize, &prim_tex_cntl_c))
		return FALSE;
	if (pMask != NULL && !R128TextureSetup(pMaskPicture, pMask, 1, &txsize,
	    &sec_tex_cntl_c))
		return FALSE;

	if (!ATIGetPixmapOffsetPitch(pDst, &dst_pitch_offset))
		return FALSE;

	BEGIN_DMA(12);
	OUT_REG(R128_REG_SCALE_3D_CNTL,
	    R128_SCALE_3D_TEXMAP_SHADE |
	    R128_SCALE_PIX_REPLICATE |
	    R128_TEX_CACHE_SPLIT |
	    R128_TEX_MAP_ALPHA_IN_TEXTURE |
	    R128_TEX_CACHE_LINE_SIZE_4QW);
	OUT_REG(ATI_REG_DST_PITCH_OFFSET, dst_pitch_offset);
	OUT_REG(ATI_REG_DP_GUI_MASTER_CNTL,
	    ATI_GMC_DST_PITCH_OFFSET_CNTL |
	    ATI_GMC_BRUSH_SOLID_COLOR |
	    (dstDatatype << 8) |
	    ATI_GMC_SRC_DATATYPE_COLOR |
	    (ATIBltRop[GXcopy] << 16) |
	    ATI_DP_SRC_SOURCE_MEMORY |
	    R128_GMC_3D_FCN_EN |
	    ATI_GMC_CLR_CMP_CNTL_DIS |
	    R128_GMC_AUX_CLIP_DIS |
	    ATI_GMC_WR_MSK_DIS);
	OUT_REG(R128_REG_MISC_3D_STATE_CNTL,
	    R128_MISC_SCALE_3D_TEXMAP_SHADE |
	    R128_MISC_SCALE_PIX_REPLICATE |
	    R128_ALPHA_COMB_ADD_CLAMP |
	    R128BlendOp[op].blendctl);
	OUT_REG(R128_REG_TEX_CNTL_C,
	    R128_TEXMAP_ENABLE |
	    ((pMask != NULL) ? R128_SEC_TEXMAP_ENABLE : 0) |
	    R128_ALPHA_ENABLE |
	    R128_TEX_CACHE_FLUSH);
	OUT_REG(R128_REG_PC_GUI_CTLSTAT, R128_PC_FLUSH_GUI);
	END_DMA();

	/* IN operator: Without a mask, only the first texture unit is enabled.
	 * With a mask, we put the source in the first unit and have it pass
	 * through as input to the 2nd.  The 2nd unit takes the incoming source
	 * pixel and modulates it with either the alpha or each of the channels
	 * in the mask, depending on componentAlpha.
	 */
	BEGIN_DMA(15);
	/* R128_REG_PRIM_TEX_CNTL_C,
	 * R128_REG_PRIM_TEXTURE_COMBINE_CNTL_C,
	 * R128_REG_TEX_SIZE_PITCH_C,
	 * R128_REG_PRIM_TEX_0_OFFSET_C - R128_REG_PRIM_TEX_10_OFFSET_C
	 */
	OUT_RING(DMA_PACKET0(R128_REG_PRIM_TEX_CNTL_C, 14));
	OUT_RING(prim_tex_cntl_c);

	/* If this is the only stage and the dest is a8, route the alpha result 
	 * to the color (red channel, in particular), too.  Otherwise, be sure
	 * to zero out color channels of an a8 source.
	 */
	if (pMaskPicture == NULL && pDstPicture->format == PICT_a8)
		color_factor = R128_COLOR_FACTOR_ALPHA;
	else if (pSrcPicture->format == PICT_a8)
		color_factor = R128_COLOR_FACTOR_CONST_COLOR;
	else
		color_factor = R128_COLOR_FACTOR_TEX;

	OUT_RING(R128_COMB_COPY |
	    color_factor |
	    R128_INPUT_FACTOR_INT_COLOR |
	    R128_COMB_ALPHA_DIS |
	    R128_ALPHA_FACTOR_TEX_ALPHA |
	    R128_INP_FACTOR_A_INT_ALPHA);
	OUT_RING(txsize);
	/* We could save some output by only writing the offset register that
	 * will actually be used.  On the other hand, this is easy.
	 */
	for (i = 0; i <= 10; i++)
		OUT_RING(((CARD8 *)pSrc->devPrivate.ptr -
		    pScreenPriv->screen->memory_base));
	END_DMA();

	if (pMask != NULL) {
		BEGIN_DMA(14);
		/* R128_REG_SEC_TEX_CNTL_C,
		 * R128_REG_SEC_TEXTURE_COMBINE_CNTL_C,
		 * R128_REG_SEC_TEX_0_OFFSET_C - R128_REG_SEC_TEX_10_OFFSET_C
		 */
		OUT_RING(DMA_PACKET0(R128_REG_SEC_TEX_CNTL_C, 13));
		OUT_RING(sec_tex_cntl_c);

		/* XXX: Need to check with keithp to see if component alpha a8
		 * masks should act as non-CA, or if they should expand the RGB
		 * channels as 0.
		 */
		if (pDstPicture->format == PICT_a8) {
			color_factor = R128_COLOR_FACTOR_ALPHA;
			in_color_factor = R128_INPUT_FACTOR_PREV_ALPHA;
		} else if (pMaskPicture->componentAlpha) {
			color_factor = R128_COLOR_FACTOR_TEX;
			in_color_factor = R128_INPUT_FACTOR_PREV_COLOR;
		} else {
			color_factor = R128_COLOR_FACTOR_ALPHA;
			in_color_factor = R128_INPUT_FACTOR_PREV_COLOR;
		}

		OUT_RING(R128_COMB_MODULATE |
		    color_factor |
		    in_color_factor |
		    R128_COMB_ALPHA_MODULATE |
		    R128_ALPHA_FACTOR_TEX_ALPHA |
		    R128_INP_FACTOR_A_PREV_ALPHA);
		for (i = 0; i <= 10; i++)
			OUT_RING(((CARD8 *)pMask->devPrivate.ptr -
			    pScreenPriv->screen->memory_base));
		END_DMA();
	}

	return TRUE;
}

union intfloat {
	float f;
	CARD32 i;
};

struct blend_vertex {
	union intfloat x, y, z, w;
	union intfloat s0, t0;
	union intfloat s1, t1;
};

#define VTX_RING_COUNT 8

#define VTX_OUT(vtx)		\
do {				\
	OUT_RING(vtx.x.i);	\
	OUT_RING(vtx.y.i);	\
	OUT_RING(vtx.z.i);	\
	OUT_RING(vtx.w.i);	\
	OUT_RING(vtx.s0.i);	\
	OUT_RING(vtx.t0.i);	\
	OUT_RING(vtx.s1.i);	\
	OUT_RING(vtx.t1.i);	\
} while (0)

void
R128Composite(int srcX, int srcY, int maskX, int maskY, int dstX, int dstY,
    int w, int h)
{
	ATIScreenInfo *atis = accel_atis;
	struct blend_vertex vtx[4];
	int i;
	RING_LOCALS;

	/*ErrorF("R128Composite (%d,%d) (%d,%d) (%d,%d) (%d,%d)\n",
	    srcX, srcY, maskX, maskY,dstX, dstY, w, h);*/

	vtx[0].x.f = dstX;
	vtx[0].y.f = dstY;
	vtx[0].z.f = 0.0;
	vtx[0].w.f = 1.0;
	vtx[0].s0.f = srcX;
	vtx[0].t0.f = srcY;
	vtx[0].s1.f = maskX;
	vtx[0].t1.f = maskY;

	vtx[1].x.f = dstX;
	vtx[1].y.f = dstY + h;
	vtx[1].z.f = 0.0;
	vtx[1].w.f = 1.0;
	vtx[1].s0.f = srcX;
	vtx[1].t0.f = srcY + h;
	vtx[1].s1.f = maskX;
	vtx[1].t1.f = maskY + h;

	vtx[2].x.f = dstX + w;
	vtx[2].y.f = dstY + h;
	vtx[2].z.f = 0.0;
	vtx[2].w.f = 1.0;
	vtx[2].s0.f = srcX + w;
	vtx[2].t0.f = srcY + h;
	vtx[2].s1.f = maskX + w;
	vtx[2].t1.f = maskY + h;

	vtx[3].x.f = dstX + w;
	vtx[3].y.f = dstY;
	vtx[3].z.f = 0.0;
	vtx[3].w.f = 1.0;
	vtx[3].s0.f = srcX + w;
	vtx[3].t0.f = srcY;
	vtx[3].s1.f = maskX + w;
	vtx[3].t1.f = maskY;

	for (i = 0; i < 4; i++) {
		vtx[i].x.f += 0.0;
		vtx[i].y.f += 0.125;
		vtx[i].s0.f /= widths[0];
		vtx[i].t0.f /= heights[0];
		vtx[i].s1.f /= widths[1];
		vtx[i].t1.f /= heights[1];
	}

	BEGIN_DMA(3 + 4 * VTX_RING_COUNT);
	OUT_RING(DMA_PACKET3(ATI_CCE_PACKET3_3D_RNDR_GEN_PRIM,
	    2 + 4 * VTX_RING_COUNT));
	OUT_RING(R128_CCE_VC_FRMT_RHW |
	    R128_CCE_VC_FRMT_S_T |
	    R128_CCE_VC_FRMT_S2_T2);
	OUT_RING(R128_CCE_VC_CNTL_PRIM_TYPE_TRI_FAN |
	    R128_CCE_VC_CNTL_PRIM_WALK_RING |
	    (4 << R128_CCE_VC_CNTL_NUM_SHIFT));

	VTX_OUT(vtx[0]);
	VTX_OUT(vtx[1]);
	VTX_OUT(vtx[2]);
	VTX_OUT(vtx[3]);

	END_DMA();
}

void
R128DoneComposite(void)
{
}
