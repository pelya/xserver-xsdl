/*
 * Copyright © 2006 Red Hat, Inc
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
#include <drm_sarea.h>
#include <xf86drm.h>
#include <xf86dristr.h>
#include <xf86str.h>
#include <xf86.h>
#include <dri.h>

#define DRI_NEW_INTERFACE_ONLY
#include "glxserver.h"
#include "glxutil.h"
#include "glcontextmodes.h"

#include "g_disptab.h"
#include "glapitable.h"
#include "glapi.h"
#include "glthread.h"
#include "dispatch.h"
#include "extension_string.h"


#define STRINGIFY(macro_or_string)	STRINGIFY_ARG (macro_or_string)
#define	STRINGIFY_ARG(contents)	#contents

typedef struct __GLXDRIscreen      __GLXDRIscreen;
typedef struct __GLXDRIcontext         __GLXDRIcontext;
typedef struct __GLXDRIdrawable __GLXDRIdrawable;

struct __GLXDRIscreen {
    __GLXscreen		 base;

    __DRIscreen			 driScreen;
    void			*driver;

    xf86EnterVTProc *enterVT;
    xf86LeaveVTProc *leaveVT;

    unsigned char glx_enable_bits[__GLX_EXT_BYTES];
};

struct __GLXDRIcontext {
    __GLXcontext		 base;

    __DRIcontext		 driContext;
};

struct __GLXDRIdrawable {
    __GLXdrawable	 base;

    __DRIdrawable		*driDrawable;
};

/* History:
 * 20021121 - Initial version
 * 20021128 - Added __glXWindowExists() function
 * 20021207 - Added support for dynamic GLX extensions,
 *            GLX_SGI_swap_control, GLX_SGI_video_sync,
 *            GLX_OML_sync_control, and GLX_MESA_swap_control.
 *            Never officially released.  Do NOT test against
 *            this version.  Use 20030317 instead.
 * 20030317 - Added support GLX_SGIX_fbconfig,
 *            GLX_MESA_swap_frame_usage, GLX_OML_swap_method,
 *            GLX_{ARB,SGIS}_multisample, and
 *            GLX_SGIX_visual_select_group.
 * 20030606 - Added support for GLX_SGI_make_current_read.
 * 20030813 - Made support for dynamic extensions multi-head aware.
 * 20030818 - Added support for GLX_MESA_allocate_memory in place of the
 *            deprecated GLX_NV_vertex_array_range & GLX_MESA_agp_offset
 *            interfaces.
 * 20031201 - Added support for the first round of DRI interface changes.
 *            Do NOT test against this version!  It has binary
 *            compatibility bugs, use 20040317 instead.
 * 20040317 - Added the 'mode' field to __DRIcontextRec.
 * 20040415 - Added support for bindContext3 and unbindContext3.
 * 20040602 - Add __glXGetDrawableInfo.  I though that was there
 *            months ago. :(
 * 20050727 - Gut all the old interfaces.  This breaks compatability with
 *            any DRI driver built to any previous version.
 * 20060314 - Added support for GLX_MESA_copy_sub_buffer.
 */

#define INTERNAL_VERSION 20050727

static const char CREATE_NEW_SCREEN_FUNC[] =
    "__driCreateNewScreen_" STRINGIFY (INTERNAL_VERSION);

/* The DRI driver entry point version wasn't bumped when the
 * copySubBuffer functionality was added to the DRI drivers, but the
 * functionality is still conditional on the value of the
 * internal_api_version passed to __driCreateNewScreen.  However, the
 * screen constructor doesn't fail for a DRI driver that's older than
 * the passed in version number, so there's no way we can know for
 * sure that we can actually use the copySubBuffer functionality.  But
 * since the earliest (and at this point only) released mesa version
 * (6.5) that uses the 20050727 entry point does have copySubBuffer,
 * we'll just settle for that.  We still have to pass in a higher to
 * the screen constructor to enable the functionality.
 */
#define COPY_SUB_BUFFER_INTERNAL_VERSION 20060314

static void
__glXDRIleaveServer(void)
{
  DRIBlockHandler(NULL, NULL, NULL);
}
    
static void
__glXDRIenterServer(void)
{
  DRIWakeupHandler(NULL, 0, NULL);
}

