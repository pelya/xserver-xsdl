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
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <xf86drm.h>

#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#define EGL_DISPLAY_NO_X_MESA

#if GLAMOR_GLES2
#include <GLES2/gl2.h>
#else
#include <GL/gl.h>
#endif

#define MESA_EGL_NO_X11_HEADERS
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "../../../mi/micmap.h"
#include <xf86Crtc.h>
#include <xf86.h>
#define GC XORG_GC
#include <glamor.h>
#undef GC

#include "glamor_ddx.h"

#define GLAMOR_VERSION_MAJOR 0
#define GLAMOR_VERSION_MINOR 1
#define GLAMOR_VERSION_PATCH 0

static const char glamor_name[] = "glamor";

static void
glamor_identify(int flags)
{
	xf86Msg(X_INFO, "%s: OpenGL accelerated X.org driver\n", glamor_name);
}

struct glamor_screen_private {
	EGLDisplay display;
	EGLContext context;
	EGLImageKHR root;
	EGLint major, minor;

	CreateScreenResourcesProcPtr CreateScreenResources;
	CloseScreenProcPtr CloseScreen;
	int fd;
	int cpp;

	PFNEGLCREATEDRMIMAGEMESA egl_create_drm_image_mesa;
	PFNEGLEXPORTDRMIMAGEMESA egl_export_drm_image_mesa;

};

static inline struct glamor_screen_private *
glamor_get_screen_private(ScrnInfoPtr scrn)
{
	return (struct glamor_screen_private *) (scrn->driverPrivate);
}

Bool
glamor_resize(ScrnInfoPtr scrn, int width, int height)
{
	struct glamor_screen_private *glamor = glamor_get_screen_private(scrn);
	ScreenPtr screen = screenInfo.screens[scrn->scrnIndex];
	EGLImageKHR image;
	GLuint texture;
	EGLint attribs[] = {
		EGL_WIDTH,	0,
		EGL_HEIGHT,	0,
		EGL_DRM_BUFFER_FORMAT_MESA,	EGL_DRM_BUFFER_FORMAT_ARGB32_MESA,
		EGL_DRM_BUFFER_USE_MESA,	EGL_DRM_BUFFER_USE_SHARE_MESA |
					EGL_DRM_BUFFER_USE_SCANOUT_MESA,
		EGL_NONE
	};

	if (glamor->root != EGL_NO_IMAGE_KHR &&
	    scrn->virtualX == width && scrn->virtualY == height)
		return TRUE;

	attribs[1] = width;
	attribs[3] = height;
	image =	 (glamor->egl_create_drm_image_mesa)(glamor->display, attribs);  
	if (image == EGL_NO_IMAGE_KHR)
		return FALSE;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image); 

	glamor_set_screen_pixmap_texture(screen, width, height, texture);
	glamor->root = image;
	scrn->virtualX = width;
	scrn->virtualY = height;
	return TRUE;
}

void
glamor_frontbuffer_handle(ScrnInfoPtr scrn, uint32_t *handle, uint32_t *pitch)
{
	struct glamor_screen_private *glamor = glamor_get_screen_private(scrn);
	EGLint name;
	(glamor->egl_export_drm_image_mesa)(glamor->display, glamor->root, &name, (EGLint*) handle, (EGLint*) pitch);
}

EGLImageKHR glamor_create_cursor_argb(ScrnInfoPtr scrn, int width, int height)
{
	struct glamor_screen_private *glamor = glamor_get_screen_private(scrn);
	EGLint attribs[] = {
		EGL_WIDTH,	0,
		EGL_HEIGHT,	0,
		EGL_DRM_BUFFER_FORMAT_MESA,	EGL_DRM_BUFFER_FORMAT_ARGB32_MESA,
		EGL_DRM_BUFFER_USE_MESA,	EGL_DRM_BUFFER_USE_SHARE_MESA |
					EGL_DRM_BUFFER_USE_SCANOUT_MESA | EGL_DRM_BUFFER_USE_CURSOR_MESA,
		EGL_NONE
	};

	attribs[1] = width;
	attribs[3] = height;
	return	(glamor->egl_create_drm_image_mesa)(glamor->display, attribs); 
}

void glamor_destroy_cursor(ScrnInfoPtr scrn, EGLImageKHR cursor)
{
	struct glamor_screen_private *glamor = glamor_get_screen_private(scrn);
        eglDestroyImageKHR(glamor->display, cursor);
}

