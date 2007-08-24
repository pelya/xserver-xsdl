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

#include <sys/time.h>

#ifdef HAVE_CONFIG_H
#include <kdrive-config.h>
#endif

#include "ati.h"
#include "ati_reg.h"
#include "ati_dma.h"
#include "ati_dri.h"
#include "ati_dripriv.h"
#include "sarea.h"
#include "ati_sarea.h"
#include "ati_draw.h"
#include "r128_common.h"
#include "radeon_common.h"
#include "kaa.h"

/* ?? HACK - for now, put this here... */
/* ?? Alpha - this may need to be a variable to handle UP1x00 vs TITAN */
#if defined(__alpha__)
# define DRM_PAGE_SIZE 8192
#elif defined(__ia64__)
# define DRM_PAGE_SIZE getpagesize()
#else
# define DRM_PAGE_SIZE 4096
#endif

#ifdef GLXEXT
/* Initialize the visual configs that are supported by the hardware.
 * These are combined with the visual configs that the indirect
 * rendering core supports, and the intersection is exported to the
 * client.
 */
static Bool ATIInitVisualConfigs(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	ATIScreenInfo(pScreenPriv);
	int numConfigs = 0;
	__GLXvisualConfig *pConfigs = NULL;
	ATIConfigPrivPtr pATIConfigs = NULL;
	ATIConfigPrivPtr *pATIConfigPtrs = NULL;
	int i, accum, stencil, db, use_db;
	int depth = pScreenPriv->screen->fb[0].depth;
	int bpp = pScreenPriv->screen->fb[0].bitsPerPixel;

	if (depth != 16 && (depth != 24 || bpp != 32))
		ErrorF("DRI GLX unsupported at %d/%d depth/bpp\n", depth, bpp);

	if (atis->depthOffset != 0)
		use_db = 1;
	else
		use_db = 0;

	numConfigs = 4;
	if (use_db)
		numConfigs *= 2;

	pConfigs = xcalloc(sizeof(__GLXvisualConfig), numConfigs);
	pATIConfigs = xcalloc(sizeof(ATIConfigPrivRec), numConfigs);
	pATIConfigPtrs = xcalloc(sizeof(ATIConfigPrivPtr), numConfigs);
	if (pConfigs == NULL || pATIConfigs == NULL || pATIConfigPtrs == NULL) {
		xfree(pConfigs);
		xfree(pATIConfigs);
		xfree(pATIConfigPtrs);
		return FALSE;
	}

	i = 0;
	for (db = 0; db <= use_db; db++) {
	  for (accum = 0; accum <= 1; accum++) {
	    for (stencil = 0; stencil <= 1; stencil++) {
		pATIConfigPtrs[i] = &pATIConfigs[i];

		pConfigs[i].vid                = (VisualID)(-1);
		pConfigs[i].class              = -1;
		pConfigs[i].rgba               = TRUE;
		if (depth == 16) {
			pConfigs[i].redSize            = 5;
			pConfigs[i].greenSize          = 6;
			pConfigs[i].blueSize           = 5;
			pConfigs[i].alphaSize          = 0;
			pConfigs[i].redMask            = 0x0000F800;
			pConfigs[i].greenMask          = 0x000007E0;
			pConfigs[i].blueMask           = 0x0000001F;
			pConfigs[i].alphaMask          = 0x00000000;
		} else {
			pConfigs[i].redSize            = 8;
			pConfigs[i].greenSize          = 8;
			pConfigs[i].blueSize           = 8;
			pConfigs[i].alphaSize          = 8;
			pConfigs[i].redMask            = 0x00FF0000;
			pConfigs[i].greenMask          = 0x0000FF00;
			pConfigs[i].blueMask           = 0x000000FF;
			pConfigs[i].alphaMask          = 0xFF000000;
		}
		if (accum) { /* Simulated in software */
			pConfigs[i].accumRedSize   = 16;
			pConfigs[i].accumGreenSize = 16;
			pConfigs[i].accumBlueSize  = 16;
			if (depth == 16)
				pConfigs[i].accumAlphaSize = 0;
			else
				pConfigs[i].accumAlphaSize = 16;
		} else {
		    pConfigs[i].accumRedSize   = 0;
		    pConfigs[i].accumGreenSize = 0;
		    pConfigs[i].accumBlueSize  = 0;
		    pConfigs[i].accumAlphaSize = 0;
		}
		if (db)
		    pConfigs[i].doubleBuffer   = TRUE;
		else
		    pConfigs[i].doubleBuffer   = FALSE;
		pConfigs[i].stereo             = FALSE;
		if (depth == 16) {
			pConfigs[i].bufferSize         = 16;
			pConfigs[i].depthSize          = 16;
			if (stencil)
			    pConfigs[i].stencilSize    = 8;
			else
			    pConfigs[i].stencilSize    = 0;
		} else {
			pConfigs[i].bufferSize         = 32;
			if (stencil) {
			    pConfigs[i].depthSize      = 24;
			    pConfigs[i].stencilSize    = 8;
			} else {
			    pConfigs[i].depthSize      = 24;
			    pConfigs[i].stencilSize    = 0;
			}
		}
		pConfigs[i].auxBuffers         = 0;
		pConfigs[i].level              = 0;
		if (accum) {
		   pConfigs[i].visualRating    = GLX_SLOW_CONFIG;
		} else {
		   pConfigs[i].visualRating    = GLX_NONE;
		}
		pConfigs[i].transparentPixel   = GLX_NONE;
		pConfigs[i].transparentRed     = 0;
		pConfigs[i].transparentGreen   = 0;
		pConfigs[i].transparentBlue    = 0;
		pConfigs[i].transparentAlpha   = 0;
		pConfigs[i].transparentIndex   = 0;
		i++;
	    }
	  }
	}

	atis->numVisualConfigs = numConfigs;
	atis->pVisualConfigs = pConfigs;
	atis->pVisualConfigsPriv = pATIConfigs;
	GlxSetVisualConfigs(numConfigs, pConfigs, (void**)pATIConfigPtrs);
	return TRUE;
}
#endif /* GLXEXT */

