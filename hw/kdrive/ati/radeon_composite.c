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
#include "ati_draw.h"
#include "radeon_common.h"
#include "r128_common.h"
#include "ati_sarea.h"

#define TAG(x)		x##DMA
#define LOCALS		RING_LOCALS; \
			(void)atic;
#define BEGIN(x)	BEGIN_RING(x * 2)
#define OUT_REG(reg, val) OUT_RING_REG(reg, val)
#define END()		ADVANCE_RING()

extern ATIScreenInfo *accel_atis;

static CARD32 RadeonBlendOp[] = {
	/* Clear */
	RADEON_SRC_BLEND_GL_ZERO		| RADEON_DST_BLEND_GL_ZERO,
	/* Src */
	RADEON_SRC_BLEND_GL_ONE			| RADEON_DST_BLEND_GL_ZERO,
	/* Dst */
	RADEON_SRC_BLEND_GL_ZERO		| RADEON_DST_BLEND_GL_ONE,
	/* Over */
	RADEON_SRC_BLEND_GL_ONE			| RADEON_DST_BLEND_GL_ONE_MINUS_SRC_ALPHA,
	/* OverReverse */
	RADEON_SRC_BLEND_GL_ONE_MINUS_DST_ALPHA	| RADEON_DST_BLEND_GL_ONE,
	/* In */
	RADEON_SRC_BLEND_GL_DST_ALPHA		| RADEON_DST_BLEND_GL_ZERO,
	/* InReverse */
	RADEON_SRC_BLEND_GL_ZERO		| RADEON_DST_BLEND_GL_SRC_ALPHA,
	/* Out */
	RADEON_SRC_BLEND_GL_ONE_MINUS_DST_ALPHA	| RADEON_DST_BLEND_GL_ZERO,
	/* OutReverse */
	RADEON_SRC_BLEND_GL_ZERO		| RADEON_DST_BLEND_GL_ONE_MINUS_SRC_ALPHA,
	/* Atop */
	RADEON_SRC_BLEND_GL_DST_ALPHA		| RADEON_DST_BLEND_GL_ONE_MINUS_SRC_ALPHA,
	/* AtopReverse */
	RADEON_SRC_BLEND_GL_ONE_MINUS_DST_ALPHA	| RADEON_DST_BLEND_GL_SRC_ALPHA,
	/* Xor */
	RADEON_SRC_BLEND_GL_ONE_MINUS_DST_ALPHA	| RADEON_DST_BLEND_GL_ONE_MINUS_SRC_ALPHA,
	/* Add */
	RADEON_SRC_BLEND_GL_ONE			| RADEON_DST_BLEND_GL_ONE,
	/* Saturate */
	RADEON_SRC_BLEND_GL_SRC_ALPHA_SATURATE	| RADEON_DST_BLEND_GL_ONE,
	/* DisjointClear */
	RADEON_SRC_BLEND_GL_ZERO		| RADEON_DST_BLEND_GL_ZERO,
	/* DisjointSrc */
	RADEON_SRC_BLEND_GL_ONE			| RADEON_DST_BLEND_GL_ZERO,
	/* DisjointDst */
	RADEON_SRC_BLEND_GL_ZERO		| RADEON_DST_BLEND_GL_ONE,
};

