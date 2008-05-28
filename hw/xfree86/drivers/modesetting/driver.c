/*
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * Author: Alan Hourihane <alanh@tungstengraphics.com>
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xf86.h"
#include "xf86_OSproc.h"
#include "compiler.h"
#include "xf86RAC.h"
#include "xf86PciInfo.h"
#include "xf86Pci.h"
#include "xf86Resources.h"
#include "mipointer.h"
#include "micmap.h"
#include "shadowfb.h"
#include <X11/extensions/randr.h>
#include "fb.h"
#include "edid.h"
#include "xf86i2c.h"
#include "xf86Crtc.h"
#include "miscstruct.h"
#include "dixstruct.h"
#include "xf86xv.h"
#include <X11/extensions/Xv.h>
#include "shadow.h"
#include <xorg-server.h>
#if XSERVER_LIBPCIACCESS
#include <pciaccess.h>
#endif

#include "driver.h"

static void AdjustFrame(int scrnIndex, int x, int y, int flags);
static Bool CloseScreen(int scrnIndex, ScreenPtr pScreen);
static Bool EnterVT(int scrnIndex, int flags);
static Bool SaveHWState(ScrnInfoPtr pScrn);
static Bool RestoreHWState(ScrnInfoPtr pScrn);
static void Identify(int flags);
static const OptionInfoRec *AvailableOptions(int chipid, int busid);
static ModeStatus ValidMode(int scrnIndex, DisplayModePtr mode, Bool verbose, int flags);
static void FreeScreen(int scrnIndex, int flags);
static void LeaveVT(int scrnIndex, int flags);
static Bool SwitchMode(int scrnIndex, DisplayModePtr mode, int flags);
static Bool ScreenInit(int scrnIndex, ScreenPtr pScreen, int argc, char **argv);
static Bool PreInit(ScrnInfoPtr pScrn, int flags);
#if XSERVER_LIBPCIACCESS
static Bool
pci_probe(DriverPtr driver,
	  int entity_num, struct pci_device *device, intptr_t match_data);
#else
static Bool Probe(DriverPtr drv, int flags);
#endif

#if XSERVER_LIBPCIACCESS
static const struct pci_id_match device_match[] = {
   {0x8086, 0x2592, 0xffff, 0xffff, 0, 0, 0},
   {0, 0, 0},
};
#endif

_X_EXPORT DriverRec modesetting = {
   1,
   "modesetting",
   Identify,
#if XSERVER_LIBPCIACCESS
   NULL,
#else
   Probe,
#endif
   AvailableOptions,
   NULL,
   0,
   NULL,
#if XSERVER_LIBPCIACCESS
   device_match,
   pci_probe
#endif
};

static SymTabRec Chipsets[] = {
   {0x2592, "Intel Graphics Device"},
   {-1, NULL}
};

static PciChipsets PciDevices[] = {
   {0x2592, 0x2592, RES_SHARED_VGA},
   {-1, -1, RES_UNDEFINED}
};

typedef enum
{
   OPTION_NOACCEL,
   OPTION_SW_CURSOR,
   OPTION_SHADOWFB,
} modesettingOpts;

static const OptionInfoRec Options[] = {
   {OPTION_NOACCEL, "NoAccel", OPTV_BOOLEAN, {0}, FALSE},
   {OPTION_SW_CURSOR, "SWcursor", OPTV_BOOLEAN, {0}, FALSE},
   {OPTION_SHADOWFB, "ShadowFB", OPTV_BOOLEAN, {0}, FALSE},
   {-1, NULL, OPTV_NONE, {0}, FALSE}
};

static const char *fbSymbols[] = {
   "fbPictureInit",
   "fbScreenInit",
   NULL
};

static const char *ddcSymbols[] = {
   "xf86PrintEDID",
   "xf86SetDDCproperties",
   NULL
};

static const char *shadowSymbols[] = {
    "shadowInit",
    "shadowUpdatePackedWeak",
    NULL
};

static const char *i2cSymbols[] = {
   "xf86CreateI2CBusRec",
   "xf86I2CBusInit",
   NULL
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
       * Tell the loader about symbols from other modules that this module
       * might refer to.
       */
      LoaderRefSymLists(fbSymbols,
			shadowSymbols, ddcSymbols, NULL);

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

