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

#ifndef _ATI_H_
#define _ATI_H_

#include "config.h"

#ifdef KDRIVEFBDEV
#include <fbdev.h>
#endif
#ifdef KDRIVEVESA
#include <vesa.h>
#endif

#ifdef XF86DRI
#define USE_DRI
#include "libdrm.h"
#include "dri.h"
#endif

#define RADEON_REG_BASE(c)		((c)->attr.address[1])
#define RADEON_REG_SIZE(c)		(0x4000)

#ifdef __powerpc__

static __inline__ void
MMIO_OUT32(__volatile__ void *base, const unsigned long offset,
	   const unsigned int val)
{
	__asm__ __volatile__(
			"stwbrx %1,%2,%3\n\t"
			"eieio"
			: "=m" (*((volatile unsigned char *)base+offset))
			: "r" (val), "b" (base), "r" (offset));
}

static __inline__ CARD32
MMIO_IN32(__volatile__ void *base, const unsigned long offset)
{
	register unsigned int val;
	__asm__ __volatile__(
			"lwbrx %0,%1,%2\n\t"
			"eieio"
			: "=r" (val)
			: "b" (base), "r" (offset),
			"m" (*((volatile unsigned char *)base+offset)));
	return val;
}

#else

#define MMIO_OUT32(mmio, a, v)		(*(VOL32 *)((mmio) + (a)) = (v))
#define MMIO_IN32(mmio, a)		(*(VOL32 *)((mmio) + (a)))

#endif

typedef volatile CARD8	VOL8;
typedef volatile CARD16	VOL16;
typedef volatile CARD32	VOL32;

#define CAP_RADEON	0x1	/* Whether it's a Radeon vs R128 */
#define CAP_R200	0x2	/* If CAP_RADEON, whether it's an R200 */
#define CAP_NODRM	0x4	/* Set if no initialization for the DRM yet. */

struct pci_id_entry {
	CARD16 vendor;
	CARD16 device;
	CARD8 caps;
	char *name;
};

struct backend_funcs {
	void    (*cardfini)(KdCardInfo *);
	void    (*scrfini)(KdScreenInfo *);
	Bool    (*initScreen)(ScreenPtr);
	Bool    (*finishInitScreen)(ScreenPtr pScreen);
	Bool	(*createRes)(ScreenPtr);
	void    (*preserve)(KdCardInfo *);
	void    (*restore)(KdCardInfo *);
	Bool    (*dpms)(ScreenPtr, int);
	Bool    (*enable)(ScreenPtr);
	void    (*disable)(ScreenPtr);
	void    (*getColors)(ScreenPtr, int, int, xColorItem *);
	void    (*putColors)(ScreenPtr, int, int, xColorItem *);
};

typedef struct _ATICardInfo {
	union {
#ifdef KDRIVEFBDEV
		FbdevPriv fbdev;
#endif
#ifdef KDRIVEVESA
		VesaCardPrivRec vesa;
#endif
	} backend_priv;
	struct backend_funcs backend_funcs;

	struct pci_id_entry *pci_id;
	CARD8 *reg_base;
	Bool is_radeon;
	Bool is_r200;
	char *busid;
#ifdef USE_DRI
	int drmFd;
#endif /* USE_DRI */
	Bool use_fbdev, use_vesa;
} ATICardInfo;

#define getATICardInfo(kd)	((ATICardInfo *) ((kd)->card->driver))
#define ATICardInfo(kd)		ATICardInfo *atic = getATICardInfo(kd)

