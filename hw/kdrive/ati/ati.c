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
#if defined(USE_DRI) && defined(GLXEXT)
#include "ati_sarea.h"
#endif

static Bool ATIIsAGP(ATICardInfo *atic);

#define CAP_SERIESMASK		0xf
#define CAP_R128		0x1	/* If it's a Rage 128 */
#define CAP_R100		0x2	/* If it's an r100 series radeon. */
#define CAP_R200		0x3	/* If it's an r200 series radeon. */
#define CAP_R300		0x4	/* If it's an r300 series radeon. */

#define CAP_FEATURESMASK	0xf0
#define CAP_NOAGP		0x10	/* If it's a PCI-only card. */

struct pci_id_entry ati_pci_ids[] = {
	{0x1002, 0x4136, 0x2, "ATI Radeon RS100"},
	{0x1002, 0x4137, 0x2, "ATI Radeon RS200"},
	{0x1002, 0x4237, 0x2, "ATI Radeon RS250"},
	{0x1002, 0x4144, 0x4, "ATI Radeon R300 AD"},
	{0x1002, 0x4145, 0x4, "ATI Radeon R300 AE"},
	{0x1002, 0x4146, 0x4, "ATI Radeon R300 AF"},
	{0x1002, 0x4147, 0x4, "ATI Radeon R300 AG"},
	{0x1002, 0x4148, 0x4, "ATI Radeon R350 AH"},
	{0x1002, 0x4149, 0x4, "ATI Radeon R350 AI"},
	{0x1002, 0x414a, 0x4, "ATI Radeon R350 AJ"},
	{0x1002, 0x414b, 0x4, "ATI Radeon R350 AK"},
	{0x1002, 0x4150, 0x4, "ATI Radeon RV350 AP"},
	{0x1002, 0x4151, 0x4, "ATI Radeon RV350 AQ"},
	{0x1002, 0x4152, 0x4, "ATI Radeon RV350 AR"},
	{0x1002, 0x4153, 0x4, "ATI Radeon RV350 AS"},
	{0x1002, 0x4154, 0x4, "ATI Radeon RV350 AT"},
	{0x1002, 0x4156, 0x4, "ATI Radeon RV350 AV"},
	{0x1002, 0x4242, 0x3, "ATI Radeon R200 BB"},
	{0x1002, 0x4243, 0x3, "ATI Radeon R200 BC"},
	{0x1002, 0x4336, 0x2, "ATI Radeon RS100"},
	{0x1002, 0x4337, 0x2, "ATI Radeon RS200"},
	{0x1002, 0x4437, 0x2, "ATI Radeon RS250"},
	{0x1002, 0x4964, 0x2, "ATI Radeon RV250 Id"},
	{0x1002, 0x4965, 0x2, "ATI Radeon RV250 Ie"},
	{0x1002, 0x4966, 0x2, "ATI Radeon RV250 If"},
	{0x1002, 0x4967, 0x2, "ATI Radeon R250 Ig"},
	{0x1002, 0x4c45, 0x11, "ATI Rage 128 LE"},
	{0x1002, 0x4c46, 0x1, "ATI Rage 128 LF"},
	{0x1002, 0x4c57, 0x2, "ATI Radeon Mobiliy M7 RV200 LW (7500)"},
	{0x1002, 0x4c58, 0x2, "ATI Radeon Mobiliy M7 RV200 LX (7500)"},
	{0x1002, 0x4c59, 0x2, "ATI Radeon Mobility M6 LY"},
	{0x1002, 0x4c5a, 0x2, "ATI Radeon Mobility M6 LZ"},
	{0x1002, 0x4c64, 0x3, "ATI Radeon RV250 Ld"},
	{0x1002, 0x4c65, 0x3, "ATI Radeon RV250 Le"},
	{0x1002, 0x4c66, 0x3, "ATI Radeon Mobility M9 RV250 Lf"},
	{0x1002, 0x4c67, 0x3, "ATI Radeon RV250 Lg"},
	{0x1002, 0x4d46, 0x1, "ATI Rage 128 MF"},
	{0x1002, 0x4d46, 0x1, "ATI Rage 128 ML"},
	{0x1002, 0x4e44, 0x4, "ATI Radeon R300 ND"},
	{0x1002, 0x4e45, 0x4, "ATI Radeon R300 NE"},
	{0x1002, 0x4e46, 0x4, "ATI Radeon R300 NF"},
	{0x1002, 0x4e47, 0x4, "ATI Radeon R300 NG"},
	{0x1002, 0x4e48, 0x4, "ATI Radeon R350 NH"},
	{0x1002, 0x4e49, 0x4, "ATI Radeon R350 NI"},
	{0x1002, 0x4e4a, 0x4, "ATI Radeon R350 NJ"},
	{0x1002, 0x4e4b, 0x4, "ATI Radeon R350 NK"},
	{0x1002, 0x4e50, 0x4, "ATI Radeon Mobility RV350 NP"},
	{0x1002, 0x4e51, 0x4, "ATI Radeon Mobility RV350 NQ"},
	{0x1002, 0x4e52, 0x4, "ATI Radeon Mobility RV350 NR"},
	{0x1002, 0x4e53, 0x4, "ATI Radeon Mobility RV350 NS"},
	{0x1002, 0x4e54, 0x4, "ATI Radeon Mobility RV350 NT"},
	{0x1002, 0x4e56, 0x4, "ATI Radeon Mobility RV350 NV"},
	{0x1002, 0x5041, 0x1, "ATI Rage 128 PA"},
	{0x1002, 0x5042, 0x1, "ATI Rage 128 PB"},
	{0x1002, 0x5043, 0x1, "ATI Rage 128 PC"},
	{0x1002, 0x5044, 0x11, "ATI Rage 128 PD"},
	{0x1002, 0x5045, 0x1, "ATI Rage 128 PE"},
	{0x1002, 0x5046, 0x1, "ATI Rage 128 PF"},
	{0x1002, 0x5047, 0x1, "ATI Rage 128 PG"},
	{0x1002, 0x5048, 0x1, "ATI Rage 128 PH"},
	{0x1002, 0x5049, 0x1, "ATI Rage 128 PI"},
	{0x1002, 0x504a, 0x1, "ATI Rage 128 PJ"},
	{0x1002, 0x504b, 0x1, "ATI Rage 128 PK"},
	{0x1002, 0x504c, 0x1, "ATI Rage 128 PL"},
	{0x1002, 0x504d, 0x1, "ATI Rage 128 PM"},
	{0x1002, 0x504e, 0x1, "ATI Rage 128 PN"},
	{0x1002, 0x504f, 0x1, "ATI Rage 128 PO"},
	{0x1002, 0x5050, 0x11, "ATI Rage 128 PP"},
	{0x1002, 0x5051, 0x1, "ATI Rage 128 PQ"},
	{0x1002, 0x5052, 0x11, "ATI Rage 128 PR"},
	{0x1002, 0x5053, 0x1, "ATI Rage 128 PS"},
	{0x1002, 0x5054, 0x1, "ATI Rage 128 PT"},
	{0x1002, 0x5055, 0x1, "ATI Rage 128 PU"},
	{0x1002, 0x5056, 0x1, "ATI Rage 128 PV"},
	{0x1002, 0x5057, 0x1, "ATI Rage 128 PW"},
	{0x1002, 0x5058, 0x1, "ATI Rage 128 PX"},
	{0x1002, 0x5144, 0x2, "ATI Radeon R100 QD"},
	{0x1002, 0x5145, 0x2, "ATI Radeon R100 QE"},
	{0x1002, 0x5146, 0x2, "ATI Radeon R100 QF"},
	{0x1002, 0x5147, 0x2, "ATI Radeon R100 QG"},
	{0x1002, 0x5148, 0x3, "ATI Radeon R200 QH"},
	{0x1002, 0x514c, 0x3, "ATI Radeon R200 QL"},
	{0x1002, 0x514d, 0x3, "ATI Radeon R200 QM"},
	{0x1002, 0x5157, 0x2, "ATI Radeon RV200 QW (7500)"},
	{0x1002, 0x5158, 0x2, "ATI Radeon RV200 QX (7500)"},
	{0x1002, 0x5159, 0x2, "ATI Radeon RV100 QY"},
	{0x1002, 0x515a, 0x2, "ATI Radeon RV100 QZ"},
	{0x1002, 0x5245, 0x11, "ATI Rage 128 RE"},
	{0x1002, 0x5246, 0x1, "ATI Rage 128 RF"},
	{0x1002, 0x5247, 0x1, "ATI Rage 128 RG"},
	{0x1002, 0x524b, 0x11, "ATI Rage 128 RK"},
	{0x1002, 0x524c, 0x1, "ATI Rage 128 RL"},
	{0x1002, 0x5345, 0x1, "ATI Rage 128 SE"},
	{0x1002, 0x5346, 0x1, "ATI Rage 128 SF"},
	{0x1002, 0x5347, 0x1, "ATI Rage 128 SG"},
	{0x1002, 0x5348, 0x1, "ATI Rage 128 SH"},
	{0x1002, 0x534b, 0x1, "ATI Rage 128 SK"},
	{0x1002, 0x534c, 0x1, "ATI Rage 128 SL"},
	{0x1002, 0x534d, 0x1, "ATI Rage 128 SM"},
	{0x1002, 0x534e, 0x1, "ATI Rage 128 SN"},
	{0x1002, 0x5446, 0x1, "ATI Rage 128 TF"},
	{0x1002, 0x544c, 0x1, "ATI Rage 128 TL"},
	{0x1002, 0x5452, 0x1, "ATI Rage 128 TR"},
	{0x1002, 0x5453, 0x1, "ATI Rage 128 TS"},
	{0x1002, 0x5454, 0x1, "ATI Rage 128 TT"},
	{0x1002, 0x5455, 0x1, "ATI Rage 128 TU"},
	{0x1002, 0x5834, 0x3, "ATI Radeon RS300"},
	{0x1002, 0x5835, 0x3, "ATI Radeon RS300 Mobility"},
	{0x1002, 0x5941, 0x3, "ATI Radeon RV280 (9200)"},
	{0x1002, 0x5961, 0x3, "ATI Radeon RV280 (9200 SE)"},
	{0x1002, 0x5964, 0x3, "ATI Radeon RV280 (9200 SE)"},
	{0x1002, 0x5c60, 0x3, "ATI Radeon RV280"},
	{0x1002, 0x5c61, 0x3, "ATI Radeon RV280 Mobility"},
	{0x1002, 0x5c62, 0x3, "ATI Radeon RV280"},
	{0x1002, 0x5c63, 0x3, "ATI Radeon RV280 Mobility"},
	{0x1002, 0x5c64, 0x3, "ATI Radeon RV280"},
	{0, 0, 0, NULL}
};