void
glamor_cursor_handle(ScrnInfoPtr scrn, EGLImageKHR cursor, uint32_t *handle, uint32_t *pitch)
{
	struct glamor_screen_private *glamor = glamor_get_screen_private(scrn);
	EGLint name;
	(glamor->egl_export_drm_image_mesa)(glamor->display, cursor, &name, (EGLint*) handle, (EGLint*) pitch);
	ErrorF("cursor stride: %d\n", *pitch);
}

char * dri_device_name = "/dev/dri/card0";
static Bool
glamor_pre_init_ddx(ScrnInfoPtr scrn, int flags)
{
	struct glamor_screen_private *glamor;
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

	return TRUE;

fail:
	scrn->driverPrivate = NULL;
	free(glamor);
	return FALSE;
}

static void
glamor_adjust_frame_ddx(int scrnIndex, int x, int y, int flags)
{
}

static Bool
glamor_enter_vt_ddx(int scrnIndex, int flags)
{
	ScrnInfoPtr scrn = xf86Screens[scrnIndex];
	struct glamor_screen_private *glamor = glamor_get_screen_private(scrn);

	if (drmSetMaster(glamor->fd)) {
		xf86DrvMsg(scrn->scrnIndex, X_WARNING,
			   "drmSetMaster failed: %s\n", strerror(errno));
		return FALSE;
	}
#if 0
        if (!xf86SetDesiredModes(scrn))
	  return FALSE;
#endif               
	return TRUE;
}

static void
glamor_leave_vt_ddx(int scrnIndex, int flags)
{
	ScrnInfoPtr scrn = xf86Screens[scrnIndex];
	struct glamor_screen_private *glamor = glamor_get_screen_private(scrn);

	drmDropMaster(glamor->fd);
}

static Bool
glamor_create_screen_resources_ddx(ScreenPtr screen)
{
	ScrnInfoPtr scrn = xf86Screens[screen->myNum];
	struct glamor_screen_private *glamor = glamor_get_screen_private(scrn);

	screen->CreateScreenResources = glamor->CreateScreenResources;
	if (!(*screen->CreateScreenResources) (screen))
		return FALSE;


	if (!xf86SetDesiredModes(scrn))
		return FALSE;

	return TRUE;
}

