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
#include <sys/io.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "ati.h"
#include "ati_reg.h"

CARD8 ATISolidRop[16] = {
    /* GXclear      */      0x00,         /* 0 */
    /* GXand        */      0xa0,         /* src AND dst */
    /* GXandReverse */      0x50,         /* src AND NOT dst */
    /* GXcopy       */      0xf0,         /* src */
    /* GXandInverted*/      0x0a,         /* NOT src AND dst */
    /* GXnoop       */      0xaa,         /* dst */
    /* GXxor        */      0x5a,         /* src XOR dst */
    /* GXor         */      0xfa,         /* src OR dst */
    /* GXnor        */      0x05,         /* NOT src AND NOT dst */
    /* GXequiv      */      0xa5,         /* NOT src XOR dst */
    /* GXinvert     */      0x55,         /* NOT dst */
    /* GXorReverse  */      0xf5,         /* src OR NOT dst */
    /* GXcopyInverted*/     0x0f,         /* NOT src */
    /* GXorInverted */      0xaf,         /* NOT src OR dst */
    /* GXnand       */      0x5f,         /* NOT src OR NOT dst */
    /* GXset        */      0xff,         /* 1 */
};

CARD8 ATIBltRop[16] = {
    /* GXclear      */      0x00,         /* 0 */
    /* GXand        */      0x88,         /* src AND dst */
    /* GXandReverse */      0x44,         /* src AND NOT dst */
    /* GXcopy       */      0xcc,         /* src */
    /* GXandInverted*/      0x22,         /* NOT src AND dst */
    /* GXnoop       */      0xaa,         /* dst */
    /* GXxor        */      0x66,         /* src XOR dst */
    /* GXor         */      0xee,         /* src OR dst */
    /* GXnor        */      0x11,         /* NOT src AND NOT dst */
    /* GXequiv      */      0x99,         /* NOT src XOR dst */
    /* GXinvert     */      0x55,         /* NOT dst */
    /* GXorReverse  */      0xdd,         /* src OR NOT dst */
    /* GXcopyInverted*/     0x33,         /* NOT src */
    /* GXorInverted */      0xbb,         /* NOT src OR dst */
    /* GXnand       */      0x77,         /* NOT src OR NOT dst */
    /* GXset        */      0xff,         /* 1 */
};

int copydx, copydy;
Bool is_radeon;
/* If is_24bpp is set, then we are using the accelerator in 8-bit mode due
 * to it being broken for 24bpp, so coordinates have to be multiplied by 3.
 */
Bool is_24bpp;
int fifo_size;
char *mmio;
CARD32 bltCmd;

static void
ATIWaitAvail(int n)
{
	if (fifo_size >= n) {
		fifo_size -= n;
		return;
	}
	if (is_radeon) {
		fifo_size = MMIO_IN32(mmio, RADEON_REG_RBBM_STATUS) &
		    RADEON_RBBM_FIFOCNT_MASK;
		while (fifo_size < n) {
			usleep(1);
			fifo_size = MMIO_IN32(mmio, RADEON_REG_RBBM_STATUS) &
			    RADEON_RBBM_FIFOCNT_MASK;
		}
	} else {
		fifo_size = MMIO_IN32(mmio, R128_REG_GUI_STAT) & 0xfff;
		while (fifo_size < n) {
			fifo_size = MMIO_IN32(mmio, R128_REG_GUI_STAT) & 0xfff;
			usleep(1);
		}
	}
	fifo_size -= n;
}

static void
RadeonWaitIdle(void)
{
	CARD32 temp;
	/* Wait for the engine to go idle */
	ATIWaitAvail(64);

	while ((MMIO_IN32(mmio, RADEON_REG_RBBM_STATUS) &
	    RADEON_RBBM_ACTIVE) != 0)
		usleep(1);

	/* Flush pixel cache */
	temp = MMIO_IN32(mmio, RADEON_REG_RB2D_DSTCACHE_CTLSTAT);
	temp |= RADEON_RB2D_DC_FLUSH_ALL;
	MMIO_OUT32(mmio, RADEON_REG_RB2D_DSTCACHE_CTLSTAT, temp);

	while ((MMIO_IN32(mmio, RADEON_REG_RB2D_DSTCACHE_CTLSTAT) &
	    RADEON_RB2D_DC_BUSY) != 0)
		usleep(1);
}

