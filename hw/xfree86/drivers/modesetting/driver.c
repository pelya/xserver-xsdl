/*
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * Copyright 2011 Dave Airlie
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * Original Author: Alan Hourihane <alanh@tungstengraphics.com>
 * Rewrite: Dave Airlie <airlied@redhat.com> 
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <fcntl.h>
#include "xf86.h"
#include "xf86_OSproc.h"
#include "compiler.h"
#include "xf86PciInfo.h"
#include "xf86Pci.h"
#include "mipointer.h"
#include "micmap.h"
#include <X11/extensions/randr.h>
#include "fb.h"
#include "edid.h"
#include "xf86i2c.h"
#include "xf86Crtc.h"
#include "miscstruct.h"
#include "dixstruct.h"
#include "shadow.h"
#include "xf86xv.h"
#include <X11/extensions/Xv.h>
#include <xorg-server.h>
#if XSERVER_LIBPCIACCESS
#include <pciaccess.h>
#endif

#include "driver.h"

#ifndef DRM_CAP_DUMB_PREFER_SHADOW
#define DRM_CAP_DUMB_PREFER_SHADOW 4
#endif

static void AdjustFrame(int scrnIndex, int x, int y, int flags);
static Bool CloseScreen(int scrnIndex, ScreenPtr pScreen);
static Bool EnterVT(int scrnIndex, int flags);
static void Identify(int flags);
static const OptionInfoRec *AvailableOptions(int chipid, int busid);
static ModeStatus ValidMode(int scrnIndex, DisplayModePtr mode, Bool verbose,
			    int flags);
static void FreeScreen(int scrnIndex, int flags);
static void LeaveVT(int scrnIndex, int flags);
static Bool SwitchMode(int scrnIndex, DisplayModePtr mode, int flags);
static Bool ScreenInit(int scrnIndex, ScreenPtr pScreen, int argc,
		       char **argv);
static Bool PreInit(ScrnInfoPtr pScrn, int flags);

static Bool Probe(DriverPtr drv, int flags);
static Bool ms_pci_probe(DriverPtr driver,
			 int entity_num, struct pci_device *device,
			 intptr_t match_data);

#ifdef XSERVER_LIBPCIACCESS
static const struct pci_id_match ms_device_match[] = {
    {
	PCI_MATCH_ANY, PCI_MATCH_ANY, PCI_MATCH_ANY, PCI_MATCH_ANY,
	0x00030000, 0x00ffffff, 0
    },

    { 0, 0, 0 },
};
#endif

_X_EXPORT DriverRec modesetting = {
    1,
    "modesetting",
    Identify,
    Probe,
    AvailableOptions,
    NULL,
    0,
    NULL,
    ms_device_match,
    ms_pci_probe,
};

static SymTabRec Chipsets[] = {
    {0, "kms" },
    {-1, NULL}
};

typedef enum
{
    OPTION_SW_CURSOR,
    OPTION_DEVICE_PATH,
    OPTION_SHADOW_FB,
} modesettingOpts;

static const OptionInfoRec Options[] = {
    {OPTION_SW_CURSOR, "SWcursor", OPTV_BOOLEAN, {0}, FALSE},
    {OPTION_DEVICE_PATH, "kmsdev", OPTV_STRING, {0}, FALSE },
    {OPTION_SHADOW_FB, "ShadowFB", OPTV_BOOLEAN, {0}, FALSE },
    {-1, NULL, OPTV_NONE, {0}, FALSE}
};

int modesettingEntityIndex = -1;

static MODULESETUPPROTO(Setup);

static XF86ModuleVersionInfo VersRec = {
    "modesetting",
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XORG_VERSION_CURRENT,
    PACKAGE_VERSION_MAJOR, PACKAGE_VERSION_MINOR, PACKAGE_VERSION_PATCHLEVEL,
    ABI_CLASS_VIDEODRV,
    ABI_VIDEODRV_VERSION,
    MOD_CLASS_VIDEODRV,
    {0, 0, 0, 0}
};

_X_EXPORT XF86ModuleData modesettingModuleData = { &VersRec, Setup, NULL };

static pointer
Setup(pointer module, pointer opts, int *errmaj, int *errmin)
{
    static Bool setupDone = 0;

    /* This module should be loaded only once, but check to be sure.
     */
    if (!setupDone) {
	setupDone = 1;
	xf86AddDriver(&modesetting, module, HaveDriverFuncs);

	/*
	 * The return value must be non-NULL on success even though there
	 * is no TearDownProc.
	 */
	return (pointer) 1;
    } else {
	if (errmaj)
	    *errmaj = LDR_ONCEONLY;
	return NULL;
    }
}