static char *
make_busid(KdCardAttr *attr)
{
	char *busid;
	
	busid = xalloc(20);
	if (busid == NULL)
		return NULL;
	snprintf(busid, 20, "pci:%04x:%02x:%02x.%d", attr->domain, attr->bus, 
	    attr->slot, attr->func);
	return busid;
}

static Bool
ATICardInit(KdCardInfo *card)
{
	ATICardInfo *atic;
	int i;
	Bool initialized = FALSE;

	atic = xcalloc(sizeof(ATICardInfo), 1);
	if (atic == NULL)
		return FALSE;

#ifdef KDRIVEFBDEV
	if (!initialized && fbdevInitialize(card, &atic->backend_priv.fbdev)) {
		atic->use_fbdev = TRUE;
		initialized = TRUE;
		atic->backend_funcs.cardfini = fbdevCardFini;
		atic->backend_funcs.scrfini = fbdevScreenFini;
		atic->backend_funcs.initScreen = fbdevInitScreen;
		atic->backend_funcs.finishInitScreen = fbdevFinishInitScreen;
		atic->backend_funcs.createRes = fbdevCreateResources;
		atic->backend_funcs.preserve = fbdevPreserve;
		atic->backend_funcs.restore = fbdevRestore;
		atic->backend_funcs.dpms = fbdevDPMS;
		atic->backend_funcs.enable = fbdevEnable;
		atic->backend_funcs.disable = fbdevDisable;
		atic->backend_funcs.getColors = fbdevGetColors;
		atic->backend_funcs.putColors = fbdevPutColors;
	}
#endif
#ifdef KDRIVEVESA
	if (!initialized && vesaInitialize(card, &atic->backend_priv.vesa)) {
		atic->use_vesa = TRUE;
		initialized = TRUE;
		atic->backend_funcs.cardfini = vesaCardFini;
		atic->backend_funcs.scrfini = vesaScreenFini;
		atic->backend_funcs.initScreen = vesaInitScreen;
		atic->backend_funcs.finishInitScreen = vesaFinishInitScreen;
		atic->backend_funcs.createRes = vesaCreateResources;
		atic->backend_funcs.preserve = vesaPreserve;
		atic->backend_funcs.restore = vesaRestore;
		atic->backend_funcs.dpms = vesaDPMS;
		atic->backend_funcs.enable = vesaEnable;
		atic->backend_funcs.disable = vesaDisable;
		atic->backend_funcs.getColors = vesaGetColors;
		atic->backend_funcs.putColors = vesaPutColors;
	}
#endif

	if (!initialized || !ATIMapReg(card, atic)) {
		xfree(atic);
		return FALSE;
	}

	atic->busid = make_busid(&card->attr);
	if (atic->busid == NULL) {
		xfree(atic);
		return FALSE;
	}

#ifdef USE_DRI
	/* We demand identification by busid, not driver name */
	atic->drmFd = drmOpen(NULL, atic->busid);
	if (atic->drmFd < 0)
		ErrorF("Failed to open DRM, DRI disabled.\n");
#endif /* USE_DRI */

	card->driver = atic;

	for (i = 0; ati_pci_ids[i].name != NULL; i++) {
		if (ati_pci_ids[i].device == card->attr.deviceID) {
			atic->pci_id = &ati_pci_ids[i];
			break;
		}
	}

	if ((atic->pci_id->caps & CAP_SERIESMASK) != CAP_R128)
		atic->is_radeon = TRUE;
	if ((atic->pci_id->caps & CAP_SERIESMASK) == CAP_R100)
		atic->is_r100 = TRUE;
	if ((atic->pci_id->caps & CAP_SERIESMASK) == CAP_R200)
		atic->is_r200 = TRUE;
	if ((atic->pci_id->caps & CAP_SERIESMASK) == CAP_R300)
		atic->is_r300 = TRUE;

	atic->is_agp = ATIIsAGP(atic);

	ErrorF("Using ATI card: %s (%s) at %s\n", atic->pci_id->name,
	    atic->is_agp ? "AGP" : "PCI", atic->busid);

	return TRUE;
}