static void
R128WaitIdle(void)
{
	CARD32 temp;
	int tries;

	ATIWaitAvail(64);

	tries = 1000000;
	while (tries--) {
		if ((MMIO_IN32(mmio, R128_REG_GUI_STAT) & R128_GUI_ACTIVE) == 0)
			break;
	}

	temp = MMIO_IN32(mmio, R128_REG_PC_NGUI_CTLSTAT);
	MMIO_OUT32(mmio, R128_REG_PC_NGUI_CTLSTAT, temp | 0xff);

	tries = 1000000;
	while (tries--) {
		if ((MMIO_IN32(mmio, R128_REG_PC_NGUI_CTLSTAT) & R128_PC_BUSY) !=
		    R128_PC_BUSY)
			break;
	}
}

static void
ATIWaitIdle(void)
{
	if (is_radeon)
		RadeonWaitIdle();
	else
		R128WaitIdle();
}

static Bool
ATISetup(ScreenPtr pScreen, PixmapPtr pDst, PixmapPtr pSrc)
{
	KdScreenPriv(pScreen);
	ATICardInfo(pScreenPriv);
	int dst_offset, dst_pitch, src_offset = 0, src_pitch = 0;
	int bpp = pScreenPriv->screen->fb[0].bitsPerPixel;

	mmio = atic->reg_base;

	/* No acceleration for other formats (yet) */
	if (pDst->drawable.bitsPerPixel != bpp)
		return FALSE;

	dst_pitch = pDst->devKind;
	dst_offset = ((CARD8 *)pDst->devPrivate.ptr -
	    pScreenPriv->screen->memory_base);
	if (pSrc != NULL) {
		src_pitch = pSrc->devKind;
		src_offset = ((CARD8 *)pSrc->devPrivate.ptr -
		    pScreenPriv->screen->memory_base);
	}

	ATIWaitAvail(3);
	if (is_radeon) {
		MMIO_OUT32(mmio, RADEON_REG_DST_PITCH_OFFSET,
		    ((dst_pitch >> 6) << 22) | (dst_offset >> 10));
		if (pSrc != NULL) {
			MMIO_OUT32(mmio, RADEON_REG_SRC_PITCH_OFFSET,
			    ((src_pitch >> 6) << 22) | (src_offset >> 10));
		}
	} else {
		if (is_24bpp) {
			dst_pitch *= 3;
			src_pitch *= 3;
		}
		/* R128 pitch is in units of 8 pixels, offset in 32 bytes */
		MMIO_OUT32(mmio, RADEON_REG_DST_PITCH_OFFSET,
		    ((dst_pitch/bpp) << 21) | (dst_offset >> 5));
		if (pSrc != NULL) {
			MMIO_OUT32(mmio, RADEON_REG_SRC_PITCH_OFFSET,
			    ((src_pitch/bpp) << 21) | (src_offset >> 5));
		}
	}
	MMIO_OUT32(mmio, RADEON_REG_DEFAULT_SC_BOTTOM_RIGHT,
	    (RADEON_DEFAULT_SC_RIGHT_MAX | RADEON_DEFAULT_SC_BOTTOM_MAX));

	return TRUE;
}

static Bool
ATIPrepareSolid(PixmapPtr pPixmap, int alu, Pixel pm, Pixel fg)
{
	KdScreenPriv(pPixmap->drawable.pScreen);
	ATIScreenInfo(pScreenPriv);

	if (is_24bpp) {
		if (pm != 0xffffffff)
			return FALSE;
		/* Solid fills in fake-24bpp mode only work if the pixel color
		 * is all the same byte.
		 */
		if ((fg & 0xffffff) != (((fg & 0xff) << 16) | ((fg >> 8) &
		    0xffff)))
			return FALSE;
	}

	if (!ATISetup(pPixmap->drawable.pScreen, pPixmap, NULL))
		return FALSE;

	ATIWaitAvail(4);
	MMIO_OUT32(mmio, RADEON_REG_DP_GUI_MASTER_CNTL,
	    atis->dp_gui_master_cntl |
	    RADEON_GMC_BRUSH_SOLID_COLOR |
	    RADEON_GMC_DST_PITCH_OFFSET_CNTL |
	    RADEON_GMC_SRC_DATATYPE_COLOR |
	    (ATISolidRop[alu] << 16));
	MMIO_OUT32(mmio, RADEON_REG_DP_BRUSH_FRGD_CLR, fg);
	MMIO_OUT32(mmio, RADEON_REG_DP_WRITE_MASK, pm);
	MMIO_OUT32(mmio, RADEON_REG_DP_CNTL, RADEON_DST_X_LEFT_TO_RIGHT |
	    RADEON_DST_Y_TOP_TO_BOTTOM);

	return TRUE;
}

