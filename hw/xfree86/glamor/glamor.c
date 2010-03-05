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

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <xf86drm.h>

#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#define EGL_DISPLAY_NO_X_MESA
#include <GL/gl.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "../../../mi/micmap.h"
#include <xf86Crtc.h>
#include <xf86.h>
#include <glamor.h>

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
	EGLImageKHR root, cursor;
	EGLint major, minor;
	GLuint cursor_tex;

	CreateScreenResourcesProcPtr CreateScreenResources;
	CloseScreenProcPtr CloseScreen;
	int fd;
	int cpp;
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
		EGL_IMAGE_WIDTH_INTEL,	0,
		EGL_IMAGE_HEIGHT_INTEL,	0,
		EGL_IMAGE_FORMAT_INTEL,	EGL_FORMAT_RGBA_8888_KHR,
		EGL_IMAGE_USE_INTEL,	EGL_IMAGE_USE_SHARE_INTEL |
					EGL_IMAGE_USE_SCANOUT_INTEL,
		EGL_NONE
	};

	if (glamor->root != EGL_NO_IMAGE_KHR &&
	    scrn->virtualX == width && scrn->virtualY == height)
		return TRUE;

	attribs[1] = width;
	attribs[3] = height;
	image = eglCreateImageKHR(glamor->display, glamor->context,
				  EGL_SYSTEM_IMAGE_INTEL,
				  NULL, attribs);
	if (image == EGL_NO_IMAGE_KHR)
		return FALSE;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image);

	glamor_set_pixmap_texture(screen->GetScreenPixmap(screen),
				  width, height, texture);
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

	eglShareImageINTEL (glamor->display, glamor->context, glamor->root, 0,
			    &name, (EGLint *) handle, (EGLint *) pitch);
}

Bool
glamor_load_cursor(ScrnInfoPtr scrn, CARD32 *image, int width, int height)
{
	struct glamor_screen_private *glamor = glamor_get_screen_private(scrn);

	if (glamor->cursor == NULL) {
		EGLint attribs[] = {
			EGL_IMAGE_WIDTH_INTEL,	0,
			EGL_IMAGE_HEIGHT_INTEL,	0,
			EGL_IMAGE_FORMAT_INTEL,	EGL_FORMAT_RGBA_8888_KHR,
			EGL_IMAGE_USE_INTEL,	EGL_IMAGE_USE_SHARE_INTEL |
						EGL_IMAGE_USE_SCANOUT_INTEL,
			EGL_NONE
		};

		attribs[1] = width;
		attribs[3] = height;
		glamor->cursor =
			eglCreateImageKHR(glamor->display, glamor->context,
					  EGL_SYSTEM_IMAGE_INTEL,
					  NULL, attribs);
		if (image == EGL_NO_IMAGE_KHR)
			return FALSE;

		glGenTextures(1, &glamor->cursor_tex);
		glBindTexture(GL_TEXTURE_2D, glamor->cursor_tex);
		glTexParameteri(GL_TEXTURE_2D,
				GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,
				GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, glamor->cursor);
	}

	glBindTexture(GL_TEXTURE_2D, glamor->cursor_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0,
		     GL_RGBA, GL_UNSIGNED_BYTE, image);

	return TRUE;
}

void
glamor_cursor_handle(ScrnInfoPtr scrn, uint32_t *handle, uint32_t *pitch)
{
	struct glamor_screen_private *glamor = glamor_get_screen_private(scrn);
	EGLint name;

	eglShareImageINTEL (glamor->display, glamor->context, glamor->cursor, 
			    0, &name, (EGLint *) handle, (EGLint *) pitch);
	ErrorF("cursor stride: %d\n", *pitch);
}

static Bool
glamor_pre_init(ScrnInfoPtr scrn, int flags)
{
	struct glamor_screen_private *glamor;
	rgb defaultWeight = { 0, 0, 0 };

	glamor = xnfcalloc(sizeof *glamor, 1);

	scrn->driverPrivate = glamor;

	glamor->fd = open("/dev/dri/card0", O_RDWR);
	glamor->cpp = 4;

	scrn->monitor = scrn->confScreen->monitor;
	scrn->progClock = TRUE;
	scrn->rgbBits = 8;

	if (!xf86SetDepthBpp(scrn, 0, 0, 0, Support32bppFb))
		return FALSE;

	xf86PrintDepthBpp(scrn);

	if (!xf86SetWeight(scrn, defaultWeight, defaultWeight))
		return FALSE;
	if (!xf86SetDefaultVisual(scrn, -1))
		return FALSE;

	glamor->cpp = scrn->bitsPerPixel / 8;

	if (drmmode_pre_init(scrn, glamor->fd, glamor->cpp) == FALSE) {
		xf86DrvMsg(scrn->scrnIndex, X_ERROR,
			   "Kernel modesetting setup failed\n");
		xfree(glamor);
		return FALSE;
	}

	scrn->currentMode = scrn->modes;
	xf86SetDpi(scrn, 0, 0);

	/* Load the required sub modules */
	if (!xf86LoadSubModule(scrn, "fb"))
		return FALSE;

	return TRUE;
}