typedef struct _ATIScreenInfo {
	union {
#ifdef KDRIVEFBDEV
		FbdevScrPriv fbdev;
#endif
#ifdef KDRIVEVESA
		VesaScreenPrivRec vesa;
#endif
	} backend_priv;
	KaaScreenInfoRec kaa;
	ATICardInfo *atic;

	Bool using_dri;
	Bool using_dma;

#ifdef USE_DRI
	drmSize         registerSize;
	drmHandle       registerHandle;
	drmHandle       fbHandle;

	int		IsAGP;
	drmSize		gartSize;
	drmHandle	agpMemHandle;		/* Handle from drmAgpAlloc */
	unsigned long	gartOffset;
	unsigned char	*AGP;			/* Map */
	int		agpMode;
	drmSize         pciSize;
	drmHandle       pciMemHandle;

	/* ring buffer data */
	unsigned long	ringStart;		/* Offset into AGP space */
	drmHandle	ringHandle;		/* Handle from drmAddMap */
	drmSize		ringMapSize;		/* Size of map */
	int		ringSize;		/* Size of ring (MB) */
	unsigned char	*ring;			/* Map */

	unsigned long	ringReadOffset;		/* Offset into AGP space */
	drmHandle	ringReadPtrHandle;	/* Handle from drmAddMap */
	drmSize		ringReadMapSize;	/* Size of map */
	unsigned char	*ringReadPtr;		/* Map */

	/* vertex/indirect buffer data */
	unsigned long	bufStart;		/* Offset into AGP space */
	drmHandle	bufHandle;		/* Handle from drmAddMap */
	drmSize		bufMapSize;		/* Size of map */
	int		bufSize;		/* Size of buffers (MB) */
	unsigned char	*buf;			/* Map */
	int		bufNumBufs;		/* Number of buffers */
	drmBufMapPtr	buffers;		/* Buffer map */
	
	/* AGP Texture data */
	unsigned long	gartTexStart;		/* Offset into AGP space */
	drmHandle	gartTexHandle;		/* Handle from drmAddMap */
	drmSize		gartTexMapSize;		/* Size of map */
	int		gartTexSize;		/* Size of AGP tex space (MB) */
	unsigned char	*gartTex;		/* Map */
	int		log2GARTTexGran;

	int		CCEMode;          /* CCE mode that server/clients use */
	int		CPMode;           /* CP mode that server/clients use */
	int		CCEFifoSize;      /* Size of the CCE command FIFO */
	int		DMAusecTimeout;   /* CCE timeout in usecs */

	/* DMA 2D accleration */
	drmBufPtr	indirectBuffer;
	int		indirectStart;

	/* DRI screen private data */
	int		fbX;
	int		fbY;
	int		backX;
	int		backY;
	int		depthX;
	int		depthY;
	
	int		frontOffset;
	int		frontPitch;
	int		backOffset;
	int		backPitch;
	int		depthOffset;
	int		depthPitch;
	int		spanOffset;
	int		textureOffset;
	int		textureSize;
	int		log2TexGran;

	int		irqEnabled;

	int		serverContext;

	DRIInfoPtr	pDRIInfo;
#endif /* USE_DRI */
} ATIScreenInfo;

#define getATIScreenInfo(kd)	((ATIScreenInfo *) ((kd)->screen->driver))
#define ATIScreenInfo(kd)	ATIScreenInfo *atis = getATIScreenInfo(kd)

Bool
ATIMapReg(KdCardInfo *card, ATICardInfo *atic);

void
ATIUnmapReg(KdCardInfo *card, ATICardInfo *atic);

Bool
ATIDrawSetup(ScreenPtr pScreen);

Bool
ATIDrawInit(ScreenPtr pScreen);

void
ATIDrawEnable(ScreenPtr pScreen);

void
ATIDrawSync(ScreenPtr pScreen);

void
ATIDrawDisable(ScreenPtr pScreen);

void
ATIDrawFini(ScreenPtr pScreen);

#ifdef USE_DRI
Bool
ATIDRIScreenInit(ScreenPtr pScreen);

void
ATIDRICloseScreen(ScreenPtr pScreen);
#endif /* USE_DRI */

extern KdCardFuncs ATIFuncs;

#endif /* _ATI_H_ */
