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
#ifdef USE_DRI
#include "radeon_common.h"
#include "r128_common.h"
#include "ati_sarea.h"
#endif /* USE_DRI */

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

static CARD32 R128BlendOp[] = {
	/* Clear */
	R128_ALPHA_BLEND_SRC_ZERO	 | R128_ALPHA_BLEND_DST_ZERO,
	/* Src */
	R128_ALPHA_BLEND_SRC_ONE	 | R128_ALPHA_BLEND_DST_ZERO,
	/* Dst */
	R128_ALPHA_BLEND_SRC_ZERO	 | R128_ALPHA_BLEND_DST_ONE,
	/* Over */
	R128_ALPHA_BLEND_SRC_ONE	 | R128_ALPHA_BLEND_DST_INVSRCALPHA,
	/* OverReverse */
	R128_ALPHA_BLEND_SRC_INVDSTALPHA | R128_ALPHA_BLEND_DST_ONE,
	/* In */
	R128_ALPHA_BLEND_SRC_DSTALPHA	 | R128_ALPHA_BLEND_DST_ZERO,
	/* InReverse */
	R128_ALPHA_BLEND_SRC_ZERO	 | R128_ALPHA_BLEND_DST_SRCALPHA,
	/* Out */
	R128_ALPHA_BLEND_SRC_INVDSTALPHA | R128_ALPHA_BLEND_DST_ZERO,
	/* OutReverse */
	R128_ALPHA_BLEND_SRC_ZERO	 | R128_ALPHA_BLEND_DST_INVSRCALPHA,
	/* Atop */
	R128_ALPHA_BLEND_SRC_DSTALPHA	 | R128_ALPHA_BLEND_DST_INVSRCALPHA,
	/* AtopReverse */
	R128_ALPHA_BLEND_SRC_INVDSTALPHA | R128_ALPHA_BLEND_DST_SRCALPHA,
	/* Xor */
	R128_ALPHA_BLEND_SRC_INVDSTALPHA | R128_ALPHA_BLEND_DST_INVSRCALPHA,
	/* Add */
	R128_ALPHA_BLEND_SRC_ONE	 | R128_ALPHA_BLEND_DST_ONE,
};

int copydx, copydy;
int fifo_size;
ATIScreenInfo *accel_atis;
int src_pitch;
int src_offset;
int src_bpp;
/* If is_24bpp is set, then we are using the accelerator in 8-bit mode due
 * to it being broken for 24bpp, so coordinates have to be multiplied by 3.
 */
Bool is_24bpp;
/* For r128 Blend, tells whether to force src x/y offset to (0,0). */
Bool is_repeat;

static void
ATIWaitAvailMMIO(int n)
{
	ATICardInfo *atic = accel_atis->atic;
	char *mmio = atic->reg_base;

	if (fifo_size >= n) {
		fifo_size -= n;
		return;
	}
	if (atic->is_radeon) {
		do {
			fifo_size = MMIO_IN32(mmio, RADEON_REG_RBBM_STATUS) &
			    RADEON_RBBM_FIFOCNT_MASK;
		} while (fifo_size < n);
	} else {
		do {
			fifo_size = MMIO_IN32(mmio, R128_REG_GUI_STAT) & 0xfff;
		} while (fifo_size < n);
	}
	fifo_size -= n;
}

static void
RadeonWaitIdle(void)
{
	ATIScreenInfo *atis = accel_atis;
	ATICardInfo *atic = atis->atic;
	char *mmio = atic->reg_base;
	CARD32 temp;

#ifdef USE_DRI
	if (atis->using_dma) {
		int ret;

		do {
			ret = drmCommandNone(atic->drmFd, DRM_RADEON_CP_IDLE);
		} while (ret == -EBUSY);
		if (ret != 0)
			ErrorF("Failed to idle DMA, returned %d\n", ret);
	}
#endif /* USE_DRI */

	/* Wait for the engine to go idle */
	ATIWaitAvailMMIO(64);

	while ((MMIO_IN32(mmio, RADEON_REG_RBBM_STATUS) &
	    RADEON_RBBM_ACTIVE) != 0)
		;

	/* Flush pixel cache */
	temp = MMIO_IN32(mmio, RADEON_REG_RB2D_DSTCACHE_CTLSTAT);
	temp |= RADEON_RB2D_DC_FLUSH_ALL;
	MMIO_OUT32(mmio, RADEON_REG_RB2D_DSTCACHE_CTLSTAT, temp);

	while ((MMIO_IN32(mmio, RADEON_REG_RB2D_DSTCACHE_CTLSTAT) &
	    RADEON_RB2D_DC_BUSY) != 0)
		;
}

