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

struct pci_id_list radeon_id_list[] = {
	{0x1002, 0x4136, "ATI Radeon RS100"},
	{0x1002, 0x4137, "ATI Radeon RS200"},
	{0x1002, 0x4237, "ATI Radeon RS250"},
	{0x1002, 0x4144, "ATI Radeon R300 AD"},
	{0x1002, 0x4145, "ATI Radeon R300 AE"},
	{0x1002, 0x4146, "ATI Radeon R300 AF"},
	{0x1002, 0x4147, "ATI Radeon R300 AG"},
	{0x1002, 0x4148, "ATI Radeon R350 AH"},
	{0x1002, 0x4149, "ATI Radeon R350 AI"},
	{0x1002, 0x414a, "ATI Radeon R350 AJ"},
	{0x1002, 0x414b, "ATI Radeon R350 AK"},
	{0x1002, 0x4150, "ATI Radeon RV350 AP"},
	{0x1002, 0x4151, "ATI Radeon RV350 AQ"},
	{0x1002, 0x4152, "ATI Radeon RV350 AR"},
	{0x1002, 0x4153, "ATI Radeon RV350 AS"},
	{0x1002, 0x4154, "ATI Radeon RV350 AT"},
	{0x1002, 0x4156, "ATI Radeon RV350 AV"},
	{0x1002, 0x4242, "ATI Radeon R200 BB"},
	{0x1002, 0x4243, "ATI Radeon R200 BC"},
	{0x1002, 0x4336, "ATI Radeon RS100"},
	{0x1002, 0x4337, "ATI Radeon RS200"},
	{0x1002, 0x4437, "ATI Radeon RS250"},
	{0x1002, 0x4c57, "ATI Radeon RV200 LW"},
	{0x1002, 0x4c58, "ATI Radeon RV200 LX"},
	{0x1002, 0x4c59, "ATI Radeon Mobility M6 LY"},
	{0x1002, 0x4c5a, "ATI Radeon Mobility LZ"},
	{0x1002, 0x4e44, "ATI Radeon R300 ND"},
	{0x1002, 0x4e45, "ATI Radeon R300 NE"},
	{0x1002, 0x4e46, "ATI Radeon R300 NF"},
	{0x1002, 0x4e47, "ATI Radeon R300 NG"},
	{0x1002, 0x4e48, "ATI Radeon R350 NH"},
	{0x1002, 0x4e49, "ATI Radeon R350 NI"},
	{0x1002, 0x4e4a, "ATI Radeon R350 NJ"},
	{0x1002, 0x4e4b, "ATI Radeon R350 NK"},
	{0x1002, 0x4e50, "ATI Radeon Mobility RV350 NP"},
	{0x1002, 0x4e51, "ATI Radeon Mobility RV350 NQ"},
	{0x1002, 0x4e52, "ATI Radeon Mobility RV350 NR"},
	{0x1002, 0x4e53, "ATI Radeon Mobility RV350 NS"},
	{0x1002, 0x4e54, "ATI Radeon Mobility RV350 NT"},
	{0x1002, 0x4e56, "ATI Radeon Mobility RV350 NV"},
	{0x1002, 0x5144, "ATI Radeon QD"},
	{0x1002, 0x5148, "ATI Radeon R200 QH"},
	{0x1002, 0x514c, "ATI Radeon R200 QL"},
	{0x1002, 0x514d, "ATI Radeon R200 QM"},
	{0x1002, 0x5157, "ATI Radeon RV200 QW"},
	{0x1002, 0x5158, "ATI Radeon RV200 QX"},
	{0x1002, 0x5159, "ATI Radeon RV100 QY"},
	{0x1002, 0x515a, "ATI Radeon RV100 QZ"},
	{0x1002, 0x5834, "ATI Radeon RS300"},
	{0x1002, 0x5835, "ATI Radeon Mobility RS300"},
	{0x1002, 0x5c60, "ATI Radeon RV280"},
	{0x1002, 0x5c61, "ATI Mobility Radeon RV280"},
	{0x1002, 0x5c62, "ATI Radeon RV280"},
	{0x1002, 0x5c63, "ATI Mobility Radeon RV280"},
	{0x1002, 0x5c64, "ATI Radeon RV280"},
	{0, 0, NULL}
};

