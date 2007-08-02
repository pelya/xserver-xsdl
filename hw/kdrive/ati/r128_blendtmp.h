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

#ifdef USE_DMA
#define TAG(x)		x##DMA
#define LOCALS		RING_LOCALS; \
			(void)atic
#define BEGIN(x)	BEGIN_RING(x * 2)
#define OUT_REG(reg, val) OUT_RING_REG(reg, val)
#define END()		ADVANCE_RING()
#else
#define TAG(x)		x##MMIO
#define LOCALS		char *mmio = atic->reg_base
#define BEGIN(x)	ATIWaitAvailMMIO(x)
#define OUT_REG(reg, val) MMIO_OUT32(mmio, (reg), (val))
#define END()
#endif

static Bool
TAG(R128PrepareBlend)(int op, PicturePtr pSrcPicture, PicturePtr pDstPicture,
    PixmapPtr pSrc, PixmapPtr pDst)
{
	KdScreenPriv(pDst->drawable.pScreen);
	ATIScreenInfo(pScreenPriv);
	ATICardInfo(pScreenPriv);
	CARD32 dstDatatype, srcDatatype;
	LOCALS;

	accel_atis = atis;

	if (!TAG(ATISetup)(pDst, pSrc))
		return FALSE;

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

	BEGIN(11);
	OUT_REG(RADEON_REG_DP_GUI_MASTER_CNTL,
	    (dstDatatype << 8) |
	    RADEON_GMC_SRC_DATATYPE_COLOR |
	    RADEON_GMC_DST_PITCH_OFFSET_CNTL |
	    RADEON_GMC_CLR_CMP_CNTL_DIS |
	    RADEON_GMC_AUX_CLIP_DIS |
	    (ATIBltRop[3] << 16) |
	    RADEON_GMC_3D_FCN_EN);
	OUT_REG(R128_REG_TEX_CNTL_C, R128_TEX_ALPHA_EN | R128_TEX_CACHE_FLUSH);
	OUT_REG(R128_REG_PRIM_TEXTURE_COMBINE_CNTL_C, 0);
	OUT_REG(R128_REG_SCALE_3D_CNTL,
	    R128_SCALE_3D_SCALE |
	    R128BlendOp[op] |
	    R128_TEX_MAP_ALPHA_IN_TEXTURE);
	OUT_REG(R128_REG_SCALE_3D_DATATYPE, srcDatatype);
	OUT_REG(R128_REG_SCALE_PITCH, src_pitch / src_bpp);
	/* 4.16 fixed point scaling factor? */
	if (is_repeat) {
		OUT_REG(R128_REG_SCALE_X_INC, 0);
		OUT_REG(R128_REG_SCALE_Y_INC, 0);
	} else {
		OUT_REG(R128_REG_SCALE_X_INC, 65536);
		OUT_REG(R128_REG_SCALE_Y_INC, 65536);
	}
	OUT_REG(R128_REG_SCALE_HACC, 0x00000000);
	OUT_REG(R128_REG_SCALE_VACC, 0x00000000);
	OUT_REG(RADEON_REG_DP_CNTL, 
	    RADEON_DST_X_LEFT_TO_RIGHT | RADEON_DST_Y_TOP_TO_BOTTOM );
	END();

	return TRUE;
}

static void
TAG(R128Blend)(int srcX, int srcY, int dstX, int dstY, int width, int height)
{
	ATIScreenInfo *atis = accel_atis;
	ATICardInfo *atic = atis->atic;
	LOCALS;

	if (is_repeat) {
		srcX = 0;
		srcY = 0;
	}

	BEGIN(4);
	OUT_REG(R128_REG_SCALE_OFFSET_0, src_offset + srcY * src_pitch + srcX *
	    (src_bpp >> 3));
	OUT_REG(R128_REG_SCALE_SRC_HEIGHT_WIDTH, (height << 16) | width);
	OUT_REG(R128_REG_SCALE_DST_X_Y, (dstX << 16) | dstY);
	OUT_REG(R128_REG_SCALE_DST_HEIGHT_WIDTH, (height << 16) | width);
	END();
}

static void
TAG(R128DoneBlend)(void)
{
}

#undef TAG
#undef LOCALS
#undef BEGIN
#undef OUT_REG
#undef END