static void
Identify(int flags)
{
    xf86PrintChipsets("modesetting", "Driver for Modesetting Kernel Drivers",
		      Chipsets);
}

static int open_hw(char *dev)
{
    int fd;
    if (dev)
	fd = open(dev, O_RDWR, 0);
    else {
	dev = getenv("KMSDEVICE");
	if ((NULL == dev) || ((fd = open(dev, O_RDWR, 0)) == -1)) {
	    dev = "/dev/dri/card0";
	    fd = open(dev,O_RDWR, 0);
	}
    }
    if (fd == -1)
	xf86DrvMsg(-1, X_ERROR,"open %s: %s\n", dev, strerror(errno));

    return fd;
}

static Bool probe_hw(char *dev)
{
    int fd = open_hw(dev);
    if (fd != -1) {
        close(fd);
	return TRUE;
    }
    return FALSE;
}

static const OptionInfoRec *
AvailableOptions(int chipid, int busid)
{
    return Options;
}

#if XSERVER_LIBPCIACCESS
static Bool
ms_pci_probe(DriverPtr driver,
	     int entity_num, struct pci_device *dev, intptr_t match_data)
{
    ScrnInfoPtr scrn = NULL;

    scrn = xf86ConfigPciEntity(scrn, 0, entity_num, NULL,
			       NULL, NULL, NULL, NULL, NULL);
    if (scrn) {
	char *devpath;
	GDevPtr devSection = xf86GetDevFromEntity(scrn->entityList[0],
						  scrn->entityInstanceList[0]);

	devpath = xf86FindOptionValue(devSection->options, "kmsdev");
	if (probe_hw(devpath)) {
	    scrn->driverVersion = 1;
	    scrn->driverName = "modesetting";
	    scrn->name = "modeset";
	    scrn->Probe = NULL;
	    scrn->PreInit = PreInit;
	    scrn->ScreenInit = ScreenInit;
	    scrn->SwitchMode = SwitchMode;
	    scrn->AdjustFrame = AdjustFrame;
	    scrn->EnterVT = EnterVT;
	    scrn->LeaveVT = LeaveVT;
	    scrn->FreeScreen = FreeScreen;
	    scrn->ValidMode = ValidMode;

	    xf86DrvMsg(scrn->scrnIndex, X_CONFIG,
		       "claimed PCI slot %d@%d:%d:%d\n", 
		       dev->bus, dev->domain, dev->dev, dev->func);
	    xf86DrvMsg(scrn->scrnIndex, X_INFO,
		       "using %s\n", devpath ? devpath : "default device");
	} else
	    scrn = NULL;
    }
    return scrn != NULL;
}
#endif

static Bool
Probe(DriverPtr drv, int flags)
{
    int i, numDevSections;
    GDevPtr *devSections;
    Bool foundScreen = FALSE;
    char *dev;
    ScrnInfoPtr scrn = NULL;

    /* For now, just bail out for PROBE_DETECT. */
    if (flags & PROBE_DETECT)
	return FALSE;

    /*
     * Find the config file Device sections that match this
     * driver, and return if there are none.
     */
    if ((numDevSections = xf86MatchDevice("modesetting", &devSections)) <= 0) {
	return FALSE;
    }

    for (i = 0; i < numDevSections; i++) {

	dev = xf86FindOptionValue(devSections[i]->options,"kmsdev");
	if (probe_hw(dev)) {
	    int entity;
	    entity = xf86ClaimFbSlot(drv, 0, devSections[i], TRUE);
	    scrn = xf86ConfigFbEntity(scrn, 0, entity,
				  NULL, NULL, NULL, NULL);
	}

	if (scrn) {
	    foundScreen = TRUE;
	    scrn->driverVersion = 1;
	    scrn->driverName = "modesetting";
	    scrn->name = "modesetting";
	    scrn->Probe = Probe;
	    scrn->PreInit = PreInit;
	    scrn->ScreenInit = ScreenInit;
	    scrn->SwitchMode = SwitchMode;
	    scrn->AdjustFrame = AdjustFrame;
	    scrn->EnterVT = EnterVT;
	    scrn->LeaveVT = LeaveVT;
	    scrn->FreeScreen = FreeScreen;
	    scrn->ValidMode = ValidMode;

	    xf86DrvMsg(scrn->scrnIndex, X_INFO,
			   "using %s\n", dev ? dev : "default device");
	}
    }

    free(devSections);

    return foundScreen;
}

