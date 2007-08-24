/*
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

#ifdef USE_DMA
#define TAG(x)		x##DMA
#define LOCALS		RING_LOCALS; \
			(void)atic
#define BEGIN(x)	BEGIN_RING(x * 2)
#define OUT_REG(reg, val) OUT_RING_REG(reg, val)
#define END()		ADVANCE_RING()
#else
#define TAG(x)		x##MMIO
#define LOCALS		char *mmio = atic->reg_base; \
			(void)atis
#define BEGIN(x)	ATIWaitAvailMMIO(x)
#define OUT_REG(reg, val) MMIO_OUT32((mmio), (reg), (val))
#define END()
#endif

static Bool
TAG(ATISetup)(PixmapPtr pDst, PixmapPtr pSrc)
{
	KdScreenPriv(pDst->drawable.pScreen);
	ATIScreenInfo(pScreenPriv);
	ATICardInfo(pScreenPriv);
	int dst_offset, dst_pitch;
	int bpp = pDst->drawable.bitsPerPixel;
	LOCALS;

	accel_atis = atis;

	dst_pitch = pDst->devKind;
	dst_offset = ((CARD8 *)pDst->devPrivate.ptr -
	    pScreenPriv->screen->memory_base);
	if ((dst_pitch & (atis->kaa.offscreenPitch - 1)) != 0)
		ATI_FALLBACK(("Bad dst pitch 0x%x\n", dst_pitch));
	if ((dst_offset & (atis->kaa.offscreenByteAlign - 1)) != 0)
		ATI_FALLBACK(("Bad dst offset 0x%x\n", dst_offset));

	if (pSrc != NULL) {
		src_pitch = pSrc->devKind;
		src_offset = ((CARD8 *)pSrc->devPrivate.ptr -
		    pScreenPriv->screen->memory_base);
		if ((src_pitch & (atis->kaa.offscreenPitch - 1)) != 0)
			ATI_FALLBACK(("Bad src pitch 0x%x\n", src_pitch));
		if ((src_offset & (atis->kaa.offscreenByteAlign - 1)) != 0)
			ATI_FALLBACK(("Bad src offset 0x%x\n", src_offset));
	}

#ifdef USE_DMA
	if (atic->is_radeon && !atic->is_r200)
		RadeonSwitchTo2D();
#endif
	BEGIN((pSrc != NULL) ? 3 : 2);
	if (atic->is_radeon) {
		OUT_REG(RADEON_REG_DST_PITCH_OFFSET,
		    ((dst_pitch >> 6) << 22) | (dst_offset >> 10));
		if (pSrc != NULL) {
			OUT_REG(RADEON_REG_SRC_PITCH_OFFSET,
			    ((src_pitch >> 6) << 22) | (src_offset >> 10));
		}
	} else {
		if (is_24bpp) {
			dst_pitch *= 3;
			src_pitch *= 3;
		}
		/* R128 pitch is in units of 8 pixels, offset in 32 bytes */
		OUT_REG(RADEON_REG_DST_PITCH_OFFSET,
		    ((dst_pitch/bpp) << 21) | (dst_offset >> 5));
		if (pSrc != NULL) {
			OUT_REG(RADEON_REG_SRC_PITCH_OFFSET,
			    ((src_pitch/bpp) << 21) | (src_offset >> 5));
		}
	}
	OUT_REG(RADEON_REG_DEFAULT_SC_BOTTOM_RIGHT,
	    (RADEON_DEFAULT_SC_RIGHT_MAX | RADEON_DEFAULT_SC_BOTTOM_MAX));
	END();

	return TRUE;
}