static void
ATICardFini(KdCardInfo *card)
{
	ATICardInfo *atic = (ATICardInfo *)card->driver;

	ATIUnmapReg(card, atic);
	atic->backend_funcs.cardfini(card);
}

static Bool
ATIScreenInit(KdScreenInfo *screen)
{
	ATIScreenInfo *atis;
	ATICardInfo(screen);
	Bool success = FALSE;
	int screen_size = 0;
#if defined(USE_DRI) && defined(GLXEXT)
	int l;
#endif

	atis = xcalloc(sizeof(ATIScreenInfo), 1);
	if (atis == NULL)
		return FALSE;

	atis->atic = atic;
	atis->screen = screen;
	screen->driver = atis;

#ifdef KDRIVEFBDEV
	if (atic->use_fbdev) {
		success = fbdevScreenInitialize(screen,
						&atis->backend_priv.fbdev);
		screen->memory_size = atic->backend_priv.fbdev.fix.smem_len;
		screen_size = atic->backend_priv.fbdev.var.yres_virtual *
		    screen->fb[0].byteStride;
	}
#endif
#ifdef KDRIVEVESA
	if (atic->use_vesa) {
		if (screen->fb[0].depth == 0)
			screen->fb[0].depth = 16;
		success = vesaScreenInitialize(screen,
		    &atis->backend_priv.vesa);
		screen_size = screen->off_screen_base;
	}
#endif

	if (!success) {
		screen->driver = NULL;
		xfree(atis);
		return FALSE;
	}

	screen->off_screen_base = screen_size;

	/* Reserve the area for the monochrome cursor. */
	if (screen->off_screen_base +
	    ATI_CURSOR_HEIGHT * ATI_CURSOR_PITCH * 3 <= screen->memory_size) {
		atis->cursor.offset = screen->off_screen_base;
		screen->off_screen_base += ATI_CURSOR_HEIGHT * ATI_CURSOR_PITCH * 2;
	}

#if defined(USE_DRI) && defined(GLXEXT)
	/* Reserve a static area for the back buffer the same size as the
	 * visible screen.  XXX: This would be better initialized in ati_dri.c
	 * when GLX is set up, but the offscreen memory manager's allocations
	 * don't last through VT switches, while the kernel's understanding of
	 * offscreen locations does.
	 */
	atis->frontOffset = 0;
	atis->frontPitch = screen->fb[0].byteStride;

	if (screen->off_screen_base + screen_size <= screen->memory_size) {
		atis->backOffset = screen->off_screen_base;
		atis->backPitch = screen->fb[0].byteStride;
		screen->off_screen_base += screen_size;
	}

	/* Reserve the depth span for Rage 128 */
	if (!atic->is_radeon && screen->off_screen_base +
	    screen->fb[0].byteStride <= screen->memory_size) {
		atis->spanOffset = screen->off_screen_base;
		screen->off_screen_base += screen->fb[0].byteStride;
	}

	/* Reserve the static depth buffer, which happens to be the same
	 * bitsPerPixel as the screen.
	 */
	if (screen->off_screen_base + screen_size <= screen->memory_size) {
		atis->depthOffset = screen->off_screen_base;
		atis->depthPitch = screen->fb[0].byteStride;
		screen->off_screen_base += screen_size;
	}

	/* Reserve approx. half of remaining offscreen memory for local
	 * textures.  Round down to a whole number of texture regions.
	 */
	atis->textureSize = (screen->memory_size - screen->off_screen_base) / 2;
	l = ATILog2(atis->textureSize / ATI_NR_TEX_REGIONS);
	if (l < ATI_LOG_TEX_GRANULARITY)
		l = ATI_LOG_TEX_GRANULARITY;
	atis->textureSize = (atis->textureSize >> l) << l;
	if (atis->textureSize >= 512 * 1024) {
		atis->textureOffset = screen->off_screen_base;
		screen->off_screen_base += atis->textureSize;
	} else {
		/* Minimum texture size is for 2 256x256x32bpp textures */
		atis->textureSize = 0;
	}
#endif /* USE_DRI && GLXEXT */

	/* Reserve a scratch area.  It'll be used for storing glyph data during
	 * Composite operations, because glyphs aren't in real pixmaps and thus
	 * can't be migrated.
	 */
	atis->scratch_size = 131072;	/* big enough for 128x128@32bpp */
	if (screen->off_screen_base + atis->scratch_size <= screen->memory_size)
	{
		atis->scratch_offset = screen->off_screen_base;
		screen->off_screen_base += atis->scratch_size;
		atis->scratch_next = atis->scratch_offset;
	} else {
		atis->scratch_size = 0;
	}

	return TRUE;
}