static const OptionInfoRec *
AvailableOptions(int chipid, int busid)
{
   return Options;
}

#if XSERVER_LIBPCIACCESS
static Bool
pci_probe(DriverPtr driver,
	  int entity_num, struct pci_device *device, intptr_t match_data)
{
   ScrnInfoPtr scrn = NULL;
   EntityInfoPtr entity;
   DevUnion *private;

   scrn = xf86ConfigPciEntity(scrn, 0, entity_num, PciDevices,
			      NULL, NULL, NULL, NULL, NULL);
   if (scrn != NULL) {
      scrn->driverVersion = 1;
      scrn->driverName = "modesetting";
      scrn->name = "modesetting";
      scrn->Probe = NULL;

      entity = xf86GetEntityInfo(entity_num);

      switch (device->device_id) {
      case 0x2592:
	 scrn->PreInit = PreInit;
	 scrn->ScreenInit = ScreenInit;
	 scrn->SwitchMode = SwitchMode;
	 scrn->AdjustFrame = AdjustFrame;
	 scrn->EnterVT = EnterVT;
	 scrn->LeaveVT = LeaveVT;
	 scrn->FreeScreen = FreeScreen;
	 scrn->ValidMode = ValidMode;
	 break;
      }
   }
   return scrn != NULL;
}
#else
static Bool
Probe(DriverPtr drv, int flags)
{
   int i, numUsed, numDevSections, *usedChips;
   EntPtr msEnt = NULL;
   DevUnion *pPriv;
   GDevPtr *devSections;
   Bool foundScreen = FALSE;
   pciVideoPtr *VideoInfo;
   pciVideoPtr *ppPci;
   int numDevs;

   /*
    * Find the config file Device sections that match this
    * driver, and return if there are none.
    */
   if ((numDevSections =
	xf86MatchDevice("modesetting", &devSections)) <= 0) {
      return FALSE;
   }

   /*
    * This probing is just checking the PCI data the server already
    * collected.
    */
   if (!(VideoInfo = xf86GetPciVideoInfo()))
      return FALSE;

#if 0
   numUsed = 0;
   for (ppPci = VideoInfo; ppPci != NULL && *ppPci != NULL; ppPci++) {
      for (numDevs = 0; numDevs < numDevSections; numDevs++) {
         if (devSections[numDevs]->busID && *devSections[numDevs]->busID) {
	 if (xf86ComparePciBusString(devSections[numDevs]->busID, (*ppPci)->bus, (*ppPci)->device, (*ppPci)->func)) {
         /* Claim slot */
         if (xf86CheckPciSlot((*ppPci)->bus, (*ppPci)->device,
				    (*ppPci)->func)) {
	    usedChips[numUsed++] = xf86ClaimPciSlot((*ppPci)->bus, (*ppPci)->device,
				   (*ppPci)->func, drv, (*ppPci)->chipType,
				   NULL, TRUE);
				   ErrorF("CLAIMED %d %d %d\n",(*ppPci)->bus,(*ppPci)->device, (*ppPci)->func);
	 }
	 }
	 }
      }
   }
#else
   /* Look for Intel i8xx devices. */
   numUsed = xf86MatchPciInstances("modesetting", PCI_VENDOR_INTEL,
				   Chipsets, PciDevices,
				   devSections, numDevSections,
				   drv, &usedChips);
#endif

   if (flags & PROBE_DETECT) {
      if (numUsed > 0)
	 foundScreen = TRUE;
   } else {
   ErrorF("NUMUSED %d\n",numUsed);
      for (i = 0; i < numUsed; i++) {
	 ScrnInfoPtr pScrn = NULL;

	 /* Allocate new ScrnInfoRec and claim the slot */
	 if ((pScrn = xf86ConfigPciEntity(pScrn, 0, usedChips[i],
					  PciDevices, NULL, NULL, NULL,
					  NULL, NULL))) {
	    EntityInfoPtr pEnt;

	    pEnt = xf86GetEntityInfo(usedChips[i]);

	    pScrn->driverVersion = 1;
	    pScrn->driverName = "modesetting";
	    pScrn->name = "modesetting";
	    pScrn->Probe = Probe;
	    foundScreen = TRUE;
	    {
	       /* Allocate an entity private if necessary */
	       if (modesettingEntityIndex < 0)
		  modesettingEntityIndex = xf86AllocateEntityPrivateIndex();

	       pPriv = xf86GetEntityPrivate(pScrn->entityList[0],
					    modesettingEntityIndex);
	       if (!pPriv->ptr) {
		  pPriv->ptr = xnfcalloc(sizeof(EntRec), 1);
		  msEnt = pPriv->ptr;
		  msEnt->lastInstance = -1;
	       } else {
		  msEnt = pPriv->ptr;
	       }

	       /*
	        * Set the entity instance for this instance of the driver.
	        * For dual head per card, instance 0 is the "master"
	        * instance, driving the primary head, and instance 1 is
	        * the "slave".
	        */
	       msEnt->lastInstance++;
	       xf86SetEntityInstanceForScreen(pScrn,
					      pScrn->entityList[0],
					      msEnt->lastInstance);
	       pScrn->PreInit = PreInit;
	       pScrn->ScreenInit = ScreenInit;
	       pScrn->SwitchMode = SwitchMode;
	       pScrn->AdjustFrame = AdjustFrame;
	       pScrn->EnterVT = EnterVT;
	       pScrn->LeaveVT = LeaveVT;
	       pScrn->FreeScreen = FreeScreen;
	       pScrn->ValidMode = ValidMode;
	       break;
	    }
	 } else
	 	ErrorF("FAILED PSCRN\n");
      }
   }

   xfree(usedChips);
   xfree(devSections);

   return foundScreen;
}
#endif