static Bool
TAG(ATIPrepareSolid)(PixmapPtr pPixmap, int alu, Pixel pm, Pixel fg)
{
	KdScreenPriv(pPixmap->drawable.pScreen);
	ATIScreenInfo(pScreenPriv);
	ATICardInfo(pScreenPriv);
	CARD32 datatype;
	LOCALS;

	if (is_24bpp) {
		/* Solid fills in fake-24bpp mode only work if the pixel color
		 * and planemask are all the same byte.
		 */
		if ((fg & 0xffffff) != (((fg & 0xff) << 16) | ((fg >> 8) &
		    0xffff)))
			ATI_FALLBACK(("Can't do solid color %d in 24bpp\n"));
		if ((pm & 0xffffff) != (((pm & 0xff) << 16) | ((pm >> 8) &
		    0xffff)))
			ATI_FALLBACK(("Can't do planemask %d in 24bpp\n"));
	}

	if (!ATIGetDatatypeBpp(pPixmap->drawable.bitsPerPixel, &datatype))
		return FALSE;
	if (!TAG(ATISetup)(pPixmap, NULL))
		return FALSE;

	BEGIN(4);
	OUT_REG(RADEON_REG_DP_GUI_MASTER_CNTL,
	    (datatype << 8) |
	    RADEON_GMC_CLR_CMP_CNTL_DIS |
	    RADEON_GMC_AUX_CLIP_DIS |
	    RADEON_GMC_BRUSH_SOLID_COLOR |
	    RADEON_GMC_DST_PITCH_OFFSET_CNTL |
	    RADEON_GMC_SRC_DATATYPE_COLOR |
	    (ATISolidRop[alu] << 16));
	OUT_REG(RADEON_REG_DP_BRUSH_FRGD_CLR, fg);
	OUT_REG(RADEON_REG_DP_WRITE_MASK, pm);
	OUT_REG(RADEON_REG_DP_CNTL, RADEON_DST_X_LEFT_TO_RIGHT |
	    RADEON_DST_Y_TOP_TO_BOTTOM);
	END();

	return TRUE;
}

static void
TAG(ATISolid)(int x1, int y1, int x2, int y2)
{
	ATIScreenInfo *atis = accel_atis;
	ATICardInfo *atic = atis->atic;
	LOCALS;
	
	if (is_24bpp) {
		x1 *= 3;
		x2 *= 3;
	}
	BEGIN(2);
	OUT_REG(RADEON_REG_DST_Y_X, (y1 << 16) | x1);
	OUT_REG(RADEON_REG_DST_WIDTH_HEIGHT, ((x2 - x1) << 16) | (y2 - y1));
	END();
}

static Bool
TAG(ATIPrepareCopy)(PixmapPtr pSrc, PixmapPtr pDst, int dx, int dy, int alu, Pixel pm)
{
	KdScreenPriv(pDst->drawable.pScreen);
	ATIScreenInfo(pScreenPriv);
	ATICardInfo(pScreenPriv);
	CARD32 datatype;
	LOCALS;

	copydx = dx;
	copydy = dy;

	if (is_24bpp && ((pm & 0xffffff) != (((pm & 0xff) << 16) | ((pm >> 8) &
	    0xffff))))
		ATI_FALLBACK(("Can't do planemask %d in 24bpp\n"));

	if (!ATIGetDatatypeBpp(pDst->drawable.bitsPerPixel, &datatype))
		return FALSE;
	if (!TAG(ATISetup)(pDst, pSrc))
		return FALSE;

	BEGIN(3);
	OUT_REG(RADEON_REG_DP_GUI_MASTER_CNTL,
	    (datatype << 8) |
	    RADEON_GMC_CLR_CMP_CNTL_DIS |
	    RADEON_GMC_AUX_CLIP_DIS |
	    RADEON_GMC_BRUSH_SOLID_COLOR |
	    RADEON_GMC_SRC_DATATYPE_COLOR |
	    (ATIBltRop[alu] << 16) |
	    RADEON_GMC_SRC_PITCH_OFFSET_CNTL |
	    RADEON_GMC_DST_PITCH_OFFSET_CNTL |
	    RADEON_DP_SRC_SOURCE_MEMORY);
	OUT_REG(RADEON_REG_DP_WRITE_MASK, pm);
	OUT_REG(RADEON_REG_DP_CNTL, 
	    (dx >= 0 ? RADEON_DST_X_LEFT_TO_RIGHT : 0) |
	    (dy >= 0 ? RADEON_DST_Y_TOP_TO_BOTTOM : 0));
	END();

	return TRUE;
}

static void
TAG(ATICopy)(int srcX, int srcY, int dstX, int dstY, int w, int h)
{
	ATIScreenInfo *atis = accel_atis;
	ATICardInfo *atic = atis->atic;
	LOCALS;

	if (is_24bpp) {
		srcX *= 3;
		dstX *= 3;
		w *= 3;
	}

	if (copydx < 0) {
		srcX += w - 1;
		dstX += w - 1;
	}

	if (copydy < 0)  {
		srcY += h - 1;
		dstY += h - 1;
	}

	BEGIN(3);
	OUT_REG(RADEON_REG_SRC_Y_X, (srcY << 16) | srcX);
	OUT_REG(RADEON_REG_DST_Y_X, (dstY << 16) | dstX);
	OUT_REG(RADEON_REG_DST_HEIGHT_WIDTH, (h << 16) | w);
	END();
}

#undef TAG
#undef LOCALS
#undef BEGIN
#undef OUT_REG
#undef END
