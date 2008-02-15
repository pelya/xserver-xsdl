/*
 * Copyright © 2007 Red Hat, Inc
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of Red Hat,
 * Inc not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  Red Hat, Inc makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * RED HAT, INC DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL RED HAT, INC BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <dlfcn.h>

#include <drm.h>
#include <GL/gl.h>
#include <GL/internal/dri_interface.h>

#include <windowstr.h>
#include <os.h>

#define _XF86DRI_SERVER_
#include <xf86drm.h>
#include <xf86dristr.h>
#include <xf86str.h>
#include <xf86.h>
#include <dri2.h>

#include "glxserver.h"
#include "glxutil.h"
#include "glcontextmodes.h"

#include "g_disptab.h"
#include "glapitable.h"
#include "glapi.h"
#include "glthread.h"
#include "dispatch.h"
#include "extension_string.h"

#define containerOf(ptr, type, member)			\
    (type *)( (char *)ptr - offsetof(type,member) )

typedef struct __GLXDRIscreen   __GLXDRIscreen;
typedef struct __GLXDRIcontext  __GLXDRIcontext;
typedef struct __GLXDRIdrawable __GLXDRIdrawable;

struct __GLXDRIscreen {
    __GLXscreen		 base;
    __DRIscreen		 driScreen;
    void		*driver;
    int			 fd;

    xf86EnterVTProc	*enterVT;
    xf86LeaveVTProc	*leaveVT;

    __DRIcopySubBufferExtension *copySubBuffer;
    __DRIswapControlExtension *swapControl;
    __DRItexBufferExtension *texBuffer;

    unsigned char glx_enable_bits[__GLX_EXT_BYTES];
};

struct __GLXDRIcontext {
    __GLXcontext base;
    __DRIcontext driContext;
    drm_context_t hwContext;
};

struct __GLXDRIdrawable {
    __GLXdrawable base;
    __DRIdrawable driDrawable;
};

static const char CREATE_NEW_SCREEN_FUNC[] = __DRI2_CREATE_NEW_SCREEN_STRING;

static void
__glXDRIdrawableDestroy(__GLXdrawable *drawable)
{
    __GLXDRIdrawable *private = (__GLXDRIdrawable *) drawable;

    (*private->driDrawable.destroyDrawable)(&private->driDrawable);

    /* If the X window was destroyed, the dri DestroyWindow hook will
     * aready have taken care of this, so only call if pDraw isn't NULL. */
    if (drawable->pDraw != NULL)
	DRI2DestroyDrawable(drawable->pDraw->pScreen, drawable->pDraw);

    xfree(private);
}

static GLboolean
__glXDRIdrawableResize(__GLXdrawable *glxPriv)
{
    /* Nothing to do here, the DRI driver asks the server for drawable
     * geometry when it sess the SAREA timestamps change.*/

    return GL_TRUE;
}

static GLboolean
__glXDRIdrawableSwapBuffers(__GLXdrawable *basePrivate)
{
    __GLXDRIdrawable *private = (__GLXDRIdrawable *) basePrivate;

    (*private->driDrawable.swapBuffers)(&private->driDrawable);

    return TRUE;
}


static int
__glXDRIdrawableSwapInterval(__GLXdrawable *baseDrawable, int interval)
{
    __GLXDRIdrawable *draw = (__GLXDRIdrawable *) baseDrawable;
    __GLXDRIscreen *screen = (__GLXDRIscreen *)
	glxGetScreen(baseDrawable->pDraw->pScreen);

    if (screen->swapControl)
	screen->swapControl->setSwapInterval(&draw->driDrawable, interval);

    return 0;
}


static void
__glXDRIdrawableCopySubBuffer(__GLXdrawable *basePrivate,
			       int x, int y, int w, int h)
{
    __GLXDRIdrawable *private = (__GLXDRIdrawable *) basePrivate;
    __GLXDRIscreen *screen = (__GLXDRIscreen *)
	glxGetScreen(basePrivate->pDraw->pScreen);

    if (screen->copySubBuffer)
	screen->copySubBuffer->copySubBuffer(&private->driDrawable,
					     x, y, w, h);
}