static void
ATIDRIInitGARTValues(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	ATIScreenInfo(pScreenPriv);
	int s, l;

	atis->gartOffset = 0;

	/* Initialize the ring buffer data */
	atis->ringStart       = atis->gartOffset;
	atis->ringMapSize     = atis->ringSize * 1024 * 1024 + DRM_PAGE_SIZE;

	atis->ringReadOffset  = atis->ringStart + atis->ringMapSize;
	atis->ringReadMapSize = DRM_PAGE_SIZE;

	/* Reserve space for vertex/indirect buffers */
	atis->bufStart        = atis->ringReadOffset + atis->ringReadMapSize;
	atis->bufMapSize      = atis->bufSize * 1024 * 1024;

	/* Reserve the rest for GART textures */
	atis->gartTexStart     = atis->bufStart + atis->bufMapSize;
	s = (atis->gartSize * 1024 * 1024 - atis->gartTexStart);
	l = ATILog2((s-1) / ATI_NR_TEX_REGIONS);
	if (l < ATI_LOG_TEX_GRANULARITY) l = ATI_LOG_TEX_GRANULARITY;
	atis->gartTexMapSize   = (s >> l) << l;
	atis->log2GARTTexGran  = l;
}

static int
ATIDRIAddAndMap(int fd, drmHandle offset, drmSize size,
    drmMapType type, drmMapFlags flags, drmHandlePtr handle,
    drmAddressPtr address, char *desc)
{
	char *name;

	name = (type == DRM_AGP) ? "agp" : "pci";

	if (drmAddMap(fd, offset, size, type, flags, handle) < 0) {
		ErrorF("[%s] Could not add %s mapping\n", name, desc);
		return FALSE;
	}
	ErrorF("[%s] %s handle = 0x%08lx\n", name, desc, *handle);

	if (drmMap(fd, *handle, size, address) < 0) {
		ErrorF("[%s] Could not map %s\n", name, desc);
		return FALSE;
	}
	ErrorF("[%s] %s mapped at 0x%08lx\n", name, desc, *address);

	return TRUE;
}

/* Initialize the AGP state.  Request memory for use in AGP space, and
   initialize the Rage 128 registers to point to that memory. */
static Bool
ATIDRIAgpInit(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	ATIScreenInfo(pScreenPriv);
	ATICardInfo(pScreenPriv);
	unsigned char *mmio = atic->reg_base;
	unsigned long mode;
	int           ret;
	CARD32        cntl, chunk;

	if (drmAgpAcquire(atic->drmFd) < 0) {
		ErrorF("[agp] AGP not available\n");
		return FALSE;
	}

	ATIDRIInitGARTValues(pScreen);

	mode = drmAgpGetMode(atic->drmFd);
	if (atic->is_radeon) {
		mode &= ~RADEON_AGP_MODE_MASK;
		mode |= RADEON_AGP_1X_MODE;
	} else {
		mode &= ~R128_AGP_MODE_MASK;
		mode |= R128_AGP_1X_MODE;
	}

	if (drmAgpEnable(atic->drmFd, mode) < 0) {
		ErrorF("[agp] AGP not enabled\n");
		drmAgpRelease(atic->drmFd);
		return FALSE;
	}
	ErrorF("[agp] Mode 0x%08x selected\n", drmAgpGetMode(atic->drmFd));

	if ((ret = drmAgpAlloc(atic->drmFd, atis->gartSize * 1024 * 1024, 0,
	    NULL, &atis->agpMemHandle)) < 0) {
		ErrorF("[agp] Out of memory (%d)\n", ret);
		drmAgpRelease(atic->drmFd);
		return FALSE;
	}
	ErrorF("[agp] %d kB allocated with handle 0x%08lx\n",
	    atis->gartSize * 1024, (long)atis->agpMemHandle);

	if (drmAgpBind(atic->drmFd, atis->agpMemHandle, atis->gartOffset) < 0) {
		ErrorF("[agp] Could not bind\n");
		drmAgpFree(atic->drmFd, atis->agpMemHandle);
		drmAgpRelease(atic->drmFd);
		return FALSE;
	}

	if (!ATIDRIAddAndMap(atic->drmFd, atis->ringStart, atis->ringMapSize,
	    DRM_AGP, DRM_READ_ONLY, &atis->ringHandle,
	    (drmAddressPtr)&atis->ring, "ring"))
		return FALSE;

	if (!ATIDRIAddAndMap(atic->drmFd, atis->ringReadOffset,
	    atis->ringReadMapSize, DRM_AGP, DRM_READ_ONLY,
	    &atis->ringReadPtrHandle, (drmAddressPtr)&atis->ringReadPtr,
	    "ring read ptr"))
		return FALSE;

	if (!ATIDRIAddAndMap(atic->drmFd, atis->bufStart, atis->bufMapSize,
	    DRM_AGP, 0, &atis->bufHandle, (drmAddressPtr)&atis->buf,
	    "vertex/indirect buffers"))
		return FALSE;

	if (!ATIDRIAddAndMap(atic->drmFd, atis->gartTexStart,
	    atis->gartTexMapSize, DRM_AGP, 0, &atis->gartTexHandle,
	    (drmAddressPtr)&atis->gartTex, "AGP texture map"))
		return FALSE;

	if (atic->is_r100) {
		/* Workaround for some hardware bugs */
		cntl = MMIO_IN32(mmio, ATI_REG_AGP_CNTL);
		MMIO_OUT32(mmio, ATI_REG_AGP_CNTL, cntl |
		    RADEON_PENDING_SLOTS_VAL | RADEON_PENDING_SLOTS_SEL);
	} else if (!atic->is_radeon) {
		cntl = MMIO_IN32(mmio, ATI_REG_AGP_CNTL);
		cntl &= ~R128_AGP_APER_SIZE_MASK;
		switch (atis->gartSize) {
		case 256: cntl |= R128_AGP_APER_SIZE_256MB; break;
		case 128: cntl |= R128_AGP_APER_SIZE_128MB; break;
		case  64: cntl |= R128_AGP_APER_SIZE_64MB;  break;
		case  32: cntl |= R128_AGP_APER_SIZE_32MB;  break;
		case  16: cntl |= R128_AGP_APER_SIZE_16MB;  break;
		case   8: cntl |= R128_AGP_APER_SIZE_8MB;   break;
		case   4: cntl |= R128_AGP_APER_SIZE_4MB;   break;
		default:
			ErrorF("[agp] Illegal aperture size %d kB\n", atis->gartSize *
			    1024);
			return FALSE;
		}
		MMIO_OUT32(mmio, ATI_REG_AGP_CNTL, cntl);

		/* Disable Rage 128 PCIGART registers */
		chunk = MMIO_IN32(mmio, R128_REG_BM_CHUNK_0_VAL);
		chunk &= ~(R128_BM_PTR_FORCE_TO_PCI |
		    R128_BM_PM4_RD_FORCE_TO_PCI |
		    R128_BM_GLOBAL_FORCE_TO_PCI);
		MMIO_OUT32(mmio, R128_REG_BM_CHUNK_0_VAL, chunk);

		/* Ensure AGP GART is used (for now) */
		MMIO_OUT32(mmio, R128_REG_PCI_GART_PAGE, 1);
	}

	MMIO_OUT32(mmio, ATI_REG_AGP_BASE, drmAgpBase(atic->drmFd)); 

	return TRUE;
}

