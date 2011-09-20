/*
 * Copyright © 2010 Intel Corporation.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including
 * the next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Kristian Høgsberg <krh@bitplanet.net>
 *
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <gbm.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <xf86drm.h>

#include "../../../mi/micmap.h"
#include <xf86Crtc.h>
#include <xf86.h>
//#define GC XORG_GC
#include <glamor.h>
//#undef GC

#include "glamor_ddx.h"

#define GLAMOR_VERSION_MAJOR 0
#define GLAMOR_VERSION_MINOR 1
#define GLAMOR_VERSION_PATCH 0

static const char glamor_ddx_name[] = "glamor";

static void
glamor_ddx_identify(int flags)
{
	xf86Msg(X_INFO, "Standalone %s: OpenGL accelerated X.org driver\n", glamor_ddx_name);
}

static Bool
glamor_ddx_init_front_buffer(ScrnInfoPtr scrn, int width, int height)
{
	struct glamor_ddx_screen_private *glamor = glamor_ddx_get_screen_private(scrn);

        glamor->root_bo = gbm_bo_create(glamor->gbm, width, height,
                                        GBM_BO_FORMAT_ARGB8888,
                                        GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);

	if (glamor->root_bo == NULL)
		return FALSE;

        scrn->virtualX = width;
        scrn->virtualY = height; 
        /* XXX shall we update displayWidth here ? */
        return TRUE;
}

static Bool
glamor_create_screen_image(ScrnInfoPtr scrn)
{
	struct glamor_ddx_screen_private *glamor = glamor_ddx_get_screen_private(scrn);
	ScreenPtr screen = screenInfo.screens[scrn->scrnIndex];
	unsigned int handle, stride;
        handle = gbm_bo_get_handle(glamor->root_bo).u32;
        stride = gbm_bo_get_pitch(glamor->root_bo);
        return glamor_create_egl_screen_image(screen, handle, stride);
}

Bool
glamor_front_buffer_resize(ScrnInfoPtr scrn, int width, int height)
{
	struct glamor_ddx_screen_private *glamor = glamor_ddx_get_screen_private(scrn);
	ScreenPtr screen = screenInfo.screens[scrn->scrnIndex];

        if (glamor->root_bo != NULL) {
          glamor_close_egl_screen(screen);
          gbm_bo_destroy(glamor->root_bo);
          glamor->root_bo = NULL;
        } 

        if (!glamor_ddx_init_front_buffer(scrn, width, height)) 
           return FALSE;
        return glamor_create_screen_image(scrn);
}

void
glamor_frontbuffer_handle(ScrnInfoPtr scrn, uint32_t *handle, uint32_t *pitch)
{
	struct glamor_ddx_screen_private *glamor = glamor_ddx_get_screen_private(scrn);
        *handle = gbm_bo_get_handle(glamor->root_bo).u32;
        *pitch = gbm_bo_get_pitch(glamor->root_bo);
}

Bool
glamor_create_cursor(ScrnInfoPtr scrn, struct glamor_gbm_cursor *cursor, int width, int height)
{
	struct glamor_ddx_screen_private *glamor = glamor_ddx_get_screen_private(scrn);
	ScreenPtr screen = screenInfo.screens[scrn->scrnIndex];
        unsigned int handle, stride;
        

        if (cursor->cursor_pixmap) 
          glamor_destroy_cursor(scrn, cursor);

        cursor->cursor_pixmap = screen->CreatePixmap(screen, 0, 0, 32, 0);
        if (cursor->cursor_pixmap == NULL)
           return FALSE;
        screen->ModifyPixmapHeader(cursor->cursor_pixmap, width, height, 0, 0, 0, 0);
        cursor->cursor_bo = gbm_bo_create(glamor->gbm, width, height,
                                          GBM_BO_FORMAT_ARGB8888,
                                          GBM_BO_USE_SCANOUT 
                                          | GBM_BO_USE_RENDERING 
                                          | GBM_BO_USE_CURSOR_64X64);

	if (cursor->cursor_bo == NULL) 
          goto fail; 
        glamor_cursor_handle(cursor, &handle, &stride);
        if (!glamor_create_egl_pixmap_image(cursor->cursor_pixmap, handle, stride))
          goto fail;

        return TRUE;

fail:
        glamor_destroy_cursor(scrn, cursor);
        return FALSE; 
}