static void
__glXDRIcontextDestroy(__GLXcontext *baseContext)
{
    __GLXDRIcontext *context = (__GLXDRIcontext *) baseContext;
    __GLXDRIscreen *screen = (__GLXDRIscreen *) baseContext->pGlxScreen;

    context->driContext.destroyContext(&context->driContext);
    drmDestroyContext(screen->fd, context->hwContext);
    __glXContextDestroy(&context->base);
    xfree(context);
}

static int
__glXDRIcontextMakeCurrent(__GLXcontext *baseContext)
{
    __GLXDRIcontext *context = (__GLXDRIcontext *) baseContext;
    __GLXDRIdrawable *draw = (__GLXDRIdrawable *) baseContext->drawPriv;
    __GLXDRIdrawable *read = (__GLXDRIdrawable *) baseContext->readPriv;

    return (*context->driContext.bindContext)(&context->driContext,
					      &draw->driDrawable,
					      &read->driDrawable);
}					      

static int
__glXDRIcontextLoseCurrent(__GLXcontext *baseContext)
{
    __GLXDRIcontext *context = (__GLXDRIcontext *) baseContext;

    return (*context->driContext.unbindContext)(&context->driContext);
}

static int
__glXDRIcontextCopy(__GLXcontext *baseDst, __GLXcontext *baseSrc,
		    unsigned long mask)
{
    __GLXDRIcontext *dst = (__GLXDRIcontext *) baseDst;
    __GLXDRIcontext *src = (__GLXDRIcontext *) baseSrc;

    /* FIXME: We will need to add DRIcontext::copyContext for this. */

    (void) dst;
    (void) src;

    return FALSE;
}

static int
__glXDRIcontextForceCurrent(__GLXcontext *baseContext)
{
    __GLXDRIcontext *context = (__GLXDRIcontext *) baseContext;
    __GLXDRIdrawable *draw = (__GLXDRIdrawable *) baseContext->drawPriv;
    __GLXDRIdrawable *read = (__GLXDRIdrawable *) baseContext->readPriv;

    return (*context->driContext.bindContext)(&context->driContext,
					      &draw->driDrawable,
					      &read->driDrawable);
}

#ifdef __DRI_TEX_BUFFER

#define isPowerOfTwo(n) (((n) & ((n) - 1 )) == 0)

static int
__glXDRIbindTexImage(__GLXcontext *baseContext,
		     int buffer,
		     __GLXdrawable *glxPixmap)
{
    ScreenPtr pScreen = glxPixmap->pDraw->pScreen;
    __GLXDRIscreen * const screen = (__GLXDRIscreen *) glxGetScreen(pScreen);
    PixmapPtr pixmap;
    __GLXDRIcontext *context = (__GLXDRIcontext *) baseContext;
    unsigned int flags;
    int w, h, target;

    if (screen->texBuffer == NULL)
        return Success;

    pixmap = (PixmapPtr) glxPixmap->pDraw;
    w = pixmap->drawable.width;
    h = pixmap->drawable.height;

    if (!isPowerOfTwo(w) || !isPowerOfTwo(h))
	target = GL_TEXTURE_RECTANGLE_ARB;
    else
	target = GL_TEXTURE_2D;

    screen->texBuffer->setTexBuffer(&context->driContext,
				    target,
				    DRI2GetPixmapHandle(pixmap, &flags),
				    pixmap->drawable.depth,
				    pixmap->devKind,
				    h);

    return Success;
}

static int
__glXDRIreleaseTexImage(__GLXcontext *baseContext,
			int buffer,
			__GLXdrawable *pixmap)
{
    /* FIXME: Just unbind the texture? */
    return Success;
}

#else

static int
__glXDRIbindTexImage(__GLXcontext *baseContext,
		     int buffer,
		     __GLXdrawable *glxPixmap)
{
    return Success;
}