/**
 * \bug
 * We're jumping through hoops here to get the DRIdrawable which the DRI
 * driver tries to keep to it self...  cf. FIXME in \c createDrawable.
 */
static void
__glXDRIdrawableFoo(__GLXDRIdrawable *draw)
{
    __GLXDRIscreen * const screen =
      (__GLXDRIscreen *) __glXgetActiveScreen(draw->base.pDraw->pScreen->myNum);

    draw->driDrawable = (*screen->driScreen.getDrawable)(NULL,
							 draw->base.drawId,
							 screen->driScreen.private);
}

static void
__glXDRIdrawableDestroy(__GLXdrawable *private)
{
#if 0
    (*glxPriv->driDrawable.destroyDrawable)(NULL,
					    glxPriv->driDrawable.private);
#endif

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

    __glXDRIdrawableFoo(private);

    (*private->driDrawable->swapBuffers)(NULL,
					 private->driDrawable->private);

    return TRUE;
}


static int
__glXDRIdrawableSwapInterval(__GLXdrawable *baseDrawable, int interval)
{
    __GLXDRIdrawable *draw = (__GLXDRIdrawable *) baseDrawable;

    __glXDRIdrawableFoo(draw);

    draw->driDrawable->swap_interval = interval;
    return 0;
}


static void
__glXDRIdrawableCopySubBuffer(__GLXdrawable *basePrivate,
			       int x, int y, int w, int h)
{
    __GLXDRIdrawable *private = (__GLXDRIdrawable *) basePrivate;

    __glXDRIdrawableFoo(private);

    (*private->driDrawable->copySubBuffer)(NULL,
					   private->driDrawable->private,
					   x, y, w, h);
}

static void
__glXDRIcontextDestroy(__GLXcontext *baseContext)
{
    __GLXDRIcontext *context = (__GLXDRIcontext *) baseContext;

    context->driContext.destroyContext(NULL,
				       context->base.pScreen->myNum,
				       context->driContext.private);
    __glXContextDestroy(&context->base);
    xfree(context);
}

static int
__glXDRIcontextMakeCurrent(__GLXcontext *baseContext)
{
    __GLXDRIcontext *context = (__GLXDRIcontext *) baseContext;

    return (*context->driContext.bindContext)(NULL,
					      context->base.pScreen->myNum,
					      baseContext->drawPriv->drawId,
					      baseContext->readPriv->drawId,
					      &context->driContext);
}

static int
__glXDRIcontextLoseCurrent(__GLXcontext *baseContext)
{
    __GLXDRIcontext *context = (__GLXDRIcontext *) baseContext;

    return (*context->driContext.unbindContext)(NULL,
						context->base.pScreen->myNum,
						baseContext->drawPriv->drawId,
						baseContext->readPriv->drawId,
						&context->driContext);
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

    return (*context->driContext.bindContext)(NULL,
					      context->base.pScreen->myNum,
					      baseContext->drawPriv->drawId,
					      baseContext->readPriv->drawId,
					      &context->driContext);
}

static int
glxCountBits(int word)
{
    int ret = 0;

    while (word) {
        ret += (word & 1);
        word >>= 1;
    }

    return ret;
}

static void
glxFillAlphaChannel (PixmapPtr pixmap, int x, int y, int width, int height)
{
    int i;
    CARD32 *p, *end, *pixels = (CARD32 *)pixmap->devPrivate.ptr;
    CARD32 rowstride = pixmap->devKind / 4;
    
    for (i = y; i < y + height; i++)
    {
	p = &pixels[i * rowstride + x];
	end = p + width;
	while (p < end)
	  *p++ |= 0xFF000000;
    }
}

/*
 * (sticking this here for lack of a better place)
 * Known issues with the GLX_EXT_texture_from_pixmap implementation:
 * - In general we ignore the fbconfig, lots of examples follow
 * - No fbconfig handling for multiple mipmap levels
 * - No fbconfig handling for 1D textures
 * - No fbconfig handling for TEXTURE_TARGET
 * - No fbconfig exposure of Y inversion state
 * - No GenerateMipmapEXT support (due to no FBO support)
 * - No support for anything but 16bpp and 32bpp-sparse pixmaps
 */

