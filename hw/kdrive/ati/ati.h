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
#include <vesa.h>

#define RADEON_REG_BASE(c)		((c)->attr.address[1])
#define RADEON_REG_SIZE(c)		(0x10000)

#define MMIO_OUT32(mmio, a, v)		(*(VOL32 *)((mmio) + (a)) = (v))
#define MMIO_IN32(mmio, a)		(*(VOL32 *)((mmio) + (a)))

typedef volatile CARD8	VOL8;
typedef volatile CARD16	VOL16;
typedef volatile CARD32	VOL32;

struct pci_id_list {
	CARD16 vendor;
	CARD16 device;
	char *name;
};

typedef struct _ATICardInfo {
	VesaCardPrivRec vesa;
	CARD8 *reg_base;
	Bool is_radeon;
} ATICardInfo;

#define getATICardInfo(kd)	((ATICardInfo *) ((kd)->card->driver))
#define ATICardInfo(kd)		ATICardInfo *atic = getATICardInfo(kd)

typedef struct _ATIScreenInfo {
	VesaScreenPrivRec vesa;
	CARD8 *screen;
	CARD8 *off_screen;
	int off_screen_size;

	int pitch;
	int datatype;
	int dp_gui_master_cntl;
} ATIScreenInfo;

#define getATIScreenInfo(kd)	((ATIScreenInfo *) ((kd)->screen->driver))
#define ATIScreenInfo(kd)	ATIScreenInfo *atis = getATIScreenInfo(kd)

Bool
ATIMapReg(KdCardInfo *card, ATICardInfo *atic);

void
ATIUnmapReg(KdCardInfo *card, ATICardInfo *atic);

void
ATISetMMIO(KdCardInfo *card, ATICardInfo *atic);

void
ATIResetMMIO(KdCardInfo *card, ATICardInfo *atic);

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

extern KdCardFuncs ATIFuncs;

#endif /* _ATI_H_ */