static Bool
ATIDRIPciInit(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	ATIScreenInfo(pScreenPriv);
	ATICardInfo(pScreenPriv);
	unsigned char *mmio = atic->reg_base;
	CARD32 chunk;
	int ret;

	ATIDRIInitGARTValues(pScreen);

	ret = drmScatterGatherAlloc(atic->drmFd, atis->gartSize * 1024 * 1024,
	    &atis->pciMemHandle);
	if (ret < 0) {
		ErrorF("[pci] Out of memory (%d)\n", ret);
		return FALSE;
	}
	ErrorF("[pci] %d kB allocated with handle 0x%08lx\n",
	    atis->gartSize * 1024, (long)atis->pciMemHandle);

	if (!ATIDRIAddAndMap(atic->drmFd, atis->ringStart, atis->ringMapSize,
	    DRM_SCATTER_GATHER, DRM_READ_ONLY | DRM_LOCKED | DRM_KERNEL,
	    &atis->ringHandle, (drmAddressPtr)&atis->ring, "ring"))
		return FALSE;

	if (!ATIDRIAddAndMap(atic->drmFd, atis->ringReadOffset,
	    atis->ringReadMapSize, DRM_SCATTER_GATHER,
	    DRM_READ_ONLY | DRM_LOCKED | DRM_KERNEL,
	    &atis->ringReadPtrHandle, (drmAddressPtr)&atis->ringReadPtr,
	    "ring read ptr"))
		return FALSE;

	if (!ATIDRIAddAndMap(atic->drmFd, atis->bufStart, atis->bufMapSize,
	    DRM_SCATTER_GATHER, 0, &atis->bufHandle, (drmAddressPtr)&atis->buf,
	    "vertex/indirect buffers"))
		return FALSE;

	if (!ATIDRIAddAndMap(atic->drmFd, atis->gartTexStart,
	    atis->gartTexMapSize, DRM_SCATTER_GATHER, 0, &atis->gartTexHandle,
	    (drmAddressPtr)&atis->gartTex, "PCI texture map"))
		return FALSE;

	if (!atic->is_radeon) {
		/* Force PCI GART mode */
		chunk = MMIO_IN32(mmio, R128_REG_BM_CHUNK_0_VAL);
		chunk |= (R128_BM_PTR_FORCE_TO_PCI |
		    R128_BM_PM4_RD_FORCE_TO_PCI | R128_BM_GLOBAL_FORCE_TO_PCI);
		MMIO_OUT32(mmio, R128_REG_BM_CHUNK_0_VAL, chunk);
		/* Ensure PCI GART is used */
		MMIO_OUT32(mmio, R128_REG_PCI_GART_PAGE, 0);
	}
	return TRUE;
}