static void
ATIScreenFini(KdScreenInfo *screen)
{
	ATIScreenInfo *atis = (ATIScreenInfo *)screen->driver;
	ATICardInfo *atic = screen->card->driver;

#ifdef XV
	ATIFiniVideo(screen->pScreen);
#endif

	atic->backend_funcs.scrfini(screen);
	xfree(atis);
	screen->driver = 0;
}

Bool
ATIMapReg(KdCardInfo *card, ATICardInfo *atic)
{
	atic->reg_base = (CARD8 *)KdMapDevice(ATI_REG_BASE(card),
	    ATI_REG_SIZE(card));

	if (atic->reg_base == NULL)
		return FALSE;

	KdSetMappedMode(ATI_REG_BASE(card), ATI_REG_SIZE(card),
	    KD_MAPPED_MODE_REGISTERS);

	return TRUE;
}

void
ATIUnmapReg(KdCardInfo *card, ATICardInfo *atic)
{
	if (atic->reg_base) {
		KdResetMappedMode(ATI_REG_BASE(card), ATI_REG_SIZE(card),
		    KD_MAPPED_MODE_REGISTERS);
		KdUnmapDevice((void *)atic->reg_base, ATI_REG_SIZE(card));
		atic->reg_base = 0;
	}
}

static Bool
ATIInitScreen(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	ATICardInfo(pScreenPriv);

#ifdef XV
	ATIInitVideo(pScreen);
#endif
	return atic->backend_funcs.initScreen(pScreen);
}