static Bool
GetRec(ScrnInfoPtr pScrn)
{
    if (pScrn->driverPrivate)
	return TRUE;

    pScrn->driverPrivate = xnfcalloc(sizeof(modesettingRec), 1);

    return TRUE;
}

static void dispatch_dirty(ScreenPtr pScreen)
{
    ScrnInfoPtr scrn = xf86Screens[pScreen->myNum];
    modesettingPtr ms = modesettingPTR(scrn);
    RegionPtr dirty = DamageRegion(ms->damage);
    unsigned num_cliprects = REGION_NUM_RECTS(dirty);

    if (num_cliprects) {
	drmModeClip *clip = alloca(num_cliprects * sizeof(drmModeClip));
	BoxPtr rect = REGION_RECTS(dirty);
	int i, ret;
	    
	/* XXX no need for copy? */
	for (i = 0; i < num_cliprects; i++, rect++) {
	    clip[i].x1 = rect->x1;
	    clip[i].y1 = rect->y1;
	    clip[i].x2 = rect->x2;
	    clip[i].y2 = rect->y2;
	}

	/* TODO query connector property to see if this is needed */
	ret = drmModeDirtyFB(ms->fd, ms->drmmode.fb_id, clip, num_cliprects);
	if (ret) {
	    if (ret == -EINVAL || ret == -ENOSYS) {
		ms->dirty_enabled = FALSE;
		DamageUnregister(&pScreen->GetScreenPixmap(pScreen)->drawable, ms->damage);
		DamageDestroy(ms->damage);
		ms->damage = NULL;
		xf86DrvMsg(scrn->scrnIndex, X_INFO, "Disabling kernel dirty updates, not required.\n");
		return;
	    } else
		ErrorF("%s: failed to send dirty (%i, %s)\n",
		       __func__, ret, strerror(-ret));
	}
	
	DamageEmpty(ms->damage);
    }
}

static void msBlockHandler(int i, pointer blockData, pointer pTimeout,
			   pointer pReadmask)
{
    ScreenPtr pScreen = screenInfo.screens[i];
    modesettingPtr ms = modesettingPTR(xf86Screens[pScreen->myNum]);

    pScreen->BlockHandler = ms->BlockHandler;
    pScreen->BlockHandler(i, blockData, pTimeout, pReadmask);
    pScreen->BlockHandler = msBlockHandler;
    if (ms->dirty_enabled)
	dispatch_dirty(pScreen);
}

static void
FreeRec(ScrnInfoPtr pScrn)
{
    if (!pScrn)
	return;

    if (!pScrn->driverPrivate)
	return;

    free(pScrn->driverPrivate);

    pScrn->driverPrivate = NULL;
}