/* Initialize the kernel data structures. */
static int
R128DRIKernelInit(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	ATIScreenInfo(pScreenPriv);
	ATICardInfo(pScreenPriv);
	drmR128Init drmInfo;
	int bpp = pScreenPriv->screen->fb[0].bitsPerPixel;

	memset(&drmInfo, 0, sizeof(drmR128Init) );

	drmInfo.func                = DRM_R128_INIT_CCE;
	drmInfo.sarea_priv_offset   = sizeof(XF86DRISAREARec);
	drmInfo.is_pci              = !atis->using_agp;
	drmInfo.cce_mode            = R128_PM4_64BM_64VCBM_64INDBM;
	drmInfo.cce_secure          = TRUE;
	drmInfo.ring_size           = atis->ringSize * 1024 * 1024;
	drmInfo.usec_timeout        = atis->DMAusecTimeout;

	drmInfo.front_offset        = atis->frontOffset;
	drmInfo.front_pitch         = atis->frontPitch / (bpp / 8);
	drmInfo.back_offset         = atis->backOffset;
	drmInfo.back_pitch          = atis->backPitch / (bpp / 8);
	drmInfo.fb_bpp              = bpp;

	drmInfo.depth_offset        = atis->depthOffset;
	drmInfo.depth_pitch         = atis->depthPitch / (bpp / 8);
	drmInfo.depth_bpp           = bpp;

	drmInfo.span_offset         = atis->spanOffset;

	drmInfo.fb_offset           = atis->fbHandle;
	drmInfo.mmio_offset         = atis->registerHandle;
	drmInfo.ring_offset         = atis->ringHandle;
	drmInfo.ring_rptr_offset    = atis->ringReadPtrHandle;
	drmInfo.buffers_offset      = atis->bufHandle;
	drmInfo.agp_textures_offset = atis->gartTexHandle;

	if (drmCommandWrite(atic->drmFd, DRM_R128_INIT, &drmInfo,
	    sizeof(drmR128Init)) < 0)
		return FALSE;

	return TRUE;
}

/* Initialize the kernel data structures */
static int
RadeonDRIKernelInit(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	ATIScreenInfo(pScreenPriv);
	ATICardInfo(pScreenPriv);
	drmRadeonInit drmInfo;

	memset(&drmInfo, 0, sizeof(drmRadeonInit));

	if (atic->is_r200)
	    drmInfo.func             = DRM_RADEON_INIT_R200_CP;
	else
	    drmInfo.func             = DRM_RADEON_INIT_CP;

	drmInfo.sarea_priv_offset   = sizeof(XF86DRISAREARec);
	drmInfo.is_pci              = !atis->using_agp;
	drmInfo.cp_mode             = RADEON_CSQ_PRIBM_INDBM;
	drmInfo.gart_size           = atis->gartSize * 1024 * 1024;
	drmInfo.ring_size           = atis->ringSize * 1024 * 1024;
	drmInfo.usec_timeout        = atis->DMAusecTimeout;

	drmInfo.front_offset        = atis->frontOffset;
	drmInfo.front_pitch         = atis->frontPitch;
	drmInfo.back_offset         = atis->backOffset;
	drmInfo.back_pitch          = atis->backPitch;
	drmInfo.fb_bpp              = pScreenPriv->screen->fb[0].bitsPerPixel;
	drmInfo.depth_offset        = atis->depthOffset;
	drmInfo.depth_pitch         = atis->depthPitch;
	drmInfo.depth_bpp           = pScreenPriv->screen->fb[0].bitsPerPixel;

	drmInfo.fb_offset           = atis->fbHandle;
	drmInfo.mmio_offset         = atis->registerHandle;
	drmInfo.ring_offset         = atis->ringHandle;
	drmInfo.ring_rptr_offset    = atis->ringReadPtrHandle;
	drmInfo.buffers_offset      = atis->bufHandle;
	drmInfo.gart_textures_offset = atis->gartTexHandle;

	if (drmCommandWrite(atic->drmFd, DRM_RADEON_CP_INIT,
	    &drmInfo, sizeof(drmRadeonInit)) < 0)
		return FALSE;

	return TRUE;
}

/* Add a map for the vertex buffers that will be accessed by any
   DRI-based clients. */
static Bool
ATIDRIBufInit(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	ATIScreenInfo(pScreenPriv);
	ATICardInfo(pScreenPriv);
	int type, size;

	if (atic->is_radeon)
		size = RADEON_BUFFER_SIZE;
	else
		size = R128_BUFFER_SIZE;

	if (atis->using_agp)
		type = DRM_AGP_BUFFER;
	else
		type = DRM_SG_BUFFER;

	/* Initialize vertex buffers */
	atis->bufNumBufs = drmAddBufs(atic->drmFd, atis->bufMapSize / size,
	    size, type, atis->bufStart);

	if (atis->bufNumBufs <= 0) {
		ErrorF("[drm] Could not create vertex/indirect buffers list\n");
		return FALSE;
	}
	ErrorF("[drm] Added %d %d byte vertex/indirect buffers\n",
	    atis->bufNumBufs, size);

	atis->buffers = drmMapBufs(atic->drmFd);
	if (atis->buffers == NULL) {
		ErrorF("[drm] Failed to map vertex/indirect buffers list\n");
		return FALSE;
	}
	ErrorF("[drm] Mapped %d vertex/indirect buffers\n",
	    atis->buffers->count);

	return TRUE;
}

static int 
ATIDRIIrqInit(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	ATIScreenInfo(pScreenPriv);
	ATICardInfo(pScreenPriv);

	if (atis->irqEnabled)
		return FALSE;

	atis->irqEnabled = drmCtlInstHandler(atic->drmFd, 0);

	if (!atis->irqEnabled)
		return FALSE;

	return TRUE;
}