struct pci_id_list r128_id_list[] = {
	{0x1002, 0x4c45, "ATI Rage 128 LE"},
	{0x1002, 0x4c46, "ATI Rage 128 LF"},
	{0x1002, 0x4d46, "ATI Rage 128 MF"},
	{0x1002, 0x4d46, "ATI Rage 128 ML"},
	{0x1002, 0x5041, "ATI Rage 128 PA"},
	{0x1002, 0x5042, "ATI Rage 128 PB"},
	{0x1002, 0x5043, "ATI Rage 128 PC"},
	{0x1002, 0x5044, "ATI Rage 128 PD"},
	{0x1002, 0x5045, "ATI Rage 128 PE"},
	{0x1002, 0x5046, "ATI Rage 128 PF"},
	{0x1002, 0x5047, "ATI Rage 128 PG"},
	{0x1002, 0x5048, "ATI Rage 128 PH"},
	{0x1002, 0x5049, "ATI Rage 128 PI"},
	{0x1002, 0x504a, "ATI Rage 128 PJ"},
	{0x1002, 0x504b, "ATI Rage 128 PK"},
	{0x1002, 0x504c, "ATI Rage 128 PL"},
	{0x1002, 0x504d, "ATI Rage 128 PM"},
	{0x1002, 0x504e, "ATI Rage 128 PN"},
	{0x1002, 0x504f, "ATI Rage 128 PO"},
	{0x1002, 0x5050, "ATI Rage 128 PP"},
	{0x1002, 0x5051, "ATI Rage 128 PQ"},
	{0x1002, 0x5052, "ATI Rage 128 PR"},
	{0x1002, 0x5053, "ATI Rage 128 PS"},
	{0x1002, 0x5054, "ATI Rage 128 PT"},
	{0x1002, 0x5055, "ATI Rage 128 PU"},
	{0x1002, 0x5056, "ATI Rage 128 PV"},
	{0x1002, 0x5057, "ATI Rage 128 PW"},
	{0x1002, 0x5058, "ATI Rage 128 PX"},
	{0x1002, 0x5245, "ATI Rage 128 RE"},
	{0x1002, 0x5246, "ATI Rage 128 RF"},
	{0x1002, 0x5247, "ATI Rage 128 RG"},
	{0x1002, 0x524b, "ATI Rage 128 RK"},
	{0x1002, 0x524c, "ATI Rage 128 RL"},
	{0x1002, 0x5345, "ATI Rage 128 SE"},
	{0x1002, 0x5346, "ATI Rage 128 SF"},
	{0x1002, 0x5347, "ATI Rage 128 SG"},
	{0x1002, 0x5348, "ATI Rage 128 SH"},
	{0x1002, 0x534b, "ATI Rage 128 SK"},
	{0x1002, 0x534c, "ATI Rage 128 SL"},
	{0x1002, 0x534d, "ATI Rage 128 SM"},
	{0x1002, 0x534e, "ATI Rage 128 SN"},
	{0x1002, 0x5446, "ATI Rage 128 TF"},
	{0x1002, 0x544c, "ATI Rage 128 TL"},
	{0x1002, 0x5452, "ATI Rage 128 TR"},
	{0x1002, 0x5453, "ATI Rage 128 TS"},
	{0x1002, 0x5454, "ATI Rage 128 TT"},
	{0x1002, 0x5455, "ATI Rage 128 TU"},
	{0, 0, NULL}
};

static Bool
ATICardInit(KdCardInfo *card)
{
	ATICardInfo *atic;
	int i;

	atic = xalloc(sizeof(ATICardInfo));
	if (atic == NULL)
		return FALSE;

	ATIMapReg(card, atic);

	if (!vesaInitialize(card, &atic->vesa))
	{
		xfree(atic);
		return FALSE;
	}

	card->driver = atic;

	for (i = 0; radeon_id_list[i].name != NULL; i++) {
		if (radeon_id_list[i].device == card->attr.deviceID)
			atic->is_radeon = TRUE;
	}
	return TRUE;
}

static void
ATICardFini(KdCardInfo *card)
{
	ATICardInfo *atic = (ATICardInfo *)card->driver;

	ATIUnmapReg(card, atic);
	vesaCardFini(card);
}