static void
R128WaitIdle(void)
{
	ATIScreenInfo *atis = accel_atis;
	ATICardInfo *atic = atis->atic;
	char *mmio = atic->reg_base;
	CARD32 temp;
	int tries;

#ifdef USE_DRI
	if (atis->using_dma) {
		int ret;

		do {
			ret = drmCommandNone(atic->drmFd, DRM_R128_CCE_IDLE);
		} while (ret == -EBUSY);
		if (ret != 0)
			ErrorF("Failed to idle DMA, returned %d\n", ret);
	}
#endif /* USE_DRI */

	ATIWaitAvailMMIO(64);

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

void
ATIWaitIdle(void)
{
	ATIScreenInfo *atis = accel_atis;
	ATICardInfo *atic = atis->atic;

#ifdef USE_DRI
	/* Dispatch any accumulated commands first. */
	if (atis->using_dma && atis->indirectBuffer != NULL)
		ATIDMAFlushIndirect(0);
#endif /* USE_DRI */

	if (atic->is_radeon)
		RadeonWaitIdle();
	else
		R128WaitIdle();
}

#ifdef USE_DRI
void ATIDMAStart(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	ATICardInfo(pScreenPriv);
	ATIScreenInfo(pScreenPriv);
	int ret;

	if (atic->is_radeon)
		ret = drmCommandNone(atic->drmFd, DRM_RADEON_CP_START);
	else
		ret = drmCommandNone(atic->drmFd, DRM_R128_CCE_START);

	if (ret == 0)
		atis->using_dma = TRUE;
	else
		ErrorF("%s: DMA start returned %d\n", __FUNCTION__, ret);
}

/* Attempts to idle the DMA engine, and stops it.  Note that the ioctl is the
 * same for both R128 and Radeon, so we can just use the name of one of them.
 */
void ATIDMAStop(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	ATICardInfo(pScreenPriv);
	ATIScreenInfo(pScreenPriv);
	drmRadeonCPStop stop;
	int ret;

	stop.flush = 1;
	stop.idle  = 1;
	ret = drmCommandWrite(atic->drmFd, DRM_RADEON_CP_STOP, &stop, 
	    sizeof(drmRadeonCPStop));

	if (ret != 0 && errno == EBUSY) {
		ErrorF("Failed to idle the DMA engine\n");

		stop.idle = 0;
		ret = drmCommandWrite(atic->drmFd, DRM_RADEON_CP_STOP, &stop,
		    sizeof(drmRadeonCPStop));
	}
	atis->using_dma = FALSE;
}

/* The R128 and Radeon Indirect ioctls differ only in the ioctl number */
void ATIDMADispatchIndirect(Bool discard)
{
	ATIScreenInfo *atis = accel_atis;
	ATICardInfo *atic = atis->atic;
	drmBufPtr buffer = atis->indirectBuffer;
	drmR128Indirect indirect;
	int cmd;

	indirect.idx = buffer->idx;
	indirect.start = atis->indirectStart;
	indirect.end = buffer->used;
	indirect.discard = discard;
	cmd = atic->is_radeon ? DRM_RADEON_INDIRECT : DRM_R128_INDIRECT;
	drmCommandWriteRead(atic->drmFd, cmd, &indirect,
	    sizeof(drmR128Indirect));
}

/* Flush the indirect buffer to the kernel for submission to the card */
void ATIDMAFlushIndirect(Bool discard)
{
	ATIScreenInfo *atis = accel_atis;
	drmBufPtr buffer = atis->indirectBuffer;

	if (buffer == NULL)
		return;
	if ((atis->indirectStart == buffer->used) && !discard)
		return;

	ATIDMADispatchIndirect(discard);

	if (discard) {
		atis->indirectBuffer = ATIDMAGetBuffer();
		atis->indirectStart = 0;
	} else {
		/* Start on a double word boundary */
		atis->indirectStart = buffer->used = (buffer->used + 7) & ~7;
	}
}

/* Get an indirect buffer for the DMA 2D acceleration commands  */
drmBufPtr ATIDMAGetBuffer()
{
	ATIScreenInfo *atis = accel_atis;
	ATICardInfo *atic = atis->atic;
	drmDMAReq dma;
	drmBufPtr buf = NULL;
	int indx = 0;
	int size = 0;
	int ret;

	dma.context = atis->serverContext;
	dma.send_count = 0;
	dma.send_list = NULL;
	dma.send_sizes = NULL;
	dma.flags = 0;
	dma.request_count = 1;
	if (atis->atic->is_radeon)
		dma.request_size = RADEON_BUFFER_SIZE;
	else
		dma.request_size = R128_BUFFER_SIZE;
	dma.request_list = &indx;
	dma.request_sizes = &size;
	dma.granted_count = 0;

	do {
		ret = drmDMA(atic->drmFd, &dma);
	} while (ret != 0);

	buf = &atis->buffers->list[indx];
	buf->used = 0;
	return buf;
}

/* The hardware has a cache on the memory controller for writes to the
 * destination, which I guess is separate for 2d and 3d.  So, when switching
 * between 2d and 3d you need to wait for idle and for the cache to clean.
 */
void
RadeonSwitchTo2D(void)
{
	ATIScreenInfo *atis = accel_atis;
	RING_LOCALS;

	BEGIN_RING(2);
	OUT_RING(DMA_PACKET0(RADEON_REG_WAIT_UNTIL, 0));
	OUT_RING(RADEON_WAIT_HOST_IDLECLEAN | RADEON_WAIT_3D_IDLECLEAN);
	ADVANCE_RING();
}

void
RadeonSwitchTo3D(void)
{
	ATIScreenInfo *atis = accel_atis;
	RING_LOCALS;

	BEGIN_RING(2);
	OUT_RING(DMA_PACKET0(RADEON_REG_WAIT_UNTIL, 0));
	OUT_RING(RADEON_WAIT_HOST_IDLECLEAN | RADEON_WAIT_2D_IDLECLEAN);
	ADVANCE_RING();
}

#endif /* USE_DRI */

static Bool
ATIUploadToScreen(PixmapPtr pDst, char *src, int src_pitch)
{
	int i;
	char *dst;
	int dst_pitch;
	int bytes;

	dst = pDst->devPrivate.ptr;
	dst_pitch = pDst->devKind;
	bytes = src_pitch < dst_pitch ? src_pitch : dst_pitch;

	KdCheckSync(pDst->drawable.pScreen);

	for (i = 0; i < pDst->drawable.height; i++) {
		memcpy(dst, src, bytes);
		dst += dst_pitch;
		src += src_pitch;
	}

	return TRUE;
}


static Bool
ATIUploadToScratch(PixmapPtr pSrc, PixmapPtr pDst)
{
	KdScreenPriv(pSrc->drawable.pScreen);
	ATIScreenInfo(pScreenPriv);
	int dst_pitch;

	dst_pitch = (pSrc->drawable.width * pSrc->drawable.bitsPerPixel / 8 +
	    atis->kaa.offscreenPitch - 1) & ~(atis->kaa.offscreenPitch - 1);
	
	if (dst_pitch * pSrc->drawable.height > atis->scratch_size)
		ATI_FALLBACK(("Pixmap too large for scratch (%d,%d)\n",
		    pSrc->drawable.width, pSrc->drawable.height));

	memcpy(pDst, pSrc, sizeof(*pDst));
	pDst->devKind = dst_pitch;
	pDst->devPrivate.ptr = atis->scratch_offset +
	    pScreenPriv->screen->memory_base;

	return ATIUploadToScreen(pDst, pSrc->devPrivate.ptr, pSrc->devKind);
}

static Bool
R128GetDatatypePict(CARD32 format, CARD32 *type)
{
	switch (format) {
	case PICT_a8r8g8b8:
		*type = R128_DATATYPE_ARGB_8888;
		return TRUE;
	case PICT_r5g6b5:
		*type = R128_DATATYPE_RGB_565;
		return TRUE;
	}

	ATI_FALLBACK(("Unsupported format: %x\n", format));

	return FALSE;
}

/* Assumes that depth 15 and 16 can be used as depth 16, which is okay since we
 * require src and dest datatypes to be equal.
 */
static Bool
ATIGetDatatypeBpp(int bpp, CARD32 *type)
{
	is_24bpp = FALSE;

	switch (bpp) {
	case 8:
		*type = R128_DATATYPE_C8;
		return TRUE;
	case 16:
		*type = R128_DATATYPE_RGB_565;
		return TRUE;
	case 24:
		*type = R128_DATATYPE_C8;
		is_24bpp = TRUE;
		return TRUE;
	case 32:
		*type = R128_DATATYPE_ARGB_8888;
		return TRUE;
	default:
		ATI_FALLBACK(("Unsupported bpp: %x\n", bpp));
		return FALSE;
	}
}

#ifdef USE_DRI
#define USE_DMA
#include "ati_drawtmp.h"
#include "r128_blendtmp.h"
#endif /* USE_DRI */

#undef USE_DMA
#include "ati_drawtmp.h"
#include "r128_blendtmp.h"

static void
ATIDoneSolid(void)
{
}

static void
ATIDoneCopy(void)
{
}

Bool
ATIDrawInit(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	ATIScreenInfo(pScreenPriv);
	ATICardInfo(pScreenPriv);

	ErrorF("Screen: %d/%d depth/bpp\n", pScreenPriv->screen->fb[0].depth,
	    pScreenPriv->screen->fb[0].bitsPerPixel);
#ifdef USE_DRI
	if (atis->using_dri)
		ATIDMAStart(pScreen);
	else {
		if (!atic->is_r300 && ATIDRIScreenInit(pScreen))
			atis->using_dri = TRUE;
	}
#endif /* USE_DRI */

	memset(&atis->kaa, 0, sizeof(KaaScreenInfoRec));
#ifdef USE_DRI
	if (atis->using_dma) {
		atis->kaa.PrepareSolid = ATIPrepareSolidDMA;
		atis->kaa.Solid = ATISolidDMA;
		atis->kaa.PrepareCopy = ATIPrepareCopyDMA;
		atis->kaa.Copy = ATICopyDMA;
		if (!atic->is_radeon) {
			atis->kaa.PrepareBlend = R128PrepareBlendDMA;
			atis->kaa.Blend = R128BlendDMA;
			atis->kaa.DoneBlend = R128DoneBlendDMA;
		} else if (!atic->is_r200) {
			atis->kaa.PrepareBlend = RadeonPrepareBlend;
			atis->kaa.Blend = RadeonBlend;
			atis->kaa.DoneBlend = RadeonDoneBlend;
			atis->kaa.PrepareComposite = RadeonPrepareComposite;
			atis->kaa.Composite = RadeonComposite;
			atis->kaa.DoneComposite = RadeonDoneComposite;
		}
	} else {
#else
	{
#endif /* USE_DRI */
		atis->kaa.PrepareSolid = ATIPrepareSolidMMIO;
		atis->kaa.Solid = ATISolidMMIO;
		atis->kaa.PrepareCopy = ATIPrepareCopyMMIO;
		atis->kaa.Copy = ATICopyMMIO;
		if (!atic->is_radeon) {
			atis->kaa.PrepareBlend = R128PrepareBlendMMIO;
			atis->kaa.Blend = R128BlendMMIO;
			atis->kaa.DoneBlend = R128DoneBlendMMIO;
		}
	}
	atis->kaa.UploadToScreen = ATIUploadToScreen;
	if (atis->scratch_size != 0)
		atis->kaa.UploadToScratch = ATIUploadToScratch;
	atis->kaa.DoneSolid = ATIDoneSolid;
	atis->kaa.DoneCopy = ATIDoneCopy;
	atis->kaa.flags = KAA_OFFSCREEN_PIXMAPS;
	if (atic->is_radeon) {
		atis->kaa.offscreenByteAlign = 1024;
		atis->kaa.offscreenPitch = 64;
	} else {
		atis->kaa.offscreenByteAlign = 32;
		/* Pitch alignment is in sets of 8 pixels, and we need to cover
		 * 32bpp, so 32 bytes.
		 */
		atis->kaa.offscreenPitch = 32;
	}
	if (!kaaDrawInit(pScreen, &atis->kaa))
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
#ifdef USE_DRI
	KdScreenPriv(pScreen);
	ATIScreenInfo(pScreenPriv);

	if (atis->using_dma)
		ATIDMAStop(pScreen);

	if (atis->using_dri)
		ATIDRICloseScreen(pScreen);
#endif /* USE_DRI */

	kaaDrawFini(pScreen);
}

void
ATIDrawSync(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	ATIScreenInfo(pScreenPriv);

	accel_atis = atis;

	ATIWaitIdle();
}