static Bool
PreInit(ScrnInfoPtr pScrn, int flags)
{
    modesettingPtr ms;
    rgb defaultWeight = { 0, 0, 0 };
    EntityInfoPtr pEnt;
    EntPtr msEnt = NULL;
    char *BusID, *devicename;
    Bool prefer_shadow = TRUE;
    uint64_t value = 0;
    int ret;

    if (pScrn->numEntities != 1)
	return FALSE;

    pEnt = xf86GetEntityInfo(pScrn->entityList[0]);

    if (flags & PROBE_DETECT) {
	return FALSE;
    }

    /* Allocate driverPrivate */
    if (!GetRec(pScrn))
	return FALSE;

    ms = modesettingPTR(pScrn);
    ms->SaveGeneration = -1;
    ms->pEnt = pEnt;

    pScrn->displayWidth = 640;	       /* default it */

    /* Allocate an entity private if necessary */
    if (xf86IsEntityShared(pScrn->entityList[0])) {
	msEnt = xf86GetEntityPrivate(pScrn->entityList[0],
				     modesettingEntityIndex)->ptr;
	ms->entityPrivate = msEnt;
    } else
	ms->entityPrivate = NULL;

    if (xf86IsEntityShared(pScrn->entityList[0])) {
	if (xf86IsPrimInitDone(pScrn->entityList[0])) {
	    /* do something */
	} else {
	    xf86SetPrimInitDone(pScrn->entityList[0]);
	}
    }

    pScrn->monitor = pScrn->confScreen->monitor;
    pScrn->progClock = TRUE;
    pScrn->rgbBits = 8;

    if (!xf86SetDepthBpp
	(pScrn, 0, 0, 0,
	 PreferConvert24to32 | SupportConvert24to32 | Support32bppFb))
	return FALSE;

    switch (pScrn->depth) {
    case 15:
    case 16:
    case 24:
	break;
    default:
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Given depth (%d) is not supported by the driver\n",
		   pScrn->depth);
	return FALSE;
    }
    xf86PrintDepthBpp(pScrn);

    /* Process the options */
    xf86CollectOptions(pScrn, NULL);
    if (!(ms->Options = malloc(sizeof(Options))))
	return FALSE;
    memcpy(ms->Options, Options, sizeof(Options));
    xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, ms->Options);

    ms->PciInfo = xf86GetPciInfoForEntity(ms->pEnt->index);
    if (ms->PciInfo) {
       BusID = malloc(64);
       sprintf(BusID, "PCI:%d:%d:%d",
#if XSERVER_LIBPCIACCESS
	        ((ms->PciInfo->domain << 8) | ms->PciInfo->bus),
	        ms->PciInfo->dev, ms->PciInfo->func
#else
	        ((pciConfigPtr) ms->PciInfo->thisCard)->busnum,
	        ((pciConfigPtr) ms->PciInfo->thisCard)->devnum,
	        ((pciConfigPtr) ms->PciInfo->thisCard)->funcnum
#endif
	    );

       ms->fd = drmOpen(NULL, BusID);
    } else {
       devicename = xf86GetOptValString(ms->Options, OPTION_DEVICE_PATH);
       ms->fd = open_hw(devicename);
    }
    if (ms->fd < 0)
	return FALSE;

    if (!xf86SetWeight(pScrn, defaultWeight, defaultWeight))
	return FALSE;
    if (!xf86SetDefaultVisual(pScrn, -1))
	return FALSE;

    if (xf86ReturnOptValBool(ms->Options, OPTION_SW_CURSOR, FALSE)) {
	ms->drmmode.sw_cursor = TRUE;
    }

    ret = drmGetCap(ms->fd, DRM_CAP_DUMB_PREFER_SHADOW, &value);
    if (!ret) {
	prefer_shadow = !!value;
    }

    ms->drmmode.shadow_enable = xf86ReturnOptValBool(ms->Options, OPTION_SHADOW_FB, prefer_shadow);

    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "ShadowFB: preferred %s, enabled %s\n", prefer_shadow ? "YES" : "NO", ms->drmmode.shadow_enable ? "YES" : "NO");
    ms->drmmode.fd = ms->fd;
    if (drmmode_pre_init(pScrn, &ms->drmmode, pScrn->bitsPerPixel / 8) == FALSE) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "KMS setup failed\n");
	goto fail;
    }

    /*
     * If the driver can do gamma correction, it should call xf86SetGamma() here.
     */
    {
	Gamma zeros = { 0.0, 0.0, 0.0 };

	if (!xf86SetGamma(pScrn, zeros)) {
	    return FALSE;
	}
    }

    if (pScrn->modes == NULL) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No modes.\n");
	return FALSE;
    }

    pScrn->currentMode = pScrn->modes;

    /* Set display resolution */
    xf86SetDpi(pScrn, 0, 0);

    /* Load the required sub modules */
    if (!xf86LoadSubModule(pScrn, "fb")) {
	return FALSE;
    }

    if (ms->drmmode.shadow_enable) {
	if (!xf86LoadSubModule(pScrn, "shadow")) {
	    return FALSE;
	}
    }

    return TRUE;
    fail:
    return FALSE;
}