static Bool
RadeonTextureSetup(PicturePtr pPict, PixmapPtr pPix, int unit)
{
	ATIScreenInfo *atis = accel_atis;
	ATICardInfo *atic = atis->atic;
	KdScreenPriv(pPix->drawable.pScreen);
	CARD32 txformat, txoffset, txpitch;
	int w = pPict->pDrawable->width;
	int h = pPict->pDrawable->height;
	LOCALS;

	if ((w > 0x7ff) || (h > 0x7ff))
		ATI_FALLBACK(("Picture w/h too large (%dx%d)\n", w, h));

	switch (pPict->format) {
	case PICT_a8r8g8b8:
		txformat = RADEON_TXFORMAT_ARGB8888 |
		    RADEON_TXFORMAT_ALPHA_IN_MAP;
		break;
	case PICT_x8r8g8b8:
		txformat = RADEON_TXFORMAT_ARGB8888;
		break;
	case PICT_r5g6b5:
		txformat = RADEON_TXFORMAT_RGB565;
		break;
	case PICT_a8:
		txformat = RADEON_TXFORMAT_I8 | RADEON_TXFORMAT_ALPHA_IN_MAP;
		break;
	default:
		ATI_FALLBACK(("Unsupported picture format 0x%x\n",
		    pPict->format));
	}
	if (pPict->repeat) {
		if ((w & (w - 1)) != 0 || (h & (h - 1)) != 0)
			ATI_FALLBACK(("NPOT repeat unsupported (%dx%d)\n", w,
			    h));

		txformat |= (ATILog2(w) - 1) << RADEON_TXFORMAT_WIDTH_SHIFT;
		txformat |= (ATILog2(h) - 1) << RADEON_TXFORMAT_HEIGHT_SHIFT;
	} else 
		txformat |= RADEON_TXFORMAT_NON_POWER2;
	txformat |= unit << 24; /* RADEON_TXFORMAT_ST_ROUTE_STQX */

	txpitch = pPix->devKind;
	txoffset = ((CARD8 *)pPix->devPrivate.ptr -
	    pScreenPriv->screen->memory_base);

	if ((txoffset & 0x1f) != 0)
		ATI_FALLBACK(("Bad texture offset 0x%x\n", txoffset));
	if ((txpitch & 0x1f) != 0)
		ATI_FALLBACK(("Bad texture pitch 0x%x\n", txpitch));

	/* RADEON_REG_PP_TXFILTER_0, RADEON_REG_PP_TXFORMAT_0,
	 * RADEON_REG_PP_TXOFFSET_0
	 */
	BEGIN_RING(4);
	OUT_RING(DMA_PACKET0(RADEON_REG_PP_TXFILTER_0 + 0x18 * unit, 2));
	OUT_RING(0);
	OUT_RING(txformat);
	OUT_RING(txoffset);
	ADVANCE_RING();

	/* RADEON_REG_PP_TEX_SIZE_0, RADEON_REG_PP_TEX_PITCH_0 */
	BEGIN_RING(3);
	OUT_RING(DMA_PACKET0(RADEON_REG_PP_TEX_SIZE_0 + 0x8 * unit, 1));
	OUT_RING((pPix->drawable.width - 1) |
	    ((pPix->drawable.height - 1) << RADEON_TEX_VSIZE_SHIFT));
	OUT_RING(txpitch - 32);
	ADVANCE_RING();

	return TRUE;
}

