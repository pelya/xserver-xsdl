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
int is_radeon;
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

static void
ATISetup(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	ATICardInfo(pScreenPriv);
	ATIScreenInfo(pScreenPriv);

	mmio = atic->reg_base;

	ATIWaitAvail(5);
	if (is_radeon) {
		MMIO_OUT32(mmio, RADEON_REG_DEFAULT_OFFSET, atis->pitch << 16);
	} else {
		MMIO_OUT32(mmio, RADEON_REG_DEFAULT_OFFSET, 0);
		MMIO_OUT32(mmio, RADEON_REG_DEFAULT_PITCH,
		    pScreenPriv->screen->width >> 3);
	}
	MMIO_OUT32(mmio, RADEON_REG_DEFAULT_SC_BOTTOM_RIGHT,
	    (RADEON_DEFAULT_SC_RIGHT_MAX | RADEON_DEFAULT_SC_BOTTOM_MAX));
	MMIO_OUT32(mmio, RADEON_REG_SC_TOP_LEFT, 0);
	MMIO_OUT32(mmio, RADEON_REG_SC_BOTTOM_RIGHT,
	    RADEON_DEFAULT_SC_RIGHT_MAX | RADEON_DEFAULT_SC_BOTTOM_MAX);
}

static Bool
ATIPrepareSolid(PixmapPtr pPixmap, int alu, Pixel pm, Pixel fg)
{
	KdScreenPriv(pPixmap->drawable.pScreen);
	ATIScreenInfo(pScreenPriv);

	ATISetup(pPixmap->drawable.pScreen);

	ATIWaitAvail(4);
	MMIO_OUT32(mmio, RADEON_REG_DP_GUI_MASTER_CNTL,
	    atis->dp_gui_master_cntl | RADEON_GMC_BRUSH_SOLID_COLOR |
	    RADEON_GMC_SRC_DATATYPE_COLOR | (ATISolidRop[alu] << 16));
	MMIO_OUT32(mmio, RADEON_REG_DP_BRUSH_FRGD_CLR, fg);
	MMIO_OUT32(mmio, RADEON_REG_DP_WRITE_MASK, pm);
	MMIO_OUT32(mmio, RADEON_REG_DP_CNTL, RADEON_DST_X_LEFT_TO_RIGHT |
	    RADEON_DST_Y_TOP_TO_BOTTOM);

	return TRUE;
}

static void
ATISolid(int x1, int y1, int x2, int y2)
{
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

	ATIWaitAvail(3);
	MMIO_OUT32(mmio, RADEON_REG_DP_GUI_MASTER_CNTL,
	    atis->dp_gui_master_cntl | RADEON_GMC_BRUSH_SOLID_COLOR |
	    RADEON_GMC_SRC_DATATYPE_COLOR | (ATIBltRop[alu] << 16) |
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

KaaScreenInfoRec ATIKaa = {
	ATIPrepareSolid,
	ATISolid,
	ATIDoneSolid,

	ATIPrepareCopy,
	ATICopy,
	ATIDoneCopy,
};

Bool
ATIDrawInit(ScreenPtr pScreen)
{
	if (!kaaDrawInit(pScreen, &ATIKaa))
		return FALSE;

	return TRUE;
}

void
ATIDrawEnable(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	ATIScreenInfo(pScreenPriv);
	ATICardInfo(pScreenPriv);

	is_radeon = atic->is_radeon;
	atis->pitch = pScreenPriv->screen->width *
	    pScreenPriv->screen->fb[0].bitsPerPixel/8;
	
	/*ErrorF("depth=%d pitch=%d radeon=%d\n",
	    pScreenPriv->screen->fb[0].depth, atis->pitch, is_radeon);*/
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
		atis->datatype = 5;
		break;
	case 32:
		atis->datatype = 6;
		break;
	default:
		ErrorF("Bad depth %d\n", pScreenPriv->screen->fb[0].depth);
	}
	atis->dp_gui_master_cntl = (atis->datatype << 8) |
	    RADEON_GMC_CLR_CMP_CNTL_DIS | RADEON_GMC_AUX_CLIP_DIS;
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