static int
__glXDRIbindTexImage(__GLXcontext *baseContext,
		     int buffer,
		     __GLXpixmap *glxPixmap)
{
    RegionPtr	pRegion;
    PixmapPtr	pixmap;
    int		bpp;
    GLenum	target, format, type;

    pixmap = (PixmapPtr) glxPixmap->pDraw;
    if (!glxPixmap->pDamage) {
        glxPixmap->pDamage = DamageCreate(NULL, NULL, DamageReportNone,
					  TRUE, glxPixmap->pScreen, NULL);
	if (!glxPixmap->pDamage)
            return BadAlloc;

	DamageRegister ((DrawablePtr) pixmap, glxPixmap->pDamage);
	pRegion = NULL;
    } else {
	pRegion = DamageRegion(glxPixmap->pDamage);
	if (REGION_NIL(pRegion))
	    return Success;
    }

    /* XXX 24bpp packed, 8, etc */
    if (pixmap->drawable.depth >= 24) {
	bpp = 4;
	format = GL_BGRA;
	type =
#if X_BYTE_ORDER == X_LITTLE_ENDIAN
	    GL_UNSIGNED_BYTE;
#else
	    GL_UNSIGNED_INT_8_8_8_8_REV;
#endif
    } else {
	bpp = 2;
	format = GL_RGB;
	type = GL_UNSIGNED_SHORT_5_6_5;
    }

    if (!(glxCountBits(pixmap->drawable.width) == 1 &&
	  glxCountBits(pixmap->drawable.height) == 1)
	/* || strstr(CALL_GetString(GL_EXTENSIONS,
	             "GL_ARB_texture_non_power_of_two")) */)
	target = GL_TEXTURE_RECTANGLE_ARB;
    else
	target = GL_TEXTURE_2D;

    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ROW_LENGTH,
				       pixmap->devKind / bpp) );
    if (pRegion == NULL)
    {
	if (pixmap->drawable.depth == 24)
	    glxFillAlphaChannel(pixmap,
				pixmap->drawable.x,
				pixmap->drawable.y,
				pixmap->drawable.width,
				pixmap->drawable.height);

        CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_PIXELS,
					   pixmap->drawable.x) );
	CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_ROWS,
					   pixmap->drawable.y) );

	CALL_TexImage2D( GET_DISPATCH(),
			 (target,
			  0,
			  bpp == 4 ? 4 : 3,
			  pixmap->drawable.width,
			  pixmap->drawable.height,
			  0,
			  format,
			  type,
			  pixmap->devPrivate.ptr) );
    } else {
        int i, numRects;
	BoxPtr p;

	numRects = REGION_NUM_RECTS (pRegion);
	p = REGION_RECTS (pRegion);
	for (i = 0; i < numRects; i++)
	{
	    if (pixmap->drawable.depth == 24)
		glxFillAlphaChannel(pixmap,
				    pixmap->drawable.x + p[i].x1,
				    pixmap->drawable.y + p[i].y1,
				    p[i].x2 - p[i].x1,
				    p[i].y2 - p[i].y1);

	    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_PIXELS,
					       pixmap->drawable.x + p[i].x1) );
	    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_ROWS,
					       pixmap->drawable.y + p[i].y1) );

	    CALL_TexSubImage2D( GET_DISPATCH(),
				(target,
				 0,
				 p[i].x1, p[i].y1,
				 p[i].x2 - p[i].x1, p[i].y2 - p[i].y1,
				 format,
				 type,
				 pixmap->devPrivate.ptr) );
	}
    }

    DamageEmpty(glxPixmap->pDamage);

    return Success;
}

static int
__glXDRIreleaseTexImage(__GLXcontext *baseContext,
			int buffer,
			__GLXpixmap *pixmap)
{
    return Success;
}

static __GLXtextureFromPixmap __glXDRItextureFromPixmap = {
    __glXDRIbindTexImage,
    __glXDRIreleaseTexImage
};

static void
__glXDRIscreenDestroy(__GLXscreen *baseScreen)
{
    __GLXDRIscreen *screen = (__GLXDRIscreen *) baseScreen;

    screen->driScreen.destroyScreen(NULL,
				    screen->base.pScreen->myNum,
				    screen->driScreen.private);

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
    void *sharePrivate;

    shareContext = (__GLXDRIcontext *) baseShareContext;
    if (shareContext)
	sharePrivate = shareContext->driContext.private;
    else
	sharePrivate = NULL;

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

    context->driContext.private =
	screen->driScreen.createNewContext(NULL, modes,
					   0, /* render type */
					   sharePrivate,
					   &context->driContext);

    context->driContext.mode = modes;

    return &context->base;
}