static void ATIDRISwapContext(ScreenPtr pScreen, DRISyncType syncType,
    DRIContextType oldContextType, void *oldContext,
    DRIContextType newContextType, void *newContext)
{
	KdScreenPriv(pScreen);
	ATIScreenInfo(pScreenPriv);

	if ((syncType==DRI_3D_SYNC) && (oldContextType==DRI_2D_CONTEXT) &&
	    (newContextType==DRI_2D_CONTEXT)) {
		/* Entering from Wakeup */
		kaaMarkSync(pScreen);
	}
	if ((syncType==DRI_2D_SYNC) && (oldContextType==DRI_NO_CONTEXT) &&
	    (newContextType==DRI_2D_CONTEXT)) {
		/* Exiting from Block Handler */
		if (atis->dma_started)
			ATIFlushIndirect(atis, 1);
	}
}

static Bool ATIDRIFinishScreenInit(ScreenPtr pScreen);

/* Initialize the screen-specific data structures for the Radeon or
   Rage 128.  This is the main entry point to the device-specific
   initialization code.  It calls device-independent DRI functions to
   create the DRI data structures and initialize the DRI state. */
Bool
ATIDRIScreenInit(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	ATIScreenInfo(pScreenPriv);
	ATICardInfo(pScreenPriv);
	void *scratch_ptr;
	int scratch_int;
	DRIInfoPtr pDRIInfo;
	int devSareaSize;
	drmSetVersion sv;

	if (pScreenPriv->screen->fb[0].depth < 16 ||
	    pScreenPriv->screen->fb[0].bitsPerPixel == 24) {
		ErrorF("DRI unsupported at this depth/bpp, disabling.\n");
		return FALSE;
	}

	atis->agpMode = 1;
	atis->gartSize = 8;
	atis->ringSize = 1;
	atis->bufSize = 2;
	atis->gartTexSize = 1;
	atis->DMAusecTimeout = 10000;

	if (atic->drmFd < 0)
		return FALSE;

	sv.drm_di_major = -1;
	sv.drm_dd_major = -1;
	drmSetInterfaceVersion(atic->drmFd, &sv);
	if (atic->is_radeon) {
		if (sv.drm_dd_major != 1 || sv.drm_dd_minor < 6) {
			ErrorF("[dri] radeon kernel module version is %d.%d "
			    "but version 1.6 or greater is needed.\n",
			    sv.drm_dd_major, sv.drm_dd_minor);
			return FALSE;
		}
	} else {
		if (sv.drm_dd_major != 2 || sv.drm_dd_minor < 2) {
			ErrorF("[dri] r128 kernel module version is %d.%d "
			    "but version 2.2 or greater is needed.\n",
			    sv.drm_dd_major, sv.drm_dd_minor);
			return FALSE;
		}
	}

	/* Create the DRI data structure, and fill it in before calling the
	 * DRIScreenInit().
	 */
	pDRIInfo = DRICreateInfoRec();
	if (pDRIInfo == NULL)
		return FALSE;

	atis->pDRIInfo = pDRIInfo;
	pDRIInfo->busIdString = atic->busid;
	if (atic->is_radeon) {
		pDRIInfo->drmDriverName = "radeon";
		if (atic->is_r100)
			pDRIInfo->clientDriverName = "radeon";
		else
			pDRIInfo->clientDriverName = "r200";
	} else {
		pDRIInfo->drmDriverName = "r128";
		pDRIInfo->clientDriverName = "r128";
	}
	pDRIInfo->ddxDriverMajorVersion = 4;
	pDRIInfo->ddxDriverMinorVersion = 0;
	pDRIInfo->ddxDriverPatchVersion = 0;
	pDRIInfo->frameBufferPhysicalAddress =
	    pScreenPriv->card->attr.address[0] & 0xfc000000;
	pDRIInfo->frameBufferSize = pScreenPriv->screen->memory_size;
	pDRIInfo->frameBufferStride = pScreenPriv->screen->fb[0].byteStride;
	pDRIInfo->ddxDrawableTableEntry = SAREA_MAX_DRAWABLES;
	pDRIInfo->maxDrawableTableEntry = SAREA_MAX_DRAWABLES;

	/* For now the mapping works by using a fixed size defined
	 * in the SAREA header
	 */
	pDRIInfo->SAREASize = SAREA_MAX;

	if (atic->is_radeon) {
		pDRIInfo->devPrivateSize = sizeof(RADEONDRIRec);
		devSareaSize = sizeof(RADEONSAREAPriv);
	} else {
		pDRIInfo->devPrivateSize = sizeof(R128DRIRec);
		devSareaSize = sizeof(R128SAREAPriv);
	}

	if (sizeof(XF86DRISAREARec) + devSareaSize > SAREA_MAX) {
		ErrorF("[dri] Data does not fit in SAREA.  Disabling DRI.\n");
		return FALSE;
	}

	pDRIInfo->devPrivate = xcalloc(pDRIInfo->devPrivateSize, 1);
	if (pDRIInfo->devPrivate == NULL) {
		DRIDestroyInfoRec(atis->pDRIInfo);
		atis->pDRIInfo = NULL;
		return FALSE;
	}

	pDRIInfo->contextSize    = sizeof(ATIDRIContextRec);

	pDRIInfo->SwapContext    = ATIDRISwapContext;
	/*pDRIInfo->InitBuffers    = R128DRIInitBuffers;*/ /* XXX Appears unnecessary */
	/*pDRIInfo->MoveBuffers    = R128DRIMoveBuffers;*/ /* XXX Badness */
	pDRIInfo->bufferRequests = DRI_ALL_WINDOWS;
	/*pDRIInfo->TransitionTo2d = R128DRITransitionTo2d;
	pDRIInfo->TransitionTo3d = R128DRITransitionTo3d;
	pDRIInfo->TransitionSingleToMulti3D = R128DRITransitionSingleToMulti3d;
	pDRIInfo->TransitionMultiToSingle3D = R128DRITransitionMultiToSingle3d;*/

	pDRIInfo->createDummyCtx     = TRUE;
	pDRIInfo->createDummyCtxPriv = FALSE;

	if (!DRIScreenInit(pScreen, pDRIInfo, &atic->drmFd)) {
		ErrorF("[dri] DRIScreenInit failed.  Disabling DRI.\n");
		xfree(pDRIInfo->devPrivate);
		pDRIInfo->devPrivate = NULL;
		DRIDestroyInfoRec(pDRIInfo);
		pDRIInfo = NULL;
		return FALSE;
	}

	/* Add a map for the MMIO registers that will be accessed by any
	 * DRI-based clients.
	 */
	atis->registerSize = ATI_REG_SIZE(pScreenPriv->screen->card);
	if (drmAddMap(atic->drmFd, ATI_REG_BASE(pScreenPriv->screen->card),
	    atis->registerSize, DRM_REGISTERS, DRM_READ_ONLY,
	    &atis->registerHandle) < 0) {
		ATIDRICloseScreen(pScreen);
		return FALSE;
	}
	ErrorF("[drm] register handle = 0x%08lx\n", atis->registerHandle);

	/* DRIScreenInit adds the frame buffer map, but we need it as well */
	DRIGetDeviceInfo(pScreen, &atis->fbHandle, &scratch_int, &scratch_int,
	    &scratch_int, &scratch_int, &scratch_ptr);

	/* Initialize AGP */
	atis->using_agp = atic->is_agp;
	if (atic->is_agp && !ATIDRIAgpInit(pScreen)) {
		atis->using_agp = FALSE;
		ErrorF("[agp] AGP failed to initialize; falling back to PCI mode.\n");
		ErrorF("[agp] Make sure your kernel's AGP support is loaded and functioning.\n");
	}

	/* Initialize PCIGART */
	if (!atis->using_agp && !ATIDRIPciInit(pScreen)) {
		ATIDRICloseScreen(pScreen);
		return FALSE;
	}

#ifdef GLXEXT
	if (!ATIInitVisualConfigs(pScreen)) {
		ATIDRICloseScreen(pScreen);
		return FALSE;
	}
	ErrorF("[dri] Visual configs initialized\n");
#endif

	atis->serverContext = DRIGetContext(pScreen);

	return ATIDRIFinishScreenInit(pScreen);
}