void glamor_destroy_cursor(ScrnInfoPtr scrn, struct glamor_gbm_cursor *cursor)
{
	ScreenPtr screen = screenInfo.screens[scrn->scrnIndex];
        if (cursor->cursor_pixmap)
          screen->DestroyPixmap(cursor->cursor_pixmap);
        if (cursor->cursor_bo)
          gbm_bo_destroy(cursor->cursor_bo);  
        cursor->cursor_bo = NULL;
        cursor->cursor_pixmap = NULL;
}

void
glamor_cursor_handle(struct glamor_gbm_cursor *cursor, uint32_t *handle, uint32_t *pitch)
{
        *handle = gbm_bo_get_handle(cursor->cursor_bo).u32;
        *pitch = gbm_bo_get_pitch(cursor->cursor_bo);
	ErrorF("cursor stride: %d\n", *pitch);
}

char * dri_device_name = "/dev/dri/card0";
static Bool
glamor_ddx_pre_init(ScrnInfoPtr scrn, int flags)
{
	struct glamor_ddx_screen_private *glamor;
	rgb defaultWeight = { 0, 0, 0 };

	glamor = xnfcalloc(sizeof *glamor, 1);

	scrn->driverPrivate = glamor;
	glamor->fd = open(dri_device_name, O_RDWR);
	if (glamor->fd == -1 ) {
	  ErrorF("Failed to open %s: %s\n", dri_device_name, strerror(errno));
	  goto fail;
	}

	glamor->cpp = 4;

	scrn->monitor = scrn->confScreen->monitor;
	scrn->progClock = TRUE;
	scrn->rgbBits = 8;

	if (!xf86SetDepthBpp(scrn, 0, 0, 0, Support32bppFb))
	  goto fail;

	xf86PrintDepthBpp(scrn);

	if (!xf86SetWeight(scrn, defaultWeight, defaultWeight))
	  goto fail;
	if (!xf86SetDefaultVisual(scrn, -1))
	  goto fail;

	glamor->cpp = scrn->bitsPerPixel / 8;

	if (drmmode_pre_init(scrn, glamor->fd, glamor->cpp) == FALSE) {
		xf86DrvMsg(scrn->scrnIndex, X_ERROR,
			   "Kernel modesetting setup failed\n");
         goto fail;
	}

	scrn->currentMode = scrn->modes;
	xf86SetDpi(scrn, 0, 0);

	/* Load the required sub modules */
	if (!xf86LoadSubModule(scrn, "fb"))
	  goto fail;

	if (!xf86LoadSubModule(scrn, "glamor_dix"))
	  goto fail;

	return TRUE;

fail:
	scrn->driverPrivate = NULL;
	free(glamor);
	return FALSE;
}

static void
glamor_ddx_adjust_frame(int scrnIndex, int x, int y, int flags)
{
}

static Bool
glamor_ddx_enter_vt(int scrnIndex, int flags)
{
	ScrnInfoPtr scrn = xf86Screens[scrnIndex];
	struct glamor_ddx_screen_private *glamor = glamor_ddx_get_screen_private(scrn);

	if (drmSetMaster(glamor->fd)) {
		xf86DrvMsg(scrn->scrnIndex, X_WARNING,
			   "drmSetMaster failed: %s\n", strerror(errno));
		return FALSE;
	}
        if (!xf86SetDesiredModes(scrn))
	  return FALSE;
	return TRUE;
}

static void
glamor_ddx_leave_vt(int scrnIndex, int flags)
{
	ScrnInfoPtr scrn = xf86Screens[scrnIndex];
	struct glamor_ddx_screen_private *glamor = glamor_ddx_get_screen_private(scrn);

	drmDropMaster(glamor->fd);
}

static Bool
glamor_ddx_create_screen_resources(ScreenPtr screen)
{
	ScrnInfoPtr scrn = xf86Screens[screen->myNum];
	struct glamor_ddx_screen_private *glamor = glamor_ddx_get_screen_private(scrn);

	screen->CreateScreenResources = glamor->CreateScreenResources;
	if (!(*screen->CreateScreenResources) (screen))
		return FALSE;
        if (!glamor_glyphs_init(screen))
                return FALSE;
        if (glamor->root_bo == NULL)
                return FALSE;
        if (!glamor_create_screen_image(scrn))
                return FALSE;
	return TRUE;
}