static Bool
ATIScreenInit(KdScreenInfo *screen)
{
	ATIScreenInfo *atis;
	int screen_size, memory;

	atis = xalloc(sizeof(ATIScreenInfo));
	if (atis == NULL)
		return FALSE;
	memset(atis, '\0', sizeof(ATIScreenInfo));

	if (!vesaScreenInitialize(screen, &atis->vesa))
	{
		xfree(atis);
		return FALSE;
	}
	atis->screen = atis->vesa.fb;

	memory = atis->vesa.fb_size;
	screen_size = screen->fb[0].byteStride * screen->height;

	memory -= screen_size;
	if (memory > screen->fb[0].byteStride) {
		atis->off_screen = atis->screen + screen_size;
		atis->off_screen_size = memory;
	} else {
		atis->off_screen = 0;
		atis->off_screen_size = 0;
	}
	screen->driver = atis;
	return TRUE;
}

static void
ATIScreenFini(KdScreenInfo *screen)
{
	ATIScreenInfo *atis = (ATIScreenInfo *)screen->driver;

	vesaScreenFini(screen);
	xfree(atis);
	screen->driver = 0;
}

Bool
ATIMapReg(KdCardInfo *card, ATICardInfo *atic)
{
	atic->reg_base = (CARD8 *)KdMapDevice(RADEON_REG_BASE(card),
	    RADEON_REG_SIZE(card));

	if (atic->reg_base == NULL)
		return FALSE;

	KdSetMappedMode(RADEON_REG_BASE(card), RADEON_REG_SIZE(card),
	    KD_MAPPED_MODE_REGISTERS);

	return TRUE;
}

void
ATIUnmapReg(KdCardInfo *card, ATICardInfo *atic)
{
	if (atic->reg_base) {
		KdResetMappedMode(RADEON_REG_BASE(card), RADEON_REG_SIZE(card),
		    KD_MAPPED_MODE_REGISTERS);
		KdUnmapDevice((void *)atic->reg_base, RADEON_REG_SIZE(card));
		atic->reg_base = 0;
	}
}

void
ATISetMMIO(KdCardInfo *card, ATICardInfo *atic)
{
	if (atic->reg_base == NULL)
		ATIMapReg(card, atic);
}

void
ATIResetMMIO(KdCardInfo *card, ATICardInfo *atic)
{
	ATIUnmapReg(card, atic);
}


static Bool
ATIDPMS(ScreenPtr pScreen, int mode)
{
	/* XXX */
	return TRUE;
}

static Bool
ATIEnable(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	ATICardInfo(pScreenPriv);

	if (!vesaEnable(pScreen))
		return FALSE;

	ATISetMMIO(pScreenPriv->card, atic);
	ATIDPMS(pScreen, KD_DPMS_NORMAL);

	return TRUE;
}

static void
ATIDisable(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	ATICardInfo(pScreenPriv);

	ATIResetMMIO(pScreenPriv->card, atic);
	vesaDisable(pScreen);
}

static void
ATIRestore(KdCardInfo *card)
{
	ATICardInfo *atic = card->driver;

	ATIResetMMIO(card, atic);
	vesaRestore(card);
}

KdCardFuncs ATIFuncs = {
	ATICardInit,		/* cardinit */
	ATIScreenInit,		/* scrinit */
	vesaInitScreen,		/* initScreen */
	vesaFinishInitScreen,	/* finishInitScreen */
	vesaCreateResources,	/* createRes */
	vesaPreserve,		/* preserve */
	ATIEnable,		/* enable */
	ATIDPMS,		/* dpms */
	ATIDisable,		/* disable */
	ATIRestore,		/* restore */
	ATIScreenFini,		/* scrfini */
	ATICardFini,		/* cardfini */

	0,			/* initCursor */
	0,			/* enableCursor */
	0,			/* disableCursor */
	0,			/* finiCursor */
	0,			/* recolorCursor */

	ATIDrawInit,		/* initAccel */
	ATIDrawEnable,		/* enableAccel */
	ATIDrawSync,		/* syncAccel */
	ATIDrawDisable,		/* disableAccel */
	ATIDrawFini,		/* finiAccel */

	vesaGetColors,		/* getColors */
	vesaPutColors,		/* putColors */
};