/* Finish initializing the device-dependent DRI state, and call
   DRIFinishScreenInit() to complete the device-independent DRI
   initialization. */
static Bool
R128DRIFinishScreenInit(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	ATIScreenInfo(pScreenPriv);
	R128SAREAPrivPtr pSAREAPriv;
	R128DRIPtr       pR128DRI;
	int bpp = pScreenPriv->screen->fb[0].bitsPerPixel;

	/* Initialize the kernel data structures */
	if (!R128DRIKernelInit(pScreen)) {
		ATIDRICloseScreen(pScreen);
		return FALSE;
	}

	/* Initialize the vertex buffers list */
	if (!ATIDRIBufInit(pScreen)) {
		ATIDRICloseScreen(pScreen);
		return FALSE;
	}

	/* Initialize IRQ */
	ATIDRIIrqInit(pScreen);

	pSAREAPriv = (R128SAREAPrivPtr)DRIGetSAREAPrivate(pScreen);
	memset(pSAREAPriv, 0, sizeof(*pSAREAPriv));

	pR128DRI                    = (R128DRIPtr)atis->pDRIInfo->devPrivate;

	pR128DRI->deviceID          = pScreenPriv->screen->card->attr.deviceID;
	pR128DRI->width             = pScreenPriv->screen->width;
	pR128DRI->height            = pScreenPriv->screen->height;
	pR128DRI->depth             = pScreenPriv->screen->fb[0].depth;
	pR128DRI->bpp               = pScreenPriv->screen->fb[0].bitsPerPixel;

	pR128DRI->IsPCI             = !atis->using_agp;
	pR128DRI->AGPMode           = atis->agpMode;

	pR128DRI->frontOffset       = atis->frontOffset;
	pR128DRI->frontPitch        = atis->frontPitch / (bpp / 8);
	pR128DRI->backOffset        = atis->backOffset;
	pR128DRI->backPitch         = atis->backPitch / (bpp / 8);
	pR128DRI->depthOffset       = atis->depthOffset;
	pR128DRI->depthPitch        = atis->depthPitch / (bpp / 8);
	pR128DRI->spanOffset        = atis->spanOffset;
	pR128DRI->textureOffset     = atis->textureOffset;
	pR128DRI->textureSize       = atis->textureSize;
	pR128DRI->log2TexGran       = atis->log2TexGran;

	pR128DRI->registerHandle    = atis->registerHandle;
	pR128DRI->registerSize      = atis->registerSize;

	pR128DRI->gartTexHandle     = atis->gartTexHandle;
	pR128DRI->gartTexMapSize    = atis->gartTexMapSize;
	pR128DRI->log2AGPTexGran    = atis->log2GARTTexGran;
	pR128DRI->gartTexOffset     = atis->gartTexStart;
	pR128DRI->sarea_priv_offset = sizeof(XF86DRISAREARec);

	return TRUE;
}

/* Finish initializing the device-dependent DRI state, and call
 * DRIFinishScreenInit() to complete the device-independent DRI
 * initialization.
 */