static int
__glXDRIreleaseTexImage(__GLXcontext *baseContext,
			int buffer,
			__GLXdrawable *pixmap)
{
    return Success;
}

#endif

static __GLXtextureFromPixmap __glXDRItextureFromPixmap = {
    __glXDRIbindTexImage,
    __glXDRIreleaseTexImage
};

static void
__glXDRIscreenDestroy(__GLXscreen *baseScreen)
{
    __GLXDRIscreen *screen = (__GLXDRIscreen *) baseScreen;

    screen->driScreen.destroyScreen(&screen->driScreen);

    dlclose(screen->driver);

    __glXScreenDestroy(baseScreen);

    xfree(screen);
}

static __GLXcontext *
__glXDRIscreenCreateContext(__GLXscreen *baseScreen,
			    __GLcontextModes *modes,
			    __GLXcontext *baseShareContext)
{
    __GLXDRIscreen *screen = (__GLXDRIscreen *) baseScreen;
    __GLXDRIcontext *context, *shareContext;
    __DRIcontext *driShare;

    shareContext = (__GLXDRIcontext *) baseShareContext;
    if (shareContext)
	driShare = &shareContext->driContext;
    else
	driShare = NULL;

    context = xalloc(sizeof *context);
    if (context == NULL)
	return NULL;

    memset(context, 0, sizeof *context);
    context->base.destroy           = __glXDRIcontextDestroy;
    context->base.makeCurrent       = __glXDRIcontextMakeCurrent;
    context->base.loseCurrent       = __glXDRIcontextLoseCurrent;
    context->base.copy              = __glXDRIcontextCopy;
    context->base.forceCurrent      = __glXDRIcontextForceCurrent;
    context->base.textureFromPixmap = &__glXDRItextureFromPixmap;

    if (drmCreateContext(screen->fd, &context->hwContext))
	    return GL_FALSE;

    context->driContext.private =
	screen->driScreen.createNewContext(&screen->driScreen,
					   modes,
					   0, /* render type */
					   driShare,
					   context->hwContext,
					   &context->driContext);

    return &context->base;
}

static __GLXdrawable *
__glXDRIscreenCreateDrawable(__GLXscreen *screen,
			     DrawablePtr pDraw,
			     int type,
			     XID drawId,
			     __GLcontextModes *modes)
{
    __GLXDRIscreen *driScreen = (__GLXDRIscreen *) screen;
    __GLXDRIdrawable *private;
    GLboolean retval;
    drm_drawable_t hwDrawable;

    private = xalloc(sizeof *private);
    if (private == NULL)
	return NULL;

    memset(private, 0, sizeof *private);

    if (!__glXDrawableInit(&private->base, screen,
			   pDraw, type, drawId, modes)) {
        xfree(private);
	return NULL;
    }

    private->base.destroy       = __glXDRIdrawableDestroy;
    private->base.resize        = __glXDRIdrawableResize;
    private->base.swapBuffers   = __glXDRIdrawableSwapBuffers;
    private->base.copySubBuffer = __glXDRIdrawableCopySubBuffer;

    retval = DRI2CreateDrawable(screen->pScreen, pDraw, &hwDrawable);

    private->driDrawable.private =
	(driScreen->driScreen.createNewDrawable)(&driScreen->driScreen,
						 modes,
						 &private->driDrawable,
						 hwDrawable, 0, NULL);

    return &private->base;
}

static int
getUST(int64_t *ust)
{
    struct timeval  tv;
    
    if (ust == NULL)
	return -EFAULT;

    if (gettimeofday(&tv, NULL) == 0) {
	ust[0] = (tv.tv_sec * 1000000) + tv.tv_usec;
	return 0;
    } else {
	return -errno;
    }
}