static void *
msShadowWindow(ScreenPtr screen, CARD32 row, CARD32 offset, int mode,
	       CARD32 *size, void *closure)
{
    ScrnInfoPtr pScrn = xf86Screens[screen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    int stride;

    stride = (pScrn->displayWidth * pScrn->bitsPerPixel) / 8;
    *size = stride;

    return ((uint8_t *)ms->drmmode.front_bo->ptr + row * stride + offset);
}

static Bool
CreateScreenResources(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    PixmapPtr rootPixmap;
    Bool ret;
    void *pixels;
    pScreen->CreateScreenResources = ms->createScreenResources;
    ret = pScreen->CreateScreenResources(pScreen);
    pScreen->CreateScreenResources = CreateScreenResources;

    if (!drmmode_set_desired_modes(pScrn, &ms->drmmode))
      return FALSE;

    drmmode_uevent_init(pScrn, &ms->drmmode);

    if (!ms->SWCursor)
        drmmode_map_cursor_bos(pScrn, &ms->drmmode);
    pixels = drmmode_map_front_bo(&ms->drmmode);
    if (!pixels)
	return FALSE;

    rootPixmap = pScreen->GetScreenPixmap(pScreen);

    if (ms->drmmode.shadow_enable)
	pixels = ms->drmmode.shadow_fb;
    
    if (!pScreen->ModifyPixmapHeader(rootPixmap, -1, -1, -1, -1, -1, pixels))
	FatalError("Couldn't adjust screen pixmap\n");

    if (ms->drmmode.shadow_enable) {
	if (!shadowAdd(pScreen, rootPixmap, shadowUpdatePackedWeak(),
		       msShadowWindow, 0, 0))
	    return FALSE;
    }

    ms->damage = DamageCreate(NULL, NULL, DamageReportNone, TRUE,
                              pScreen, rootPixmap);

    if (ms->damage) {
	DamageRegister(&rootPixmap->drawable, ms->damage);
	ms->dirty_enabled = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Damage tracking initialized\n");
    } else {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Failed to create screen damage record\n");
	return FALSE;
    }
    return ret;
}

static Bool
msShadowInit(ScreenPtr pScreen)
{
    if (!shadowSetup(pScreen)) {
	return FALSE;
    }
    return TRUE;
}

static Bool
ScreenInit(int scrnIndex, ScreenPtr pScreen, int argc, char **argv)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    VisualPtr visual;
    int ret;

    pScrn->pScreen = pScreen;

    ret = drmSetMaster(ms->fd);
    if (ret) {
        ErrorF("Unable to set master\n");
        return FALSE;
    }
      
    /* HW dependent - FIXME */
    pScrn->displayWidth = pScrn->virtualX;
    if (!drmmode_create_initial_bos(pScrn, &ms->drmmode))
	return FALSE;

    if (ms->drmmode.shadow_enable) {
	ms->drmmode.shadow_fb = calloc(1, pScrn->displayWidth * pScrn->virtualY *
			       ((pScrn->bitsPerPixel + 7) >> 3));
	if (!ms->drmmode.shadow_fb)
	    ms->drmmode.shadow_enable = FALSE;
    }	
    
    miClearVisualTypes();

    if (!miSetVisualTypes(pScrn->depth,
			  miGetDefaultVisualMask(pScrn->depth),
			  pScrn->rgbBits, pScrn->defaultVisual))
	return FALSE;

    if (!miSetPixmapDepths())
	return FALSE;

    pScrn->memPhysBase = 0;
    pScrn->fbOffset = 0;

    if (!fbScreenInit(pScreen, NULL,
		      pScrn->virtualX, pScrn->virtualY,
		      pScrn->xDpi, pScrn->yDpi,
		      pScrn->displayWidth, pScrn->bitsPerPixel))
	return FALSE;

    if (pScrn->bitsPerPixel > 8) {
	/* Fixup RGB ordering */
	visual = pScreen->visuals + pScreen->numVisuals;
	while (--visual >= pScreen->visuals) {
	    if ((visual->class | DynamicClass) == DirectColor) {
		visual->offsetRed = pScrn->offset.red;
		visual->offsetGreen = pScrn->offset.green;
		visual->offsetBlue = pScrn->offset.blue;
		visual->redMask = pScrn->mask.red;
		visual->greenMask = pScrn->mask.green;
		visual->blueMask = pScrn->mask.blue;
	    }
	}
    }

    fbPictureInit(pScreen, NULL, 0);

    if (ms->drmmode.shadow_enable && !msShadowInit(pScreen)) {
	xf86DrvMsg(scrnIndex, X_ERROR,
		   "shadow fb init failed\n");
	return FALSE;
    }
  
    ms->createScreenResources = pScreen->CreateScreenResources;
    pScreen->CreateScreenResources = CreateScreenResources;

    xf86SetBlackWhitePixels(pScreen);

    miInitializeBackingStore(pScreen);
    xf86SetBackingStore(pScreen);
    xf86SetSilkenMouse(pScreen);
    miDCInitialize(pScreen, xf86GetPointerScreenFuncs());

    /* Need to extend HWcursor support to handle mask interleave */
    if (!ms->drmmode.sw_cursor)
	xf86_cursors_init(pScreen, 64, 64,
			  HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_64 |
			  HARDWARE_CURSOR_ARGB);

    /* Must force it before EnterVT, so we are in control of VT and
     * later memory should be bound when allocating, e.g rotate_mem */
    pScrn->vtSema = TRUE;

    pScreen->SaveScreen = xf86SaveScreen;
    ms->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = CloseScreen;

    ms->BlockHandler = pScreen->BlockHandler;
    pScreen->BlockHandler = msBlockHandler;

    if (!xf86CrtcScreenInit(pScreen))
	return FALSE;

    if (!miCreateDefColormap(pScreen))
	return FALSE;

    xf86DPMSInit(pScreen, xf86DPMSSet, 0);

    if (serverGeneration == 1)
	xf86ShowUnusedOptions(pScrn->scrnIndex, pScrn->options);

    return EnterVT(scrnIndex, 1);
}