static Bool
ATIFinishInitScreen(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	ATICardInfo(pScreenPriv);

	return atic->backend_funcs.finishInitScreen(pScreen);
}

static Bool
ATICreateResources(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	ATICardInfo(pScreenPriv);

	return atic->backend_funcs.createRes(pScreen);
}

static void
ATIPreserve(KdCardInfo *card)
{
	ATICardInfo *atic = card->driver;

	atic->backend_funcs.preserve(card);
}

static void
ATIRestore(KdCardInfo *card)
{
	ATICardInfo *atic = card->driver;

	ATIUnmapReg(card, atic);

	atic->backend_funcs.restore(card);
}

static Bool
ATIDPMS(ScreenPtr pScreen, int mode)
{
	KdScreenPriv(pScreen);
	ATICardInfo(pScreenPriv);

	return atic->backend_funcs.dpms(pScreen, mode);
}

static Bool
ATIEnable(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	ATICardInfo(pScreenPriv);

	if (!atic->backend_funcs.enable(pScreen))
		return FALSE;

	if ((atic->reg_base == NULL) && !ATIMapReg(pScreenPriv->screen->card,
	    atic))
		return FALSE;

	ATIDPMS(pScreen, KD_DPMS_NORMAL);

	return TRUE;
}

static void
ATIDisable(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	ATICardInfo(pScreenPriv);

	ATIUnmapReg(pScreenPriv->card, atic);

	atic->backend_funcs.disable(pScreen);
}