Bool
RadeonPrepareComposite(int op, PicturePtr pSrcPicture, PicturePtr pMaskPicture,
    PicturePtr pDstPicture, PixmapPtr pSrc, PixmapPtr pMask, PixmapPtr pDst)
{
	KdScreenPriv(pDst->drawable.pScreen);
	ATIScreenInfo(pScreenPriv);
	ATICardInfo(pScreenPriv);
	CARD32 dst_format, dst_offset, dst_pitch;
	CARD32 pp_cntl;
	int pixel_shift;
	LOCALS;

	/*ErrorF("RadeonPrepareComposite\n");*/
	
	/* Check for unsupported compositing operations. */
	if (op >= sizeof(RadeonBlendOp) / sizeof(RadeonBlendOp[0]))
		ATI_FALLBACK(("Unsupported Composite op 0x%x\n", op));
	if (pSrcPicture->transform)
		ATI_FALLBACK(("Source transform unsupported.\n"));
	if (pMaskPicture && pMaskPicture->transform)
		ATI_FALLBACK(("Mask transform unsupported.\n"));

	accel_atis = atis;

	switch (pDstPicture->format) {
	case PICT_r5g6b5:
		dst_format = RADEON_COLOR_FORMAT_RGB565;
		pixel_shift = 1;
		break;
	case PICT_a1r5g5b5:
		dst_format = RADEON_COLOR_FORMAT_ARGB1555;
		pixel_shift = 1;
		break;
	case PICT_a8r8g8b8:
	case PICT_x8r8g8b8:
		dst_format = RADEON_COLOR_FORMAT_ARGB8888;
		pixel_shift = 2;
		break;
	default:
		ATI_FALLBACK(("Unsupported dest format 0x%x\n",
		    pDstPicture->format));
	}

	dst_offset = ((CARD8 *)pDst->devPrivate.ptr -
	    pScreenPriv->screen->memory_base);
	dst_pitch = pDst->devKind;
	if ((dst_offset & 0xff) != 0)
		ATI_FALLBACK(("Bad destination offset 0x%x\n", dst_offset));
	if (((dst_pitch >> pixel_shift) & 0x7) != 0)
		ATI_FALLBACK(("Bad destination pitch 0x%x\n", dst_pitch));

	if (!RadeonTextureSetup(pSrcPicture, pSrc, 0))
		return FALSE;
	pp_cntl = RADEON_TEX_0_ENABLE | RADEON_TEX_BLEND_0_ENABLE;

	if (pMask != NULL) {
		if (!RadeonTextureSetup(pMaskPicture, pMask, 1))
			return FALSE;
		pp_cntl |= RADEON_TEX_1_ENABLE;
	}

	BEGIN(2);
	OUT_REG(RADEON_REG_RE_TOP_LEFT, 0);
	OUT_REG(RADEON_REG_RE_WIDTH_HEIGHT, 0xffffffff);
	END();

	BEGIN(11);
	OUT_REG(RADEON_REG_PP_CNTL, pp_cntl);
	OUT_REG(RADEON_REG_RB3D_CNTL, dst_format | RADEON_ALPHA_BLEND_ENABLE);
	/* COLORPITCH is multiples of 8 pixels, but located at the 3rd bit */
	OUT_REG(RADEON_REG_RB3D_COLORPITCH, (dst_pitch >> pixel_shift));
	OUT_REG(RADEON_REG_RB3D_COLOROFFSET, dst_offset);
	OUT_REG(RADEON_REG_RB3D_PLANEMASK, 0xffffffff);
	OUT_REG(RADEON_REG_SE_CNTL, RADEON_FFACE_CULL_CCW | RADEON_FFACE_SOLID |
	    RADEON_VTX_PIX_CENTER_OGL);
	OUT_REG(RADEON_REG_SE_CNTL_STATUS, RADEON_TCL_BYPASS);
	OUT_REG(RADEON_REG_SE_COORD_FMT,
	    RADEON_VTX_XY_PRE_MULT_1_OVER_W0 |
	    RADEON_VTX_ST0_NONPARAMETRIC |
	    RADEON_VTX_ST1_NONPARAMETRIC |
	    RADEON_TEX1_W_ROUTING_USE_W0);
	if (pMask != NULL) {
		/* IN operator: Multiply src by mask components or mask alpha.
		 * BLEND_CTL_ADD is A * B + C
		 */
		if (pMaskPicture->componentAlpha)
			OUT_REG(RADEON_REG_PP_TXCBLEND_0,
			    RADEON_COLOR_ARG_A_T0_COLOR |
			    RADEON_COLOR_ARG_B_T1_COLOR |
			    RADEON_COLOR_ARG_C_ZERO |
			    RADEON_BLEND_CTL_ADD |
			    RADEON_CLAMP_TX);
		else
			OUT_REG(RADEON_REG_PP_TXCBLEND_0,
			    RADEON_COLOR_ARG_A_T0_COLOR |
			    RADEON_COLOR_ARG_B_T1_ALPHA |
			    RADEON_COLOR_ARG_C_ZERO |
			    RADEON_BLEND_CTL_ADD |
			    RADEON_CLAMP_TX);
		OUT_REG(RADEON_REG_PP_TXABLEND_0,
		    RADEON_ALPHA_ARG_A_T0_ALPHA |
		    RADEON_ALPHA_ARG_B_T1_ALPHA |
		    RADEON_ALPHA_ARG_C_ZERO |
		    RADEON_BLEND_CTL_ADD);
	} else {
		OUT_REG(RADEON_REG_PP_TXCBLEND_0,
		    RADEON_COLOR_ARG_A_ZERO |
		    RADEON_COLOR_ARG_B_ZERO |
		    RADEON_COLOR_ARG_C_T0_COLOR |
		    RADEON_BLEND_CTL_ADD |
		    RADEON_CLAMP_TX);
		OUT_REG(RADEON_REG_PP_TXABLEND_0,
		    RADEON_ALPHA_ARG_A_ZERO |
		    RADEON_ALPHA_ARG_B_ZERO |
		    RADEON_ALPHA_ARG_C_T0_ALPHA |
		    RADEON_BLEND_CTL_ADD);
	}
	/* Op operator. */
	OUT_REG(RADEON_RB3D_BLENDCNTL, RadeonBlendOp[op]);
	END();

	RadeonSwitchTo3D();

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

#define VTX_REG_COUNT 6

#define VTX_OUT(vtx)		\
do {				\
	OUT_RING(vtx.x.i);	\
	OUT_RING(vtx.y.i);	\
	OUT_RING(vtx.s0.i);	\
	OUT_RING(vtx.t0.i);	\
	OUT_RING(vtx.s1.i);	\
	OUT_RING(vtx.t1.i);	\
	/*ErrorF("%f,%f %f,%f %f,%f\n", vtx.x.f, vtx.y.f, vtx.s0.f, \
	    vtx.t0.f, vtx.s1.f, vtx.t1.f);*/ \
} while (0)

void
RadeonComposite(int srcX, int srcY, int maskX, int maskY, int dstX, int dstY,
    int w, int h)
{
	ATIScreenInfo *atis = accel_atis;
	ATICardInfo *atic = atis->atic;
	struct blend_vertex vtx[4];
	LOCALS;

	/*ErrorF("RadeonComposite %d %d %d %d %d %d\n", srcX, srcY, maskX, maskY,
	    dstX, dstY, w, h);*/
	BEGIN_RING(3 + 4 * VTX_REG_COUNT);
	OUT_RING(RADEON_CP_PACKET3_3D_DRAW_IMMD |
	    ((4 * VTX_REG_COUNT + 1) << 16));
	OUT_RING(RADEON_CP_VC_FRMT_XY |
	    RADEON_CP_VC_FRMT_ST0 |
	    RADEON_CP_VC_FRMT_ST1);
	OUT_RING(RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_FAN |
	    RADEON_CP_VC_CNTL_PRIM_WALK_RING |
	    RADEON_CP_VC_CNTL_MAOS_ENABLE |
	    RADEON_CP_VC_CNTL_VTX_FMT_RADEON_MODE |
	    (4 << RADEON_CP_VC_CNTL_NUM_SHIFT));

	vtx[0].x.f = dstX;
	vtx[0].y.f = dstY;
	vtx[0].s0.f = srcX;
	vtx[0].t0.f = srcY;
	vtx[0].s1.f = maskX;
	vtx[0].t1.f = maskY;

	vtx[1].x.f = dstX;
	vtx[1].y.f = dstY + h;
	vtx[1].s0.f = srcX;
	vtx[1].t0.f = srcY + h;
	vtx[1].s1.f = maskX;
	vtx[1].t1.f = maskY + h;

	vtx[2].x.f = dstX + w;
	vtx[2].y.f = dstY + h;
	vtx[2].s0.f = srcX + w;
	vtx[2].t0.f = srcY + h;
	vtx[2].s1.f = maskX + w;
	vtx[2].t1.f = maskY + h;

	vtx[3].x.f = dstX + w;
	vtx[3].y.f = dstY;
	vtx[3].s0.f = srcX + w;
	vtx[3].t0.f = srcY;
	vtx[3].s1.f = maskX + w;
	vtx[3].t1.f = maskY;

	VTX_OUT(vtx[0]);
	VTX_OUT(vtx[1]);
	VTX_OUT(vtx[2]);
	VTX_OUT(vtx[3]);

	ADVANCE_RING();
}

void
RadeonDoneComposite(void)
{
	/*ErrorF("RadeonDoneComposite\n");*/
}

Bool
RadeonPrepareBlend(int op, PicturePtr pSrcPicture, PicturePtr pDstPicture,
    PixmapPtr pSrc, PixmapPtr pDst)
{
	return RadeonPrepareComposite(op, pSrcPicture, NULL, pDstPicture, pSrc,
	    NULL, pDst);
}

void
RadeonBlend(int srcX, int srcY, int dstX, int dstY, int width, int height)
{
	RadeonComposite(srcX, srcY, 0, 0, dstX, dstY, width, height);
}

void
RadeonDoneBlend(void)
{
	RadeonDoneComposite();
}