static Bool
GetRec(ScrnInfoPtr pScrn)
{
   if (pScrn->driverPrivate)
      return TRUE;

   pScrn->driverPrivate = xnfcalloc(sizeof(modesettingRec), 1);

   return TRUE;
}

static void
FreeRec(ScrnInfoPtr pScrn)
{
   if (!pScrn)
      return;

   if (!pScrn->driverPrivate)
      return;

   xfree(pScrn->driverPrivate);

   pScrn->driverPrivate = NULL;
}

static void
ProbeDDC(ScrnInfoPtr pScrn, int index)
{
   ConfiguredMonitor = NULL;
}

static Bool
MapMem(ScrnInfoPtr pScrn)
{
   modesettingPtr ms = modesettingPTR(pScrn);

   drmBOMap(ms->fd,
	    &ms->bo, DRM_BO_FLAG_READ | DRM_BO_FLAG_WRITE, 0, &ms->virtual);

   return TRUE;
}

static Bool
UnmapMem(ScrnInfoPtr pScrn)
{
   modesettingPtr ms = modesettingPTR(pScrn);

   drmBOUnmap(ms->fd, &ms->bo);

   return TRUE;
}

static void
LoadPalette(ScrnInfoPtr pScrn, int numColors, int *indices,
	    LOCO * colors, VisualPtr pVisual)
{
   xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);
}