static Bool
RadeonDRIFinishScreenInit(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	ATIScreenInfo(pScreenPriv);
	ATICardInfo(pScreenPriv);
	RADEONSAREAPrivPtr  pSAREAPriv;
	RADEONDRIPtr        pRADEONDRI;
	drmRadeonMemInitHeap drmHeap;

	/* Initialize the kernel data structures */
	if (!RadeonDRIKernelInit(pScreen)) {
		ATIDRICloseScreen(pScreen);
		return FALSE;
	}

	/* Initialize the vertex buffers list */
	if (!ATIDRIBufInit(pScreen)) {
		ATIDRICloseScreen(pScreen);
		return FALSE;
	}

	/* Initialize IRQ */
	ATIDRIIrqInit(pScreen);

	drmHeap.region = RADEON_MEM_REGION_GART;
	drmHeap.start  = 0;
	drmHeap.size   = atis->gartTexMapSize;

	if (drmCommandWrite(atic->drmFd, DRM_RADEON_INIT_HEAP, &drmHeap,
	    sizeof(drmHeap))) {
		ErrorF("[drm] Failed to initialize GART heap manager\n");
	}

	/* Initialize the SAREA private data structure */
	pSAREAPriv = (RADEONSAREAPrivPtr)DRIGetSAREAPrivate(pScreen);
	memset(pSAREAPriv, 0, sizeof(*pSAREAPriv));

	pRADEONDRI = (RADEONDRIPtr)atis->pDRIInfo->devPrivate;

	pRADEONDRI->deviceID          = pScreenPriv->screen->card->attr.deviceID;
	pRADEONDRI->width             = pScreenPriv->screen->width;
	pRADEONDRI->height            = pScreenPriv->screen->height;
	pRADEONDRI->depth             = pScreenPriv->screen->fb[0].depth;
	pRADEONDRI->bpp               = pScreenPriv->screen->fb[0].bitsPerPixel;

	pRADEONDRI->IsPCI             = !atis->using_agp;
	pRADEONDRI->AGPMode           = atis->agpMode;

	pRADEONDRI->frontOffset       = atis->frontOffset;
	pRADEONDRI->frontPitch        = atis->frontPitch;
	pRADEONDRI->backOffset        = atis->backOffset;
	pRADEONDRI->backPitch         = atis->backPitch;
	pRADEONDRI->depthOffset       = atis->depthOffset;
	pRADEONDRI->depthPitch        = atis->depthPitch;
	pRADEONDRI->textureOffset     = atis->textureOffset;
	pRADEONDRI->textureSize       = atis->textureSize;
	pRADEONDRI->log2TexGran       = atis->log2TexGran;

	pRADEONDRI->registerHandle    = atis->registerHandle;
	pRADEONDRI->registerSize      = atis->registerSize;

	pRADEONDRI->statusHandle      = atis->ringReadPtrHandle;
	pRADEONDRI->statusSize        = atis->ringReadMapSize;

	pRADEONDRI->gartTexHandle     = atis->gartTexHandle;
	pRADEONDRI->gartTexMapSize    = atis->gartTexMapSize;
	pRADEONDRI->log2GARTTexGran   = atis->log2GARTTexGran;
	pRADEONDRI->gartTexOffset     = atis->gartTexStart;

	pRADEONDRI->sarea_priv_offset = sizeof(XF86DRISAREARec);

	return TRUE;
}

static Bool
ATIDRIFinishScreenInit(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	ATIScreenInfo(pScreenPriv);
	ATICardInfo (pScreenPriv);

	atis->pDRIInfo->driverSwapMethod = DRI_HIDE_X_CONTEXT;

	/* NOTE: DRIFinishScreenInit must be called before *DRIKernelInit
	 * because *DRIKernelInit requires that the hardware lock is held by
	 * the X server, and the first time the hardware lock is grabbed is
	 * in DRIFinishScreenInit.
	 */
	if (!DRIFinishScreenInit(pScreen)) {
		ATIDRICloseScreen(pScreen);
		return FALSE;
	}

	if (atic->is_radeon) {
		if (!RadeonDRIFinishScreenInit(pScreen)) {
			ATIDRICloseScreen(pScreen);
			return FALSE;
		}
	} else {
		if (!R128DRIFinishScreenInit(pScreen)) {
			ATIDRICloseScreen(pScreen);
			return FALSE;
		}
	}

	return TRUE;
}

/* The screen is being closed, so clean up any state and free any
   resources used by the DRI. */