static __GLXdrawable *
__glXDRIscreenCreateDrawable(__GLXscreen *screen,
			     DrawablePtr pDraw,
			     XID drawId,
			     __GLcontextModes *modes)
{
    __GLXDRIdrawable *private;

    private = xalloc(sizeof *private);
    if (private == NULL)
	return NULL;

    memset(private, 0, sizeof *private);

    if (!__glXDrawableInit(&private->base, screen, pDraw, drawId, modes)) {
        xfree(private);
	return NULL;
    }

    private->base.destroy       = __glXDRIdrawableDestroy;
    private->base.resize        = __glXDRIdrawableResize;
    private->base.swapBuffers   = __glXDRIdrawableSwapBuffers;
    private->base.copySubBuffer = __glXDRIdrawableCopySubBuffer;

#if 0
    /* FIXME: It would only be natural that we called
     * driScreen->createNewDrawable here but the DRI drivers manage
     * them a little oddly. FIXME: describe this better.*/

    /* The last argument is 'attrs', which is used with pbuffers which
     * we currently don't support. */

    glxPriv->driDrawable.private =
	(screen->driScreen.createNewDrawable)(NULL, modes,
					      drawId,
					      &glxPriv->driDrawable,
					      0,
					      NULL);
#endif

    return &private->base;
}


static unsigned
filter_modes(__GLcontextModes **server_modes,
	     const __GLcontextModes *driver_modes)
{
    __GLcontextModes * m;
    __GLcontextModes ** prev_next;
    const __GLcontextModes * check;
    unsigned modes_count = 0;

    if ( driver_modes == NULL ) {
	LogMessage(X_WARNING,
		   "AIGLX: 3D driver returned no fbconfigs.\n");
	return 0;
    }

    /* For each mode in server_modes, check to see if a matching mode exists
     * in driver_modes.  If not, then the mode is not available.
     */

    prev_next = server_modes;
    for ( m = *prev_next ; m != NULL ; m = *prev_next ) {
	GLboolean do_delete = GL_TRUE;

	for ( check = driver_modes ; check != NULL ; check = check->next ) {
	    if ( _gl_context_modes_are_same( m, check ) ) {
		do_delete = GL_FALSE;
		break;
	    }
	}

	/* The 3D has to support all the modes that match the GLX visuals
	 * sent from the X server.
	 */
	if ( do_delete && (m->visualID != 0) ) {
	    do_delete = GL_FALSE;

	    LogMessage(X_WARNING,
		       "AIGLX: 3D driver claims to not support "
		       "visual 0x%02x\n", m->visualID);
	}

	if ( do_delete ) {
	    *prev_next = m->next;

	    m->next = NULL;
	    _gl_context_modes_destroy( m );
	}
	else {
	    modes_count++;
	    prev_next = & m->next;
	}
    }

    return modes_count;
}


static void
enable_glx_extension(void *psc, const char *ext_name)
{
    __GLXDRIscreen * const screen = (__GLXDRIscreen *) psc;

    __glXEnableExtension(screen->glx_enable_bits, ext_name);
}


static __DRIfuncPtr getProcAddress(const char *proc_name)
{
    if (strcmp(proc_name, "glxEnableExtension") == 0) {
	return (__DRIfuncPtr) enable_glx_extension;
    }

    return NULL;
}

static __DRIscreen *findScreen(__DRInativeDisplay *dpy, int scrn)
{
    __GLXDRIscreen *screen = (__GLXDRIscreen *) __glXgetActiveScreen(scrn);

    return &screen->driScreen;
}

static GLboolean windowExists(__DRInativeDisplay *dpy, __DRIid draw)
{
    WindowPtr pWin = (WindowPtr) LookupIDByType(draw, RT_WINDOW);

    return pWin == NULL ? GL_FALSE : GL_TRUE;
}