static void
AdjustFrame(int scrnIndex, int x, int y, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(pScrn);
    xf86OutputPtr output = config->output[config->compat_output];
    xf86CrtcPtr crtc = output->crtc;

    if (crtc && crtc->enabled) {
	crtc->funcs->mode_set(crtc, pScrn->currentMode, pScrn->currentMode, x,
			      y);
	crtc->x = output->initial_x + x;
	crtc->y = output->initial_y + y;
    }
}

static void
FreeScreen(int scrnIndex, int flags)
{
    FreeRec(xf86Screens[scrnIndex]);
}

static void
LeaveVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];

    pScrn->vtSema = FALSE;
}

/*
 * This gets called when gaining control of the VT, and from ScreenInit().
 */
static Bool
EnterVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    modesettingPtr ms = modesettingPTR(pScrn);

    pScrn->vtSema = TRUE;

    if (!drmmode_set_desired_modes(pScrn, &ms->drmmode))
	return FALSE;

    return TRUE;
}

static Bool
SwitchMode(int scrnIndex, DisplayModePtr mode, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];

    return xf86SetSingleMode(pScrn, mode, RR_Rotate_0);
}

static Bool
CloseScreen(int scrnIndex, ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    modesettingPtr ms = modesettingPTR(pScrn);

    if (ms->damage) {
	DamageUnregister(&pScreen->GetScreenPixmap(pScreen)->drawable, ms->damage);
	DamageDestroy(ms->damage);
	ms->damage = NULL;
    }

    if (ms->drmmode.shadow_enable) {
	shadowRemove(pScreen, pScreen->GetScreenPixmap(pScreen));
	free(ms->drmmode.shadow_fb);
	ms->drmmode.shadow_fb = NULL;
    }
    drmmode_uevent_fini(pScrn, &ms->drmmode);

    drmmode_free_bos(pScrn, &ms->drmmode);

    if (pScrn->vtSema) {
	LeaveVT(scrnIndex, 0);
    }

    pScreen->CreateScreenResources = ms->createScreenResources;
    pScreen->BlockHandler = ms->BlockHandler;

    drmDropMaster(ms->fd);

    pScrn->vtSema = FALSE;
    pScreen->CloseScreen = ms->CloseScreen;
    return (*pScreen->CloseScreen) (scrnIndex, pScreen);
}

static ModeStatus
ValidMode(int scrnIndex, DisplayModePtr mode, Bool verbose, int flags)
{
    return MODE_OK;
}