static void
ATIGetColors(ScreenPtr pScreen, int fb, int n, xColorItem *pdefs)
{
	KdScreenPriv(pScreen);
	ATICardInfo(pScreenPriv);

	atic->backend_funcs.getColors(pScreen, fb, n, pdefs);
}

static void
ATIPutColors(ScreenPtr pScreen, int fb, int n, xColorItem *pdefs)
{
	KdScreenPriv(pScreen);
	ATICardInfo(pScreenPriv);

	atic->backend_funcs.putColors(pScreen, fb, n, pdefs);
}

/* Compute log base 2 of val. */
int
ATILog2(int val)
{
	int bits;

	if (!val)
		return 1;
	for (bits = 0; val != 0; val >>= 1, ++bits)
		;
	return bits - 1;
}

static Bool
ATIIsAGP(ATICardInfo *atic)
{
	char *mmio = atic->reg_base;
	CARD32 agp_command;
	Bool is_agp = FALSE;

	if (mmio == NULL)
		return FALSE;

	if (atic->is_radeon) {
		/* XXX: Apparently this doesn't work.  Maybe it needs to be done
		 * through the PCI config aperture then.
		 */
		agp_command = MMIO_IN32(mmio, RADEON_REG_AGP_COMMAND);
		MMIO_OUT32(mmio, RADEON_REG_AGP_COMMAND, agp_command |
		    RADEON_AGP_ENABLE);
		if (MMIO_IN32(mmio, RADEON_REG_AGP_COMMAND) & RADEON_AGP_ENABLE)
			is_agp = TRUE;
		MMIO_OUT32(mmio, RADEON_REG_AGP_COMMAND, agp_command);
	} else {
		/* Don't know any way to detect R128 AGP automatically, so
		 * assume AGP for all cards not marked as PCI-only by XFree86.
		 */
		if ((atic->pci_id->caps & CAP_FEATURESMASK) != CAP_NOAGP)
			is_agp = TRUE;
	}

	return is_agp;
}

/* This function is required to work around a hardware bug in some (all?)
 * revisions of the R300.  This workaround should be called after every
 * CLOCK_CNTL_INDEX register access.  If not, register reads afterward
 * may not be correct.
 */
void R300CGWorkaround(ATIScreenInfo *atis) {
	ATICardInfo *atic = atis->atic;
	char *mmio = atic->reg_base;
	CARD32 save;

	save = MMIO_IN32(mmio, ATI_REG_CLOCK_CNTL_INDEX);
	MMIO_OUT32(mmio, ATI_REG_CLOCK_CNTL_INDEX, save & ~(0x3f |
	    ATI_PLL_WR_EN));
	MMIO_IN32(mmio, ATI_REG_CLOCK_CNTL_INDEX);
	MMIO_OUT32(mmio, ATI_REG_CLOCK_CNTL_INDEX, save);
}

KdCardFuncs ATIFuncs = {
	ATICardInit,		/* cardinit */
	ATIScreenInit,		/* scrinit */
	ATIInitScreen,		/* initScreen */
	ATIFinishInitScreen,	/* finishInitScreen */
	ATICreateResources,	/* createRes */
	ATIPreserve,		/* preserve */
	ATIEnable,		/* enable */
	ATIDPMS,		/* dpms */
	ATIDisable,		/* disable */
	ATIRestore,		/* restore */
	ATIScreenFini,		/* scrfini */
	ATICardFini,		/* cardfini */

	ATICursorInit,		/* initCursor */
	ATICursorEnable,	/* enableCursor */
	ATICursorDisable,	/* disableCursor */
	ATICursorFini,		/* finiCursor */
	ATIRecolorCursor,	/* recolorCursor */

	ATIDrawInit,		/* initAccel */
	ATIDrawEnable,		/* enableAccel */
	ATIDrawSync,		/* syncAccel */
	ATIDrawDisable,		/* disableAccel */
	ATIDrawFini,		/* finiAccel */

	ATIGetColors,		/* getColors */
	ATIPutColors,		/* putColors */
};