static GLboolean createContext(__DRInativeDisplay *dpy, int screen,
			       int configID, void *contextID,
			       drm_context_t *hw_context)
{
    XID fakeID;
    VisualPtr visual;
    int i;
    ScreenPtr pScreen;
    GLboolean retval;

    pScreen = screenInfo.screens[screen];

    /* Find the requested X visual */
    visual = pScreen->visuals;
    for (i = 0; i < pScreen->numVisuals; i++, visual++)
	if (visual->vid == configID)
	    break;
    if (i == pScreen->numVisuals)
	return GL_FALSE;

    fakeID = FakeClientID(0);
    *(XID *) contextID = fakeID;

    __glXDRIenterServer();
    retval = DRICreateContext(pScreen, visual, fakeID, hw_context);
    __glXDRIleaveServer();
    return retval;
}

static GLboolean destroyContext(__DRInativeDisplay *dpy, int screen,
				__DRIid context)
{
    GLboolean retval;

    __glXDRIenterServer();
    retval = DRIDestroyContext(screenInfo.screens[screen], context);
    __glXDRIleaveServer();
    return retval;
}

static GLboolean
createDrawable(__DRInativeDisplay *dpy, int screen,
	       __DRIid drawable, drm_drawable_t *hHWDrawable)
{
    DrawablePtr pDrawable;
    GLboolean retval;

    pDrawable = (DrawablePtr) LookupIDByClass(drawable, RC_DRAWABLE);
    if (!pDrawable)
	return GL_FALSE;

    __glXDRIenterServer();
    retval = DRICreateDrawable(screenInfo.screens[screen],
			    drawable,
			    pDrawable,
			    hHWDrawable);
    __glXDRIleaveServer();
    return retval;
}

static GLboolean
destroyDrawable(__DRInativeDisplay *dpy, int screen, __DRIid drawable)
{
    DrawablePtr pDrawable;
    GLboolean retval;

    pDrawable = (DrawablePtr) LookupIDByClass(drawable, RC_DRAWABLE);
    if (!pDrawable)
	return GL_FALSE;

    __glXDRIenterServer();
    retval = DRIDestroyDrawable(screenInfo.screens[screen],
			     drawable,
			     pDrawable);
    __glXDRIleaveServer();
    return retval;
}