static void
glamor_adjust_frame(int scrnIndex, int x, int y, int flags)
{
}

static Bool
glamor_enter_vt(int scrnIndex, int flags)
{
	ScrnInfoPtr scrn = xf86Screens[scrnIndex];
	struct glamor_screen_private *glamor = glamor_get_screen_private(scrn);

	if (drmSetMaster(glamor->fd)) {
		xf86DrvMsg(scrn->scrnIndex, X_WARNING,
			   "drmSetMaster failed: %s\n", strerror(errno));
		return FALSE;
	}

	return TRUE;
}

static void
glamor_leave_vt(int scrnIndex, int flags)
{
	ScrnInfoPtr scrn = xf86Screens[scrnIndex];
	struct glamor_screen_private *glamor = glamor_get_screen_private(scrn);

	drmDropMaster(glamor->fd);
}

static Bool
glamor_create_screen_resources(ScreenPtr screen)
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
glamor_close_screen(int scrnIndex, ScreenPtr screen)
{
	ScrnInfoPtr scrn = xf86Screens[scrnIndex];
	struct glamor_screen_private *glamor = glamor_get_screen_private(scrn);

	if (scrn->vtSema == TRUE)
		glamor_leave_vt(scrnIndex, 0);

	glamor_fini(screen);

	eglMakeCurrent(glamor->display,
		       EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglTerminate(glamor->display);

	drmmode_closefb(scrn);

	screen->CloseScreen = glamor->CloseScreen;
	(*screen->CloseScreen) (scrnIndex, screen);

	return TRUE;
}

static Bool
glamor_screen_init(int scrnIndex, ScreenPtr screen, int argc, char **argv)
{
	ScrnInfoPtr scrn = xf86Screens[screen->myNum];
	struct glamor_screen_private *glamor = glamor_get_screen_private(scrn);
	EGLDisplayTypeDRMMESA display;
	const char *version;

	display.type = EGL_DISPLAY_TYPE_DRM_MESA;
	display.device = NULL;
	display.fd = glamor->fd;
	
	glamor->display = eglGetDisplay((EGLNativeDisplayType) &display);
	
	if (!eglInitialize(glamor->display, &glamor->major, &glamor->minor)) {
		xf86DrvMsg(scrn->scrnIndex, X_ERROR,
			   "eglInitialize() failed\n");
		return FALSE;
	}

	version = eglQueryString(glamor->display, EGL_VERSION);
	xf86Msg(X_INFO, "%s: EGL version %s:", glamor_name, version);

	glamor->context = eglCreateContext(glamor->display,
					   NULL, EGL_NO_CONTEXT, NULL);
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

	fbPictureInit(screen, NULL, 0);

	xf86SetBlackWhitePixels(screen);

	if (!glamor_init(screen)) {
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
				HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_64 |
				HARDWARE_CURSOR_UPDATE_UNHIDDEN |
				HARDWARE_CURSOR_ARGB))) {
		xf86DrvMsg(scrn->scrnIndex, X_ERROR,
			   "Hardware cursor initialization failed\n");
	}

	/* Must force it before EnterVT, so we are in control of VT and
	 * later memory should be bound when allocating, e.g rotate_mem */
	scrn->vtSema = TRUE;

	if (!glamor_enter_vt(scrnIndex, 0))
		return FALSE;

	screen->SaveScreen = xf86SaveScreen;
	glamor->CreateScreenResources = screen->CreateScreenResources;
	screen->CreateScreenResources = glamor_create_screen_resources;
	glamor->CloseScreen = screen->CloseScreen;
	screen->CloseScreen = glamor_close_screen;

	return TRUE;
}

static void
glamor_free_screen(int scrnIndex, int flags)
{
	ScrnInfoPtr scrn = xf86Screens[scrnIndex];
	struct glamor_screen_private *glamor = glamor_get_screen_private(scrn);

	close(glamor->fd);
	xfree(scrn->driverPrivate);
	scrn->driverPrivate = NULL;
}

static ModeStatus
glamor_valid_mode(int scrnIndex, DisplayModePtr mode, Bool verbose, int flags)
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

	scrn->PreInit = glamor_pre_init;
	scrn->ScreenInit = glamor_screen_init;
	scrn->AdjustFrame = glamor_adjust_frame;
	scrn->EnterVT = glamor_enter_vt;
	scrn->LeaveVT = glamor_leave_vt;
	scrn->FreeScreen = glamor_free_screen;
	scrn->ValidMode = glamor_valid_mode;

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