static Bool
crtc_resize(ScrnInfoPtr pScrn, int width, int height)
{
   modesettingPtr ms = modesettingPTR(pScrn);
   ScreenPtr pScreen = pScrn->pScreen;
   PixmapPtr rootPixmap = pScreen->GetScreenPixmap(pScreen);
   Bool fbAccessDisabled;
   CARD8 *fbstart;

   if (width == pScrn->virtualX && height == pScrn->virtualY)
      return TRUE;

   ErrorF("RESIZING TO %dx%d\n",width,height);

   pScrn->virtualX = width;
   pScrn->virtualY = height;
   pScrn->displayWidth = (pScrn->virtualX + 63) & ~63;

   if (ms->shadowMem) {
      xfree(ms->shadowMem);
      ms->shadowMem = NULL;
   }

   UnmapMem(pScrn);

   /* move old buffer out of the way */
   drmBOSetStatus(ms->fd, &ms->bo, DRM_BO_FLAG_READ | DRM_BO_FLAG_MEM_LOCAL,
   				   DRM_BO_MASK_MEM | DRM_BO_FLAG_NO_EVICT,
				   DRM_BO_HINT_DONT_FENCE, 0, 0);

   /* unreference it */
   drmBOUnreference(ms->fd, &ms->bo);

   drmBOCreate(ms->fd,
	       pScrn->virtualY * pScrn->displayWidth *
	       pScrn->bitsPerPixel / 8, 0, NULL,
	       DRM_BO_FLAG_READ | DRM_BO_FLAG_WRITE | DRM_BO_FLAG_SHAREABLE
	       | DRM_BO_FLAG_NO_EVICT | DRM_BO_FLAG_MAPPABLE |
	       DRM_BO_FLAG_MEM_VRAM | DRM_BO_FLAG_MEM_TT, DRM_BO_HINT_DONT_FENCE, &ms->bo);

   MapMem(pScrn);

   drmModeAddFB(ms->fd,
		pScrn->virtualX,
		pScrn->virtualY,
		pScrn->depth,
		pScrn->bitsPerPixel,
		pScrn->displayWidth * pScrn->bitsPerPixel / 8,
		ms->bo.handle, 
		&ms->fb_id);

    if (ms->shadowFB) {
	if ((ms->shadowMem =
	     shadowAlloc(pScrn->displayWidth, pScrn->virtualY,
			 pScrn->bitsPerPixel)) == NULL) {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		       "Allocation of shadow memory failed\n");
		return FALSE;
	}
	fbstart = ms->shadowMem;
    } else {
	fbstart = ms->virtual;
    }

    /*
     * If we are in a fb disabled state, the virtual address of the root
     * pixmap should always be NULL, and it will be overwritten later
     * if we try to set it to something.
     *
     * Therefore, set it to NULL, and modify the backup copy instead.
     */

    fbAccessDisabled = (rootPixmap->devPrivate.ptr == NULL);

    pScreen->ModifyPixmapHeader(rootPixmap,
				pScrn->virtualX, pScrn->virtualY,
				pScrn->depth, pScrn->bitsPerPixel,
				pScrn->displayWidth * pScrn->bitsPerPixel / 8, 
				fbstart);

    if (fbAccessDisabled) {
	pScrn->pixmapPrivate.ptr = fbstart;
	rootPixmap->devPrivate.ptr = NULL;
    }

   pScrn->frameX0 = 0;
   pScrn->frameY0 = 0;
   AdjustFrame(pScrn->scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);

   return TRUE;
}

static const xf86CrtcConfigFuncsRec crtc_config_funcs = {
   crtc_resize
};