static void __glXReportDamage(__DRIdrawable *driDraw,
			      int x, int y,
			      drm_clip_rect_t *rects, int num_rects,
			      GLboolean front_buffer)
{
    __GLXDRIdrawable *drawable =
	    containerOf(driDraw, __GLXDRIdrawable, driDrawable);
    DrawablePtr pDraw = drawable->base.pDraw;
    RegionRec region;

    REGION_INIT(pDraw->pScreen, &region, (BoxPtr) rects, num_rects);
    REGION_TRANSLATE(pScreen, &region, pDraw->x, pDraw->y);
    DamageDamageRegion(pDraw, &region);
    REGION_UNINIT(pDraw->pScreen, &region);
}

/* Table of functions that we export to the driver. */
static const __DRIinterfaceMethods interface_methods = {
    _gl_context_modes_create,
    _gl_context_modes_destroy,

    NULL,

    getUST,
    NULL,

    __glXReportDamage,
};

static const char dri_driver_path[] = DRI_DRIVER_PATH;

static Bool
glxDRIEnterVT (int index, int flags)
{
    __GLXDRIscreen *screen = (__GLXDRIscreen *) 
	glxGetScreen(screenInfo.screens[index]);

    LogMessage(X_INFO, "AIGLX: Resuming AIGLX clients after VT switch\n");

    if (!(*screen->enterVT) (index, flags))
	return FALSE;
    
    glxResumeClients();

    return TRUE;
}

static void
glxDRILeaveVT (int index, int flags)
{
    __GLXDRIscreen *screen = (__GLXDRIscreen *)
	glxGetScreen(screenInfo.screens[index]);

    LogMessage(X_INFO, "AIGLX: Suspending AIGLX clients for VT switch\n");

    glxSuspendClients();

    return (*screen->leaveVT) (index, flags);
}

static void
initializeExtensions(__GLXDRIscreen *screen)
{
    const __DRIextension **extensions;
    int i;

    extensions = screen->driScreen.getExtensions(&screen->driScreen);
    for (i = 0; extensions[i]; i++) {
#ifdef __DRI_COPY_SUB_BUFFER
	if (strcmp(extensions[i]->name, __DRI_COPY_SUB_BUFFER) == 0) {
	    screen->copySubBuffer = (__DRIcopySubBufferExtension *) extensions[i];
	    __glXEnableExtension(screen->glx_enable_bits,
				 "GLX_MESA_copy_sub_buffer");
	    
	    LogMessage(X_INFO, "AIGLX: enabled GLX_MESA_copy_sub_buffer\n");
	}
#endif

#ifdef __DRI_SWAP_CONTROL
	if (strcmp(extensions[i]->name, __DRI_SWAP_CONTROL) == 0) {
	    screen->swapControl = (__DRIswapControlExtension *) extensions[i];
	    __glXEnableExtension(screen->glx_enable_bits,
				 "GLX_SGI_swap_control");
	    __glXEnableExtension(screen->glx_enable_bits,
				 "GLX_MESA_swap_control");
	    
	    LogMessage(X_INFO, "AIGLX: enabled GLX_SGI_swap_control and GLX_MESA_swap_control\n");
	}
#endif

#ifdef __DRI_TEX_BUFFER
	if (strcmp(extensions[i]->name, __DRI_TEX_BUFFER) == 0) {
	    screen->texBuffer = (__DRItexBufferExtension *) extensions[i];
	    /* GLX_EXT_texture_from_pixmap is always enabled. */
	    LogMessage(X_INFO, "AIGLX: GLX_EXT_texture_from_pixmap backed by buffer objects\n");
	}
#endif
	/* Ignore unknown extensions */
    }
}
    