static GLboolean
getDrawableInfo(__DRInativeDisplay *dpy, int screen,
		__DRIid drawable, unsigned int *index, unsigned int *stamp,
		int *x, int *y, int *width, int *height,
		int *numClipRects, drm_clip_rect_t **ppClipRects,
		int *backX, int *backY,
		int *numBackClipRects, drm_clip_rect_t **ppBackClipRects)
{
    DrawablePtr pDrawable;
    drm_clip_rect_t *pClipRects, *pBackClipRects;
    GLboolean retval;
    size_t size;

    pDrawable = (DrawablePtr) LookupIDByClass(drawable, RC_DRAWABLE);
    if (!pDrawable) {
	ErrorF("getDrawableInfo failed to look up window\n");

        *index = 0;
	*stamp = 0;
        *x = 0;
        *y = 0;
	*width = 0;
	*height = 0;
	*numClipRects = 0;
	*ppClipRects = NULL;
	*backX = 0;
	*backY = 0;
	*numBackClipRects = 0;
	*ppBackClipRects = NULL;

	return GL_FALSE;
    }

    __glXDRIenterServer();
    retval = DRIGetDrawableInfo(screenInfo.screens[screen],
				pDrawable, index, stamp,
				x, y, width, height,
				numClipRects, &pClipRects,
				backX, backY,
				numBackClipRects, &pBackClipRects);
    __glXDRIleaveServer();

    if (*numClipRects > 0) {
	size = sizeof (drm_clip_rect_t) * *numClipRects;
	*ppClipRects = xalloc (size);
	if (*ppClipRects != NULL)
	    memcpy (*ppClipRects, pClipRects, size);
    }
    else {
      *ppClipRects = NULL;
    }
      
    if (*numBackClipRects > 0) {
	size = sizeof (drm_clip_rect_t) * *numBackClipRects;
	*ppBackClipRects = xalloc (size);
	if (*ppBackClipRects != NULL)
	    memcpy (*ppBackClipRects, pBackClipRects, size);
    }
    else {
      *ppBackClipRects = NULL;
    }

    return GL_TRUE;
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

/* Table of functions that we export to the driver. */
static const __DRIinterfaceMethods interface_methods = {
    getProcAddress,

    _gl_context_modes_create,
    _gl_context_modes_destroy,
      
    findScreen,
    windowExists,
      
    createContext,
    destroyContext,

    createDrawable,
    destroyDrawable,
    getDrawableInfo,

    getUST,
    NULL, /* glXGetMscRateOML, */
};

static const char dri_driver_path[] = DRI_DRIVER_PATH;

static Bool
glxDRIEnterVT (int index, int flags)
{
    __GLXDRIscreen *screen = (__GLXDRIscreen *) __glXgetActiveScreen(index);

    LogMessage(X_INFO, "AIGLX: Resuming AIGLX clients after VT switch\n");

    glxResumeClients();

    return (*screen->enterVT) (index, flags);
}

static void
glxDRILeaveVT (int index, int flags)
{
    __GLXDRIscreen *screen = (__GLXDRIscreen *) __glXgetActiveScreen(index);

    LogMessage(X_INFO, "AIGLX: Suspending AIGLX clients for VT switch\n");

    glxSuspendClients();

    return (*screen->leaveVT) (index, flags);
}

static __GLXscreen *
__glXDRIscreenProbe(ScreenPtr pScreen)
{
    PFNCREATENEWSCREENFUNC createNewScreen;
    drm_handle_t hSAREA;
    drmAddress pSAREA = NULL;
    char *BusID;
    __DRIversion   ddx_version;
    __DRIversion   dri_version;
    __DRIversion   drm_version;
    __DRIframebuffer  framebuffer;
    int   fd = -1;
    int   status;
    int api_ver = COPY_SUB_BUFFER_INTERNAL_VERSION;
    drm_magic_t magic;
    drmVersionPtr version;
    char *driverName;
    drm_handle_t  hFB;
    int        junk;
    __GLcontextModes * driver_modes;
    __GLXDRIscreen *screen;
    void *dev_priv = NULL;
    char filename[128];
    Bool isCapable;
    size_t buffer_size;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];

    if (!xf86LoaderCheckSymbol("DRIQueryDirectRenderingCapable")) {
	LogMessage(X_ERROR, "AIGLX: DRI module not loaded\n");
	return NULL;
    }

    if (!DRIQueryDirectRenderingCapable(pScreen, &isCapable) || !isCapable) {
	LogMessage(X_ERROR,
		   "AIGLX: Screen %d is not DRI capable\n", pScreen->myNum);
	return NULL;
    }

    screen = xalloc(sizeof *screen);
    if (screen == NULL)
      return NULL;
    memset(screen, 0, sizeof *screen);

    screen->base.destroy        = __glXDRIscreenDestroy;
    screen->base.createContext  = __glXDRIscreenCreateContext;
    screen->base.createDrawable = __glXDRIscreenCreateDrawable;
    screen->base.swapInterval   = __glXDRIdrawableSwapInterval;
    screen->base.pScreen       = pScreen;

    __glXInitExtensionEnableBits(screen->glx_enable_bits);
    screen->driScreen.screenConfigs = screen;


    /* DRI protocol version. */
    dri_version.major = XF86DRI_MAJOR_VERSION;
    dri_version.minor = XF86DRI_MINOR_VERSION;
    dri_version.patch = XF86DRI_PATCH_VERSION;

    framebuffer.base = NULL;
    framebuffer.dev_priv = NULL;

    if (!DRIOpenConnection(pScreen, &hSAREA, &BusID)) {
	LogMessage(X_ERROR, "AIGLX error: DRIOpenConnection failed\n");
	goto handle_error;
    }

    fd = drmOpen(NULL, BusID);

    if (fd < 0) {
	LogMessage(X_ERROR, "AIGLX error: drmOpen failed (%s)\n",
		   strerror(-fd));
	goto handle_error;
    }

    if (drmGetMagic(fd, &magic)) {
	LogMessage(X_ERROR, "AIGLX error: drmGetMagic failed\n");
	goto handle_error;
    }

    version = drmGetVersion(fd);
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

    if (!DRIAuthConnection(pScreen, magic)) {
	LogMessage(X_ERROR, "AIGLX error: DRIAuthConnection failed\n");
	goto handle_error;
    }

    /* Get device name (like "tdfx") and the ddx version numbers.
     * We'll check the version in each DRI driver's "createNewScreen"
     * function. */
    if (!DRIGetClientDriverName(pScreen,
				&ddx_version.major,
				&ddx_version.minor,
				&ddx_version.patch,
				&driverName)) {
	LogMessage(X_ERROR, "AIGLX error: DRIGetClientDriverName failed\n");
	goto handle_error;
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

    /*
     * Get device-specific info.  pDevPriv will point to a struct
     * (such as DRIRADEONRec in xfree86/driver/ati/radeon_dri.h) that
     * has information about the screen size, depth, pitch, ancilliary
     * buffers, DRM mmap handles, etc.
     */
    if (!DRIGetDeviceInfo(pScreen, &hFB, &junk,
			  &framebuffer.size, &framebuffer.stride,
			  &framebuffer.dev_priv_size, &framebuffer.dev_priv)) {
	LogMessage(X_ERROR, "AIGLX error: XF86DRIGetDeviceInfo failed");
	goto handle_error;
    }

    /* Sigh... the DRI interface is broken; the DRI driver will free
     * the dev private pointer using _mesa_free() on screen destroy,
     * but we can't use _mesa_malloc() here.  In fact, the DRI driver
     * shouldn't free data it didn't allocate itself, but what can you
     * do... */
    dev_priv = xalloc(framebuffer.dev_priv_size);
    if (dev_priv == NULL) {
	LogMessage(X_ERROR, "AIGLX error: dev_priv allocation failed");
	goto handle_error;
    }
    memcpy(dev_priv, framebuffer.dev_priv, framebuffer.dev_priv_size);
    framebuffer.dev_priv = dev_priv;

    framebuffer.width = pScreen->width;
    framebuffer.height = pScreen->height;

    /* Map the framebuffer region. */
    status = drmMap(fd, hFB, framebuffer.size, 
		    (drmAddressPtr)&framebuffer.base);
    if (status != 0) {
	LogMessage(X_ERROR, "AIGLX error: drmMap of framebuffer failed (%s)",
		   strerror(-status));
	goto handle_error;
    }

    /* Map the SAREA region.  Further mmap regions may be setup in
     * each DRI driver's "createNewScreen" function.
     */
    status = drmMap(fd, hSAREA, SAREA_MAX, &pSAREA);
    if (status != 0) {
	LogMessage(X_ERROR, "AIGLX error: drmMap of SAREA failed (%s)",
		   strerror(-status));
	goto handle_error;
    }
    
    driver_modes = NULL;
    screen->driScreen.private =
	(*createNewScreen)(NULL, pScreen->myNum,
			   &screen->driScreen,
			   screen->base.modes,
			   &ddx_version,
			   &dri_version,
			   &drm_version,
			   &framebuffer,
			   pSAREA,
			   fd,
			   api_ver,
			   &interface_methods,
			   &driver_modes);

    if (screen->driScreen.private == NULL) {
	LogMessage(X_ERROR, "AIGLX error: Calling driver entry point failed");
	goto handle_error;
    }

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


    filter_modes(&screen->base.modes, driver_modes);
    _gl_context_modes_destroy(driver_modes);

    __glXsetEnterLeaveServerFuncs(__glXDRIenterServer, __glXDRIleaveServer);

    screen->enterVT = pScrn->EnterVT;
    pScrn->EnterVT = glxDRIEnterVT; 
    screen->leaveVT = pScrn->LeaveVT;
    pScrn->LeaveVT = glxDRILeaveVT;

    LogMessage(X_INFO,
	       "AIGLX: Loaded and initialized %s\n", filename);

    return &screen->base;

 handle_error:
    if (pSAREA != NULL)
	drmUnmap(pSAREA, SAREA_MAX);

    if (framebuffer.base != NULL)
	drmUnmap((drmAddress)framebuffer.base, framebuffer.size);

    if (dev_priv != NULL)
	xfree(dev_priv);

    if (fd >= 0)
	drmClose(fd);

    DRICloseConnection(pScreen);

    if (screen->driver)
        dlclose(screen->driver);

    xfree(screen);

    LogMessage(X_ERROR, "AIGLX: reverting to software rendering\n");

    return NULL;
}

__GLXprovider __glXDRIProvider = {
    __glXDRIscreenProbe,
    "DRI",
    NULL
};