void
ATIDRICloseScreen(ScreenPtr pScreen)
{
	KdScreenPriv (pScreen);
	ATIScreenInfo (pScreenPriv);
	ATICardInfo (pScreenPriv);
	drmR128Init drmR128Info;
	drmRadeonInit drmRadeonInfo;

	if (atis->indirectBuffer != NULL) {
		/* Flush any remaining commands and free indirect buffers.
		 * Two steps are used because ATIFlushIndirect gets a
		 * new buffer after discarding.
		 */
		ATIFlushIndirect(atis, 1);
		ATIDRIDispatchIndirect(atis, 1);
		xfree(atis->indirectBuffer);
		atis->indirectBuffer = NULL;
		atis->indirectStart = 0;
	}
	ATIDRIDMAStop(atis);

	if (atis->irqEnabled) {
		drmCtlUninstHandler(atic->drmFd);
		atis->irqEnabled = FALSE;
	}

	/* De-allocate vertex buffers */
	if (atis->buffers) {
		drmUnmapBufs(atis->buffers);
		atis->buffers = NULL;
	}

	/* De-allocate all kernel resources */
	if (!atic->is_radeon) {
		memset(&drmR128Info, 0, sizeof(drmR128Init));
		drmR128Info.func = DRM_R128_CLEANUP_CCE;
		drmCommandWrite(atic->drmFd, DRM_R128_INIT, &drmR128Info,
		    sizeof(drmR128Init));
	} else {
		memset(&drmRadeonInfo, 0, sizeof(drmRadeonInfo));
		drmRadeonInfo.func = DRM_RADEON_CLEANUP_CP;
		drmCommandWrite(atic->drmFd, DRM_RADEON_CP_INIT, &drmRadeonInfo,
		    sizeof(drmR128Init));
	}

	/* De-allocate all AGP resources */
	if (atis->gartTex) {
		drmUnmap(atis->gartTex, atis->gartTexMapSize);
		atis->gartTex = NULL;
	}
	if (atis->buf) {
		drmUnmap(atis->buf, atis->bufMapSize);
		atis->buf = NULL;
	}
	if (atis->ringReadPtr) {
		drmUnmap(atis->ringReadPtr, atis->ringReadMapSize);
		atis->ringReadPtr = NULL;
	}
	if (atis->ring) {
		drmUnmap(atis->ring, atis->ringMapSize);
		atis->ring = NULL;
	}
	if (atis->agpMemHandle != DRM_AGP_NO_HANDLE) {
		drmAgpUnbind(atic->drmFd, atis->agpMemHandle);
		drmAgpFree(atic->drmFd, atis->agpMemHandle);
		atis->agpMemHandle = DRM_AGP_NO_HANDLE;
		drmAgpRelease(atic->drmFd);
	}
	if (atis->pciMemHandle) {
		drmScatterGatherFree(atic->drmFd, atis->pciMemHandle);
		atis->pciMemHandle = 0;
	}

	/* De-allocate all DRI resources */
	DRICloseScreen(pScreen);

	/* De-allocate all DRI data structures */
	if (atis->pDRIInfo) {
		if (atis->pDRIInfo->devPrivate) {
			xfree(atis->pDRIInfo->devPrivate);
			atis->pDRIInfo->devPrivate = NULL;
		}
		DRIDestroyInfoRec(atis->pDRIInfo);
		atis->pDRIInfo = NULL;
	}

#ifdef GLXEXT
	if (atis->pVisualConfigs) {
		xfree(atis->pVisualConfigs);
		atis->pVisualConfigs = NULL;
	}
	if (atis->pVisualConfigsPriv) {
		xfree(atis->pVisualConfigsPriv);
		atis->pVisualConfigsPriv = NULL;
	}
#endif /* GLXEXT */
	atic->drmFd = -1;
}

void
ATIDRIDMAStart(ATIScreenInfo *atis)
{
	ATICardInfo *atic = atis->atic;
	int ret;

	if (atic->is_radeon)
		ret = drmCommandNone(atic->drmFd, DRM_RADEON_CP_START);
	else
		ret = drmCommandNone(atic->drmFd, DRM_R128_CCE_START);

	if (ret == 0)
		atis->dma_started = TRUE;
	else
		FatalError("%s: DMA start returned %d\n", __FUNCTION__, ret);
}

/* Attempts to idle the DMA engine and stops it.  Note that the ioctl is the
 * same for both R128 and Radeon, so we can just use the name of one of them.
 */
void
ATIDRIDMAStop(ATIScreenInfo *atis)
{
	ATICardInfo *atic = atis->atic;
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
	atis->dma_started = FALSE;
}

void
ATIDRIDMAReset(ATIScreenInfo *atis)
{
	ATICardInfo *atic = atis->atic;
	int ret;

	ret = drmCommandNone(atic->drmFd, atic->is_radeon ?
	    DRM_RADEON_CP_RESET : DRM_R128_CCE_RESET);

	if (ret != 0)
		FatalError("Failed to reset CCE/CP\n");

	atis->dma_started = FALSE;
}

/* The R128 and Radeon Indirect ioctls differ only in the ioctl number */
void
ATIDRIDispatchIndirect(ATIScreenInfo *atis, Bool discard)
{
	ATICardInfo *atic = atis->atic;
	drmBufPtr buffer = atis->indirectBuffer->drmBuf;
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

/* Get an indirect buffer for the DMA 2D acceleration commands  */
drmBufPtr
ATIDRIGetBuffer(ATIScreenInfo *atis)
{
	ATICardInfo *atic = atis->atic;
	drmDMAReq dma;
	drmBufPtr buf = NULL;
	int indx = 0, size = 0, ret = 0;
	TIMEOUT_LOCALS;

	dma.context = atis->serverContext;
	dma.send_count = 0;
	dma.send_list = NULL;
	dma.send_sizes = NULL;
	dma.flags = 0;
	dma.request_count = 1;
	if (atic->is_radeon)
		dma.request_size = RADEON_BUFFER_SIZE;
	else
		dma.request_size = R128_BUFFER_SIZE;
	dma.request_list = &indx;
	dma.request_sizes = &size;
	dma.granted_count = 0;

	WHILE_NOT_TIMEOUT(.2) {
		ret = drmDMA(atic->drmFd, &dma);
		if (ret != -EBUSY)
			break;
	}
	if (TIMEDOUT())
		FatalError("Timeout fetching DMA buffer (card hung)\n");
	if (ret != 0)
		FatalError("Error fetching DMA buffer: %d\n", ret);

	buf = &atis->buffers->list[indx];
	buf->used = 0;
	return buf;
}