static Bool
PreInit(ScrnInfoPtr pScrn, int flags)
{
   xf86CrtcConfigPtr xf86_config;
   modesettingPtr ms;
   MessageType from = X_PROBED;
   rgb defaultWeight = { 0, 0, 0 };
   EntityInfoPtr pEnt;
   EntPtr msEnt = NULL;
   int flags24;
   char *BusID;
   int i;
   char *s;
   int num_pipe;
   int max_width, max_height;

   if (pScrn->numEntities != 1)
      return FALSE;

   pEnt = xf86GetEntityInfo(pScrn->entityList[0]);

   if (flags & PROBE_DETECT) {
      ProbeDDC(pScrn, pEnt->index);
      return TRUE;
   }

   /* Allocate driverPrivate */
   if (!GetRec(pScrn))
      return FALSE;

   ms = modesettingPTR(pScrn);
   ms->SaveGeneration = -1;
   ms->pEnt = pEnt;

   pScrn->displayWidth = 640;		/* default it */

   if (ms->pEnt->location.type != BUS_PCI)
      return FALSE;

   ms->PciInfo = xf86GetPciInfoForEntity(ms->pEnt->index);

   /* Allocate an entity private if necessary */
   if (xf86IsEntityShared(pScrn->entityList[0])) {
      msEnt = xf86GetEntityPrivate(pScrn->entityList[0],
				   modesettingEntityIndex)->ptr;
      ms->entityPrivate = msEnt;
   } else
      ms->entityPrivate = NULL;

   if (xf86RegisterResources(ms->pEnt->index, NULL, ResNone)) {
      return FALSE;
   }

   if (xf86IsEntityShared(pScrn->entityList[0])) {
      if (xf86IsPrimInitDone(pScrn->entityList[0])) {
	 /* do something */
      } else {
	 xf86SetPrimInitDone(pScrn->entityList[0]);
      }
   }

   BusID = xalloc(64);
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

   if (ms->fd < 0)
      return FALSE;

   pScrn->racMemFlags = RAC_FB | RAC_COLORMAP;
   pScrn->monitor = pScrn->confScreen->monitor;
   pScrn->progClock = TRUE;
   pScrn->rgbBits = 8;

   flags24 = Support32bppFb | PreferConvert24to32 | SupportConvert24to32;

   if (!xf86SetDepthBpp(pScrn, 0, 0, 0, flags24))
      return FALSE;

   switch (pScrn->depth) {
   case 8:
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

   if (!xf86SetWeight(pScrn, defaultWeight, defaultWeight))
      return FALSE;
   if (!xf86SetDefaultVisual(pScrn, -1))
      return FALSE;

   /* Process the options */
   xf86CollectOptions(pScrn, NULL);
   if (!(ms->Options = xalloc(sizeof(Options))))
      return FALSE;
   memcpy(ms->Options, Options, sizeof(Options));
   xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, ms->Options);

   /* Allocate an xf86CrtcConfig */
   xf86CrtcConfigInit(pScrn, &crtc_config_funcs);
   xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);

   max_width = 8192;
   max_height = 8192;
   xf86CrtcSetSizeRange(pScrn, 320, 200, max_width, max_height);

   if (xf86ReturnOptValBool(ms->Options, OPTION_NOACCEL, FALSE)) {
      ms->noAccel = TRUE;
   }

   if (xf86ReturnOptValBool(ms->Options, OPTION_SW_CURSOR, FALSE)) {
      ms->SWCursor = TRUE;
   }

   if (xf86ReturnOptValBool(ms->Options, OPTION_SHADOWFB, FALSE)) {
	if (!xf86LoadSubModule(pScrn, "shadow"))
	    return FALSE;

	xf86LoaderReqSymLists(shadowSymbols, NULL);

	ms->shadowFB = TRUE;
    }

   SaveHWState(pScrn);

   crtc_init(pScrn);
   output_init(pScrn);

   if (!xf86InitialConfiguration(pScrn, TRUE)) {
      xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No valid modes.\n");
      RestoreHWState(pScrn);
      return FALSE;
   }

   RestoreHWState(pScrn);

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

   xf86LoaderReqSymLists(fbSymbols, NULL);

   return TRUE;
}

static Bool
SaveHWState(ScrnInfoPtr pScrn)
{
   xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);

   return TRUE;
}

static Bool
RestoreHWState(ScrnInfoPtr pScrn)
{
   xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(pScrn);

   return TRUE;
}

static void *
WindowLinear(ScreenPtr pScreen, CARD32 row, CARD32 offset, int mode,
		CARD32 * size, void *closure)
{
   ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
   modesettingPtr ms = modesettingPTR(pScrn);

   if (!pScrn->vtSema)
      return NULL;

   *size = pScrn->displayWidth * pScrn->bitsPerPixel / 8;

   return ((CARD8 *) ms->virtual + row * (*size) + offset);
}