static void
ATISolid(int x1, int y1, int x2, int y2)
{
	if (is_24bpp) {
		x1 *= 3;
		x2 *= 3;
	}
	ATIWaitAvail(2);
	MMIO_OUT32(mmio, RADEON_REG_DST_Y_X, (y1 << 16) | x1);
	MMIO_OUT32(mmio, RADEON_REG_DST_WIDTH_HEIGHT, ((x2 - x1) << 16) |
	    (y2 - y1));
}

static void
ATIDoneSolid(void)
{
}

static Bool
ATIPrepareCopy(PixmapPtr pSrc, PixmapPtr pDst, int dx, int dy, int alu, Pixel pm)
{
	KdScreenPriv(pDst->drawable.pScreen);
	ATIScreenInfo(pScreenPriv);

	copydx = dx;
	copydy = dy;

	if (is_24bpp && pm != 0xffffffff)
		return FALSE;

	if (!ATISetup(pDst->drawable.pScreen, pDst, pSrc))
		return FALSE;

	ATIWaitAvail(3);
	MMIO_OUT32(mmio, RADEON_REG_DP_GUI_MASTER_CNTL,
	    atis->dp_gui_master_cntl |
	    RADEON_GMC_BRUSH_SOLID_COLOR |
	    RADEON_GMC_SRC_DATATYPE_COLOR |
	    (ATIBltRop[alu] << 16) |
	    RADEON_GMC_SRC_PITCH_OFFSET_CNTL |
	    RADEON_GMC_DST_PITCH_OFFSET_CNTL |
	    RADEON_DP_SRC_SOURCE_MEMORY);
	MMIO_OUT32(mmio, RADEON_REG_DP_WRITE_MASK, pm);
	MMIO_OUT32(mmio, RADEON_REG_DP_CNTL, 
	    (dx >= 0 ? RADEON_DST_X_LEFT_TO_RIGHT : 0) |
	    (dy >= 0 ? RADEON_DST_Y_TOP_TO_BOTTOM : 0));

	return TRUE;
}

static void
ATICopy(int srcX, int srcY, int dstX, int dstY, int w, int h)
{
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

	ATIWaitAvail(3);
	MMIO_OUT32(mmio, RADEON_REG_SRC_Y_X, (srcY << 16) | srcX);
	MMIO_OUT32(mmio, RADEON_REG_DST_Y_X, (dstY << 16) | dstX);
	MMIO_OUT32(mmio, RADEON_REG_DST_HEIGHT_WIDTH, (h << 16) | w);
}

static void
ATIDoneCopy(void)
{
}

static KaaScreenInfoRec ATIKaa = {
	ATIPrepareSolid,
	ATISolid,
	ATIDoneSolid,

	ATIPrepareCopy,
	ATICopy,
	ATIDoneCopy,

	0,				/* offscreenByteAlign */
	0,				/* offscreenPitch */
	KAA_OFFSCREEN_PIXMAPS,		/* flags */
};

Bool
ATIDrawInit(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	ATIScreenInfo(pScreenPriv);
	ATICardInfo(pScreenPriv);

	is_radeon = atic->is_radeon;
	is_24bpp = FALSE;

	switch (pScreenPriv->screen->fb[0].depth)
	{
	case 8:
		atis->datatype = 2;
		break;
	case 15:
		atis->datatype = 3;
		break;
	case 16:
		atis->datatype = 4;
		break;
	case 24:
		if (pScreenPriv->screen->fb[0].bitsPerPixel == 24) {
			is_24bpp = TRUE;
			atis->datatype = 2;
		} else {
			atis->datatype = 6;
		}
		break;
	default:
		FatalError("[ati]: depth %d unsupported\n",
		    pScreenPriv->screen->fb[0].depth);
		return FALSE;
	}

	atis->dp_gui_master_cntl = (atis->datatype << 8) |
	    RADEON_GMC_CLR_CMP_CNTL_DIS | RADEON_GMC_AUX_CLIP_DIS;

	if (is_radeon) {
		ATIKaa.offscreenByteAlign = 1024;
		ATIKaa.offscreenPitch = 64;
	} else {
		ATIKaa.offscreenByteAlign = 8;
		ATIKaa.offscreenPitch = pScreenPriv->screen->fb[0].bitsPerPixel;
	}
	if (!kaaDrawInit(pScreen, &ATIKaa))
		return FALSE;

	return TRUE;
}

void
ATIDrawEnable(ScreenPtr pScreen)
{
	KdMarkSync(pScreen);
}

void
ATIDrawDisable(ScreenPtr pScreen)
{
}

void
ATIDrawFini(ScreenPtr pScreen)
{
}

void
ATIDrawSync(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	ATICardInfo(pScreenPriv);

	mmio = atic->reg_base;

	ATIWaitIdle();
}