static __GLXscreen *
__glXDRIscreenProbe(ScreenPtr pScreen)
{
    __DRI2_CREATE_NEW_SCREEN_FUNC *createNewScreen;
    __DRIversion   ddx_version;
    __DRIversion   dri_version;
    __DRIversion   drm_version;
    drmVersionPtr version;
    const char *driverName;
    __GLXDRIscreen *screen;
    char filename[128];
    size_t buffer_size;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    unsigned int sareaHandle;

    screen = xalloc(sizeof *screen);
    if (screen == NULL)
      return NULL;
    memset(screen, 0, sizeof *screen);

    if (!xf86LoaderCheckSymbol("DRI2Connect") ||
	!DRI2Connect(pScreen,
		     &screen->fd,
		     &driverName,
		     &ddx_version.major,
		     &ddx_version.minor,
		     &ddx_version.patch,
		     &sareaHandle)) {
	LogMessage(X_INFO,
		   "AIGLX: Screen %d is not DRI2 capable\n", pScreen->myNum);
	return NULL;
    }

    screen->base.destroy        = __glXDRIscreenDestroy;
    screen->base.createContext  = __glXDRIscreenCreateContext;
    screen->base.createDrawable = __glXDRIscreenCreateDrawable;
    screen->base.swapInterval   = __glXDRIdrawableSwapInterval;
    screen->base.pScreen       = pScreen;

    __glXInitExtensionEnableBits(screen->glx_enable_bits);

    /* DRI protocol version. */
    dri_version.major = XF86DRI_MAJOR_VERSION;
    dri_version.minor = XF86DRI_MINOR_VERSION;
    dri_version.patch = XF86DRI_PATCH_VERSION;

    version = drmGetVersion(screen->fd);
    if (version) {
	drm_version.major = version->version_major;
	drm_version.minor = version->version_minor;
	drm_version.patch = version->version_patchlevel;
	drmFreeVersion(version);
    }
    else {
	drm_version.major = -1;
	drm_version.minor = -1;
	drm_version.patch = -1;
    }

    snprintf(filename, sizeof filename, "%s/%s_dri.so",
             dri_driver_path, driverName);

    screen->driver = dlopen(filename, RTLD_LAZY | RTLD_LOCAL);
    if (screen->driver == NULL) {
	LogMessage(X_ERROR, "AIGLX error: dlopen of %s failed (%s)\n",
		   filename, dlerror());
        goto handle_error;
    }

    createNewScreen = dlsym(screen->driver, CREATE_NEW_SCREEN_FUNC);
    if (createNewScreen == NULL) {
	LogMessage(X_ERROR, "AIGLX error: dlsym for %s failed (%s)\n",
		   CREATE_NEW_SCREEN_FUNC, dlerror());
      goto handle_error;
    }
    
    screen->driScreen.private =
	(*createNewScreen)(pScreen->myNum,
			   &screen->driScreen,
			   &ddx_version,
			   &dri_version,
			   &drm_version,
			   screen->fd,
			   sareaHandle,
			   &interface_methods,
			   &screen->base.fbconfigs);

    if (screen->driScreen.private == NULL) {
	LogMessage(X_ERROR, "AIGLX error: Calling driver entry point failed");
	goto handle_error;
    }

    initializeExtensions(screen);

    __glXScreenInit(&screen->base, pScreen);

    buffer_size = __glXGetExtensionString(screen->glx_enable_bits, NULL);
    if (buffer_size > 0) {
	if (screen->base.GLXextensions != NULL) {
	    xfree(screen->base.GLXextensions);
	}

	screen->base.GLXextensions = xnfalloc(buffer_size);
	(void) __glXGetExtensionString(screen->glx_enable_bits, 
				       screen->base.GLXextensions);
    }

    screen->enterVT = pScrn->EnterVT;
    pScrn->EnterVT = glxDRIEnterVT; 
    screen->leaveVT = pScrn->LeaveVT;
    pScrn->LeaveVT = glxDRILeaveVT;

    LogMessage(X_INFO,
	       "AIGLX: Loaded and initialized %s\n", filename);

    return &screen->base;

 handle_error:
    if (screen->driver)
        dlclose(screen->driver);

    xfree(screen);

    LogMessage(X_ERROR, "AIGLX: reverting to software rendering\n");

    return NULL;
}

__GLXprovider __glXDRI2Provider = {
    __glXDRIscreenProbe,
    "DRI2",
    NULL
};