static Bool
glamor_ddx_close_screen(int scrnIndex, ScreenPtr screen)
{
	ScrnInfoPtr scrn = xf86Screens[scrnIndex];
	struct glamor_ddx_screen_private *glamor = glamor_ddx_get_screen_private(scrn);

	screen->CloseScreen = glamor->CloseScreen;
	(*screen->CloseScreen) (scrnIndex, screen);

	if (scrn->vtSema == TRUE)
		glamor_ddx_leave_vt(scrnIndex, 0);

	glamor_fini(screen);
        glamor_close_egl_screen(screen);
        gbm_bo_destroy(glamor->root_bo);
	drmmode_closefb(scrn);
	
	return TRUE;
}

static Bool
glamor_ddx_screen_init(int scrnIndex, ScreenPtr screen, int argc, char **argv)
{
	ScrnInfoPtr scrn = xf86Screens[screen->myNum];
	struct glamor_ddx_screen_private *glamor = glamor_ddx_get_screen_private(scrn);
	VisualPtr visual;

        glamor->gbm = gbm_create_device(glamor->fd);
        if (glamor->gbm == NULL) {
          ErrorF("couldn't create gbm device\n");
          return FALSE;
        }

	glamor_egl_init(scrn, glamor->fd);

	miClearVisualTypes();
	if (!miSetVisualTypes(scrn->depth,
			      miGetDefaultVisualMask(scrn->depth),
			      scrn->rgbBits, scrn->defaultVisual))
		return FALSE;
	if (!miSetPixmapDepths())
		return FALSE;

        if (!glamor_ddx_init_front_buffer(scrn, scrn->virtualX, scrn->virtualY))
                return FALSE;

	if (!fbScreenInit(screen, NULL,
			  scrn->virtualX, scrn->virtualY,
			  scrn->xDpi, scrn->yDpi,
			  1, scrn->bitsPerPixel))
		return FALSE;

	if (scrn->bitsPerPixel > 8) {
	  /* Fixup RGB ordering */
	  visual = screen->visuals + screen->numVisuals;
	  while(--visual >= screen->visuals) {
	    if ((visual->class | DynamicClass) == DirectColor) {
	      visual->offsetRed = scrn->offset.red;
	      visual->offsetGreen = scrn->offset.green;
	      visual->offsetBlue = scrn->offset.blue;
	      visual->redMask = scrn->mask.red;
	      visual->blueMask = scrn->mask.blue;
	    }
	  }
	}

	fbPictureInit(screen, NULL, 0);
	xf86SetBlackWhitePixels(screen);

	if (!glamor_init(screen, GLAMOR_INVERTED_Y_AXIS)) {
		xf86DrvMsg(scrn->scrnIndex, X_ERROR,
			   "Failed to initialize glamor\n");
		return FALSE;
	}
	
	miInitializeBackingStore(screen);
	xf86SetBackingStore(screen);
	xf86SetSilkenMouse(screen);
	miDCInitialize(screen, xf86GetPointerScreenFuncs());

	xf86DrvMsg(scrn->scrnIndex, X_INFO, "Initializing HW Cursor\n");

	if (!xf86_cursors_init(screen, 64, 64,
			       (HARDWARE_CURSOR_TRUECOLOR_AT_8BPP |
				HARDWARE_CURSOR_BIT_ORDER_MSBFIRST |
				HARDWARE_CURSOR_INVERT_MASK |
				HARDWARE_CURSOR_SWAP_SOURCE_AND_MASK |
				HARDWARE_CURSOR_AND_SOURCE_WITH_MASK |
				HARDWARE_CURSOR_SOURCE_MASK_NOT_INTERLEAVED |
				HARDWARE_CURSOR_UPDATE_UNHIDDEN |
				HARDWARE_CURSOR_ARGB))) {
		xf86DrvMsg(scrn->scrnIndex, X_ERROR,
			   "Hardware cursor initialization failed\n");
	}

	/* Must force it before EnterVT, so we are in control of VT and
	 * later memory should be bound when allocating, e.g rotate_mem */
	scrn->vtSema = TRUE;

	if (!glamor_ddx_enter_vt(scrnIndex, 0))
		return FALSE;

	screen->SaveScreen = xf86SaveScreen;
	glamor->CreateScreenResources = screen->CreateScreenResources;
	screen->CreateScreenResources = glamor_ddx_create_screen_resources;
	glamor->CloseScreen = screen->CloseScreen;
	screen->CloseScreen = glamor_ddx_close_screen;
	/* Fixme should we init crtc screen here? */
	if (!xf86CrtcScreenInit(screen))
	  return FALSE;
	if (!miCreateDefColormap(screen))
	  return FALSE;
	/* Fixme should we add handle colormap here? */

	xf86DPMSInit(screen, xf86DPMSSet, 0);

	return TRUE;
}