static Bool
glamor_close_screen_ddx(int scrnIndex, ScreenPtr screen)
{
	ScrnInfoPtr scrn = xf86Screens[scrnIndex];
	struct glamor_screen_private *glamor = glamor_get_screen_private(scrn);

	screen->CloseScreen = glamor->CloseScreen;
	(*screen->CloseScreen) (scrnIndex, screen);

	if (scrn->vtSema == TRUE)
		glamor_leave_vt_ddx(scrnIndex, 0);

	glamor_fini(screen);

        eglDestroyImageKHR(glamor->display, glamor->root);

	eglMakeCurrent(glamor->display,
		       EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglTerminate(glamor->display);

	drmmode_closefb(scrn);
	
	glamor->fd = -1;

	glamor->root = EGL_NO_IMAGE_KHR;

	return TRUE;
}

static Bool
glamor_screen_init_ddx(int scrnIndex, ScreenPtr screen, int argc, char **argv)
{
	ScrnInfoPtr scrn = xf86Screens[screen->myNum];
	struct glamor_screen_private *glamor = glamor_get_screen_private(scrn);
	const char *version;
	VisualPtr visual;
        EGLint config_attribs[] = {
#ifdef GLAMOR_GLES2
        EGL_CONTEXT_CLIENT_VERSION, 2, 
#endif
          EGL_NONE
    };


	/* If serverGeneration != 1 then fd was closed during the last
	 time closing screen, actually in eglTerminate(). */

	if (glamor->fd == -1 && serverGeneration != 1) {
	  glamor->fd = open(dri_device_name, O_RDWR);
	  if (glamor->fd == -1 ) {
	    ErrorF("Failed to open %s: %s\n", dri_device_name, strerror(errno));
	    return FALSE;
	  }
	if (drmmode_pre_init(scrn, glamor->fd, glamor->cpp) == FALSE) {
		xf86DrvMsg(scrn->scrnIndex, X_ERROR,
			   "Kernel modesetting setup failed\n");
            return FALSE;
	}

	}



	glamor->display = eglGetDRMDisplayMESA(glamor->fd);
#ifndef GLAMOR_GLES2
	eglBindAPI(EGL_OPENGL_API);
#else
	eglBindAPI(EGL_OPENGL_ES_API);
#endif
	if (!eglInitialize(glamor->display, &glamor->major, &glamor->minor)) {
		xf86DrvMsg(scrn->scrnIndex, X_ERROR,
			   "eglInitialize() failed\n");
		return FALSE;
	}

	version = eglQueryString(glamor->display, EGL_VERSION);
	xf86Msg(X_INFO, "%s: EGL version %s:\n", glamor_name, version);

	glamor->egl_create_drm_image_mesa = eglGetProcAddress("eglCreateDRMImageMESA");
	glamor->egl_export_drm_image_mesa = eglGetProcAddress("eglExportDRMImageMESA");

	if (!glamor->egl_create_drm_image_mesa || !glamor->egl_export_drm_image_mesa) {
		xf86DrvMsg(scrn->scrnIndex, X_ERROR,
			   "eglGetProcAddress() failed\n");
		return FALSE;
	}

	glamor->context = eglCreateContext(glamor->display,
					   NULL, EGL_NO_CONTEXT, config_attribs);
	if (glamor->context == EGL_NO_CONTEXT) {
		xf86DrvMsg(scrn->scrnIndex, X_ERROR,
			   "Failed to create EGL context\n");
		return FALSE;
	}

	if (!eglMakeCurrent(glamor->display,
			    EGL_NO_SURFACE, EGL_NO_SURFACE, glamor->context)) {
		xf86DrvMsg(scrn->scrnIndex, X_ERROR,
			   "Failed to make EGL context current\n");
		return FALSE;
	}

	miClearVisualTypes();
	if (!miSetVisualTypes(scrn->depth,
			      miGetDefaultVisualMask(scrn->depth),
			      scrn->rgbBits, scrn->defaultVisual))
		return FALSE;
	if (!miSetPixmapDepths())
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

	if (!glamor_enter_vt_ddx(scrnIndex, 0))
		return FALSE;

	screen->SaveScreen = xf86SaveScreen;
	glamor->CreateScreenResources = screen->CreateScreenResources;
	screen->CreateScreenResources = glamor_create_screen_resources_ddx;
	glamor->CloseScreen = screen->CloseScreen;
	screen->CloseScreen = glamor_close_screen_ddx;
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
glamor_free_screen_ddx(int scrnIndex, int flags)
{
	ScrnInfoPtr scrn = xf86Screens[scrnIndex];
	struct glamor_screen_private *glamor = glamor_get_screen_private(scrn);
	if (glamor != NULL)
	{
	  close(glamor->fd);
	  free(glamor);
	  scrn->driverPrivate = NULL;
	}
}

static ModeStatus
glamor_valid_mode_ddx(int scrnIndex, DisplayModePtr mode, Bool verbose, int flags)
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
glamor_probe(struct _DriverRec *drv, int flags)
{
	ScrnInfoPtr scrn = NULL;
       	GDevPtr *sections;
	int entity, n;

	n = xf86MatchDevice(glamor_name, &sections);
	if (n <= 0)
	    return FALSE;

	entity = xf86ClaimFbSlot(drv, 0, sections[0], TRUE);

	scrn = xf86ConfigFbEntity(scrn, 0, entity, NULL, NULL, NULL, NULL);
	if (scrn == NULL) {
		xf86Msg(X_ERROR, "Failed to add fb entity\n");
		return FALSE;
	}

	scrn->driverVersion = 1;
	scrn->driverName = (char *) glamor_name;
	scrn->name = (char *) glamor_name;
	scrn->Probe = NULL;

	scrn->PreInit = glamor_pre_init_ddx;
	scrn->ScreenInit = glamor_screen_init_ddx;
	scrn->AdjustFrame = glamor_adjust_frame_ddx;
	scrn->EnterVT = glamor_enter_vt_ddx;
	scrn->LeaveVT = glamor_leave_vt_ddx;
	scrn->FreeScreen = glamor_free_screen_ddx;
	scrn->ValidMode = glamor_valid_mode_ddx;

	return TRUE;
}

static const OptionInfoRec *
glamor_available_options(int chipid, int busid)
{
	return NULL;
}

static Bool
glamor_driver_func(ScrnInfoPtr pScrn, xorgDriverFuncOp op, pointer ptr)
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

_X_EXPORT DriverRec glamor = {
	1,
	"glamor",
	glamor_identify,
	glamor_probe,
	glamor_available_options,
	NULL,
	0,
	glamor_driver_func,
};

static pointer
glamor_setup(pointer module, pointer opts, int *errmaj, int *errmin)
{
   static Bool setupDone = 0;

   /* This module should be loaded only once, but check to be sure.
    */
   if (!setupDone) {
      setupDone = 1;
      xf86AddDriver(&glamor, module, HaveDriverFuncs);

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

static XF86ModuleVersionInfo glamor_version_info = {
	glamor_name,
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
	&glamor_version_info,
	glamor_setup,
	NULL
};