static Bool
CreateScreenResources(ScreenPtr pScreen)
{
   ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
   modesettingPtr ms = modesettingPTR(pScrn);
   Bool ret;

   pScreen->CreateScreenResources = ms->createScreenResources;
   ret = pScreen->CreateScreenResources(pScreen);
   pScreen->CreateScreenResources = CreateScreenResources;

   shadowAdd(pScreen, pScreen->GetScreenPixmap(pScreen),
	      ms->update, WindowLinear, 0, 0);

   return ret;
}

static Bool
ScreenInit(int scrnIndex, ScreenPtr pScreen, int argc, char **argv)
{
   ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
   modesettingPtr ms = modesettingPTR(pScrn);
   VisualPtr visual;
   unsigned long sys_mem;
   int c;
   MessageType from;
   CARD8 *fbstart;

   pScrn->displayWidth = (pScrn->virtualX + 63) & ~63;

   miClearVisualTypes();

   if (!miSetVisualTypes(pScrn->depth,
			 miGetDefaultVisualMask(pScrn->depth),
			 pScrn->rgbBits, pScrn->defaultVisual))
      return FALSE;

   if (!miSetPixmapDepths())
      return FALSE;

   if (!MapMem(pScrn))
      return FALSE;

   pScrn->memPhysBase = 0;
   pScrn->fbOffset = 0;

   drmBOCreate(ms->fd,
	       pScrn->virtualY * pScrn->displayWidth *
	       pScrn->bitsPerPixel / 8, 0, NULL,
	       DRM_BO_FLAG_READ | DRM_BO_FLAG_WRITE | DRM_BO_FLAG_SHAREABLE
	       | DRM_BO_FLAG_NO_EVICT | DRM_BO_FLAG_MAPPABLE |
	       DRM_BO_FLAG_MEM_VRAM | DRM_BO_FLAG_MEM_TT, DRM_BO_HINT_DONT_FENCE, &ms->bo);

   MapMem(pScrn);

   drmModeAddFB(ms->fd,
		pScrn->virtualX,
		pScrn->virtualY,
		pScrn->depth,
		pScrn->bitsPerPixel,
		pScrn->displayWidth * pScrn->bitsPerPixel / 8,
		ms->bo.handle, 
		&ms->fb_id);

    if (ms->shadowFB) {
	if ((ms->shadowMem =
	     shadowAlloc(pScrn->displayWidth, pScrn->virtualY,
			 pScrn->bitsPerPixel)) == NULL) {
	    xf86DrvMsg(scrnIndex, X_ERROR,
		       "Allocation of shadow memory failed\n");
		return FALSE;
	}
	fbstart = ms->shadowMem;
    } else {
	fbstart = ms->virtual;
    }

   if (!fbScreenInit(pScreen, fbstart,
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
    if (ms->shadowFB) {
	ms->update = shadowUpdatePackedWeak();
	if (!shadowSetup(pScreen)) {
	    xf86DrvMsg(scrnIndex, X_ERROR,
		       "Shadow framebuffer initialization failed.\n");
	 	return FALSE;
	}

	ms->createScreenResources = pScreen->CreateScreenResources;
	pScreen->CreateScreenResources = CreateScreenResources;
    }

   xf86SetBlackWhitePixels(pScreen);

#if 0
   glucoseScreenInit(pScreen, 0);
#endif
#if 0
   ms->pExa = ExaInit(pScreen);
#endif

   miInitializeBackingStore(pScreen);
   xf86SetBackingStore(pScreen);
   xf86SetSilkenMouse(pScreen);
   miDCInitialize(pScreen, xf86GetPointerScreenFuncs());

   /* Must force it before EnterVT, so we are in control of VT and
    * later memory should be bound when allocating, e.g rotate_mem */
   pScrn->vtSema = TRUE;

   pScreen->SaveScreen = xf86SaveScreen;
   ms->CloseScreen = pScreen->CloseScreen;
   pScreen->CloseScreen = CloseScreen;

   if (!xf86CrtcScreenInit(pScreen))
      return FALSE;

   if (!miCreateDefColormap(pScreen))
      return FALSE;

#if 0
   if (!xf86HandleColormaps(pScreen, 256, 8, LoadPalette, NULL,
			    CMAP_RELOAD_ON_MODE_SWITCH |
			    CMAP_PALETTED_TRUECOLOR)) {
      return FALSE;
   }
#endif

   xf86DPMSInit(pScreen, xf86DPMSSet, 0);

#if 0
   glucoseInitVideo(pScreen);
#endif

   if (serverGeneration == 1)
      xf86ShowUnusedOptions(pScrn->scrnIndex, pScrn->options);

   return EnterVT(scrnIndex, 0);
}

static void
AdjustFrame(int scrnIndex, int x, int y, int flags)
{
   ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
   xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(pScrn);
   xf86OutputPtr output = config->output[config->compat_output];
   xf86CrtcPtr crtc = output->crtc;

   if (crtc && crtc->enabled) {
      crtc->funcs->mode_set(crtc, pScrn->currentMode, pScrn->currentMode, x, y);
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
   modesettingPtr ms = modesettingPTR(pScrn);
   xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(pScrn);
   int o;

   for (o = 0; o < config->num_crtc; o++) {
      xf86CrtcPtr crtc = config->crtc[o];

      if (crtc->rotatedPixmap || crtc->rotatedData) {
	 crtc->funcs->shadow_destroy(crtc, crtc->rotatedPixmap,
				     crtc->rotatedData);
	 crtc->rotatedPixmap = NULL;
	 crtc->rotatedData = NULL;
      }
   }

   xf86_hide_cursors(pScrn);

   drmMMLock(ms->fd, DRM_BO_MEM_VRAM, 1, 0);

   RestoreHWState(pScrn);
}

/*
 * This gets called when gaining control of the VT, and from ScreenInit().
 */
static Bool
EnterVT(int scrnIndex, int flags)
{
   ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
   modesettingPtr ms = modesettingPTR(pScrn);

   /*
    * Only save state once per server generation since that's what most
    * drivers do.  Could change this to save state at each VT enter.
    */
   if (ms->SaveGeneration != serverGeneration) {
      ms->SaveGeneration = serverGeneration;
      SaveHWState(pScrn);
   }

   drmMMUnlock(ms->fd, DRM_BO_MEM_VRAM, 1);

   if (!xf86SetDesiredModes(pScrn))
      return FALSE;

   AdjustFrame(pScrn->scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);

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

   if (pScrn->vtSema == TRUE) {
      LeaveVT(scrnIndex, 0);
      drmMMUnlock(ms->fd, DRM_BO_MEM_VRAM, 1);
   }

   UnmapMem(pScrn);

   if (ms->shadowFB)
      pScreen->CreateScreenResources = ms->createScreenResources;

   if (ms->shadowMem) {
      xfree(ms->shadowMem);
      ms->shadowMem = NULL;
   }

   if (ms->pExa)
      ExaClose(pScrn);

   /* move old buffer out of the way */
   drmBOSetStatus(ms->fd, &ms->bo, DRM_BO_FLAG_READ | DRM_BO_FLAG_MEM_LOCAL,
   				   DRM_BO_MASK_MEM | DRM_BO_FLAG_NO_EVICT,
				   DRM_BO_HINT_DONT_FENCE, 0, 0);

   drmBOUnreference(ms->fd, &ms->bo);

   drmClose(ms->fd);

   pScrn->vtSema = FALSE;
   pScreen->CloseScreen = ms->CloseScreen;
   return (*pScreen->CloseScreen) (scrnIndex, pScreen);
}

static ModeStatus
ValidMode(int scrnIndex, DisplayModePtr mode, Bool verbose, int flags)
{
   return MODE_OK;
}