static void
glamor_ddx_free_screen(int scrnIndex, int flags)
{
	ScrnInfoPtr scrn = xf86Screens[scrnIndex];
	struct glamor_ddx_screen_private *glamor = glamor_ddx_get_screen_private(scrn);
	if (glamor != NULL)
	{
	  close(glamor->fd);
	  free(glamor);
	  scrn->driverPrivate = NULL;
	}
}

static ModeStatus
glamor_ddx_valid_mode(int scrnIndex, DisplayModePtr mode, Bool verbose, int flags)
{
	if (mode->Flags & V_INTERLACE) {
		if (verbose) {
			xf86DrvMsg(scrnIndex, X_PROBED,
				   "Removing interlaced mode \"%s\"\n",
				   mode->name);
		}
		return MODE_BAD;
	}
	return MODE_OK;
}

static Bool
glamor_ddx_probe(struct _DriverRec *drv, int flags)
{
	ScrnInfoPtr scrn = NULL;
       	GDevPtr *sections;
	int entity, n;

	n = xf86MatchDevice(glamor_ddx_name, &sections);
	if (n <= 0)
	    return FALSE;

	entity = xf86ClaimFbSlot(drv, 0, sections[0], TRUE);

	scrn = xf86ConfigFbEntity(scrn, 0, entity, NULL, NULL, NULL, NULL);
	if (scrn == NULL) {
		xf86Msg(X_ERROR, "Failed to add fb entity\n");
		return FALSE;
	}

	scrn->driverVersion = 1;
	scrn->driverName = (char *) glamor_ddx_name;
	scrn->name = (char *) glamor_ddx_name;
	scrn->Probe = NULL;

	scrn->PreInit = glamor_ddx_pre_init;
	scrn->ScreenInit = glamor_ddx_screen_init;
	scrn->AdjustFrame = glamor_ddx_adjust_frame;
	scrn->EnterVT = glamor_ddx_enter_vt;
	scrn->LeaveVT = glamor_ddx_leave_vt;
	scrn->FreeScreen = glamor_ddx_free_screen;
	scrn->ValidMode = glamor_ddx_valid_mode;

	return TRUE;
}

static const OptionInfoRec *
glamor_ddx_available_options(int chipid, int busid)
{
	return NULL;
}

static Bool
glamor_ddx_driver_func(ScrnInfoPtr pScrn, xorgDriverFuncOp op, pointer ptr)
{
	xorgHWFlags *flag;

	switch (op) {
        case GET_REQUIRED_HW_INTERFACES:
		flag = (CARD32*)ptr;
		(*flag) = 0;
		return TRUE;
        default:
		/* Unknown or deprecated function */
		return FALSE;
	}
}

_X_EXPORT DriverRec glamor_ddx = {
	1,
	"glamor",
	glamor_ddx_identify,
	glamor_ddx_probe,
	glamor_ddx_available_options,
	NULL,
	0,
	glamor_ddx_driver_func,
};

static pointer
glamor_ddx_setup(pointer module, pointer opts, int *errmaj, int *errmin)
{
   static Bool setupDone = 0;

   /* This module should be loaded only once, but check to be sure.
    */
   if (!setupDone) {
      setupDone = 1;
      xf86AddDriver(&glamor_ddx, module, HaveDriverFuncs);

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

static XF86ModuleVersionInfo glamor_ddx_version_info = {
	glamor_ddx_name,
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XORG_VERSION_CURRENT,
	GLAMOR_VERSION_MAJOR,
	GLAMOR_VERSION_MINOR,
	GLAMOR_VERSION_PATCH,
	ABI_CLASS_VIDEODRV,
	ABI_VIDEODRV_VERSION,
	MOD_CLASS_VIDEODRV,
	{0, 0, 0, 0}
};

_X_EXPORT XF86ModuleData glamorModuleData = {
	&glamor_ddx_version_info,
	glamor_ddx_setup,
	NULL
};
