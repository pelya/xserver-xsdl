/*
 * GLX implementation that uses Apple's AGL.framework for OpenGL
 */
/*
 * Copyright (c) 2002 Greg Parker. All Rights Reserved.
 *
 * Portions of this file are copied from xf86glx.c,
 * which contains the following copyright:
 *
 * Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written authorization.
 */
/* $XFree86: xc/programs/Xserver/GL/apple/aglGlx.c,v 1.3 2003/10/13 00:30:54 torrey Exp $ */

#include "quartzCommon.h"
#include <AGL/agl.h>
#include "cr.h"

// X11 and X11's glx
#undef BOOL
#define BOOL xBOOL
#include "quartz.h"
#include <miscstruct.h>
#include <windowstr.h>
#include <resource.h>
#include <GL/glxint.h>
#include <GL/glxtokens.h>
#include <scrnintstr.h>
#include <glxserver.h>
#include <glxscreens.h>
#include <glxdrawable.h>
#include <glxcontext.h>
#include <glxext.h>
#include <glxutil.h>
#include <glxscreens.h>
#include <GL/internal/glcore.h>
#undef BOOL

// Write debugging output, or not
#ifdef GLAQUA_DEBUG
#define GLAQUA_DEBUG_MSG ErrorF
#else
#define GLAQUA_DEBUG_MSG(a, ...)
#endif


// The following GL functions don't have an EXT suffix in OpenGL.framework.
GLboolean glAreTexturesResidentEXT (GLsizei a, const GLuint *b, GLboolean *c) {
    return glAreTexturesResident(a, b, c);
}
void glDeleteTexturesEXT (GLsizei d, const GLuint *e) {
    glDeleteTextures(d, e);
}
void glGenTexturesEXT (GLsizei f, GLuint *g) {
    glGenTextures(f, g);
}
GLboolean glIsTextureEXT (GLuint h) {
    return glIsTexture(h);
}


// some prototypes
static Bool glAquaScreenProbe(int screen);
static Bool glAquaInitVisuals(VisualPtr *visualp, DepthPtr *depthp,
			      int *nvisualp, int *ndepthp,
			      int *rootDepthp, VisualID *defaultVisp,
			      unsigned long sizes, int bitsPerRGB);
static void glAquaSetVisualConfigs(int nconfigs, __GLXvisualConfig *configs,
				   void **privates);
static __GLinterface *glAquaCreateContext(__GLimports *imports,
					  __GLcontextModes *mode,
					  __GLinterface *shareGC);
static void glAquaCreateBuffer(__GLXdrawablePrivate *glxPriv);
static void glAquaResetExtension(void);


/*
 * This structure is statically allocated in the __glXScreens[]
 * structure.  This struct is not used anywhere other than in
 * __glXScreenInit to initialize each of the active screens
 * (__glXActiveScreens[]).  Several of the fields must be initialized by
 * the screenProbe routine before they are copied to the active screens
 * struct.  In particular, the contextCreate, pGlxVisual, numVisuals,
 * and numUsableVisuals fields must be initialized.
 */
__GLXscreenInfo __glDDXScreenInfo = {
    glAquaScreenProbe,   /* Must be generic and handle all screens */
    glAquaCreateContext, /* Substitute screen's createContext routine */
    glAquaCreateBuffer,  /* Substitute screen's createBuffer routine */
    NULL,                 /* Set up pGlxVisual in probe */
    NULL,                 /* Set up pVisualPriv in probe */
    0,                    /* Set up numVisuals in probe */
    0,                    /* Set up numUsableVisuals in probe */
    "Vendor String",      /* GLXvendor is overwritten by __glXScreenInit */
    "Version String",     /* GLXversion is overwritten by __glXScreenInit */
    "Extensions String",  /* GLXextensions is overwritten by __glXScreenInit */
    NULL                  /* WrappedPositionWindow is overwritten */
};

__GLXextensionInfo __glDDXExtensionInfo = {
    GL_CORE_APPLE,
    glAquaResetExtension,
    glAquaInitVisuals,
    glAquaSetVisualConfigs
};


// prototypes

static GLboolean glAquaDestroyContext(__GLcontext *gc);
static GLboolean glAquaLoseCurrent(__GLcontext *gc);
static GLboolean glAquaMakeCurrent(__GLcontext *gc);
static GLboolean glAquaShareContext(__GLcontext *gc, __GLcontext *gcShare);
static GLboolean glAquaCopyContext(__GLcontext *dst, const __GLcontext *src,
			    GLuint mask);
static GLboolean glAquaForceCurrent(__GLcontext *gc);

/* Drawing surface notification callbacks */
static GLboolean glAquaNotifyResize(__GLcontext *gc);
static void glAquaNotifyDestroy(__GLcontext *gc);
static void glAquaNotifySwapBuffers(__GLcontext *gc);

/* Dispatch table override control for external agents like libGLS */
static struct __GLdispatchStateRec* glAquaDispatchExec(__GLcontext *gc);
static void glAquaBeginDispatchOverride(__GLcontext *gc);
static void glAquaEndDispatchOverride(__GLcontext *gc);


static __GLexports glAquaExports = {
    glAquaDestroyContext,
    glAquaLoseCurrent,
    glAquaMakeCurrent,
    glAquaShareContext,
    glAquaCopyContext,
    glAquaForceCurrent,

    glAquaNotifyResize,
    glAquaNotifyDestroy,
    glAquaNotifySwapBuffers,

    glAquaDispatchExec,
    glAquaBeginDispatchOverride,
    glAquaEndDispatchOverride
};


typedef struct {
    int num_vis;
    __GLXvisualConfig *glx_vis;
    void **priv;

    // wrapped screen functions
    RealizeWindowProcPtr RealizeWindow;
    UnrealizeWindowProcPtr UnrealizeWindow;
} glAquaScreenRec;

static glAquaScreenRec glAquaScreens[MAXSCREENS];


// __GLdrawablePrivate->private
typedef struct {
    GLboolean (*resize)(__GLdrawableBuffer *buf, GLint x, GLint y,
                        GLuint width, GLuint height,
                        __GLdrawablePrivate *glPriv,
                        GLuint bufferMask);
} GLAquaDrawableRec;

struct __GLcontextRec {
  struct __GLinterfaceRec interface; // required to be first

  AGLContext ctx;
  AGLPixelFormat pixelFormat;
  Bool isAttached; // TRUE if ctx is really attached to a window
};


// Context manipulation; return GL_FALSE on failure
static GLboolean glAquaDestroyContext(__GLcontext *gc)
{
    GLAQUA_DEBUG_MSG("glAquaDestroyContext (ctx 0x%x)\n", gc->ctx);

    if (gc) {
        if (gc->ctx) aglDestroyContext(gc->ctx);
        if (gc->pixelFormat) aglDestroyPixelFormat(gc->pixelFormat);
        free(gc);
    }
    return GL_TRUE;
}


static GLboolean glAquaLoseCurrent(__GLcontext *gc)
{
    // GLAQUA_DEBUG_MSG("glAquaLoseCurrent (ctx 0x%x)\n", gc->ctx);
    aglSetCurrentContext(NULL);
    __glXLastContext = NULL; // Mesa does this; why?
    return GL_TRUE;
}

// (x, y, w, h) is global coordinates of view or -1
// glPriv may be NULL
static void attach(__GLcontext *gc, __GLdrawablePrivate *glPriv,
                   int x, int y, int w, int h)
{
    __GLXdrawablePrivate *glxPriv;

    if (glPriv == NULL) {
        // attaching to nothing
        GLAQUA_DEBUG_MSG("unattaching\n");
        aglSetDrawable(gc->ctx, NULL);
        gc->isAttached = FALSE;
        return;
    }

    // Note that when resizing, the X11 WindowPtr already has its
    // new size and position, but the Aqua window does not.

    glxPriv = (__GLXdrawablePrivate *)glPriv->other;

    if (glxPriv->type == DRAWABLE_WINDOW) {
        WindowPtr pWin = (WindowPtr) glxPriv->pDraw;
        WindowPtr topWin = quartzProcs->TopLevelParent(pWin);
        CRWindowPtr crWinPtr;
        AGLDrawable newPort;

        crWinPtr = (CRWindowPtr) quartzProcs->FrameForWindow(pWin, TRUE);

        if (crWinPtr) {
            newPort = (AGLDrawable) crWinPtr->port;
        } else {
            newPort = NULL;
        }

        if (newPort) {
            // fixme won't be a CGrafPtr if currently offscreen or fullscreen
            AGLDrawable oldPort = aglGetDrawable(gc->ctx);
            // AGLDrawable newPort = GetWindowPort(window);
            GLint frame[4] = {x, y, w, h}; // fixme multi-screen?
            // frame is now X11-global
            frame[0] -= topWin->drawable.x - topWin->borderWidth;
            frame[1] -= topWin->drawable.y - topWin->borderWidth;
            // frame is now window-local
            // GL uses flipped coordinates
            frame[1] = topWin->drawable.height + 2*topWin->borderWidth -
                       frame[1] - frame[3];

            // GLAQUA_DEBUG_MSG("local size %d %d %d %d\n",
            // frame[0], frame[1], frame[2], frame[3]);

            if (oldPort != newPort) {
                // fixme retain/release windows
                aglSetDrawable(gc->ctx, newPort);
            }
            aglSetInteger(gc->ctx, AGL_BUFFER_RECT, frame);
            aglEnable(gc->ctx, AGL_BUFFER_RECT);
            // aglSetInteger(gc->ctx, AGL_SWAP_RECT, frame);
            // aglEnable(gc->ctx, AGL_SWAP_RECT);
            aglUpdateContext(gc->ctx);
            gc->isAttached = TRUE;
            GLAQUA_DEBUG_MSG("attached\n");
        } else {
            // attach to not-yet-realized window - will really attach later
            GLAQUA_DEBUG_MSG("couldn't attach\n");
            aglSetDrawable(gc->ctx, NULL);
            gc->isAttached = FALSE;
        }
    } else {
        GLAQUA_DEBUG_MSG("attach: attach to non-window unimplemented\n");
        aglSetDrawable(gc->ctx, NULL);
        gc->isAttached = FALSE;
    }
}

static GLboolean glAquaMakeCurrent(__GLcontext *gc)
{
#if 0
    __GLdrawablePrivate *glPriv = gc->interface.imports.getDrawablePrivate(gc);
    GLAQUA_DEBUG_MSG("glAquaMakeCurrent (ctx 0x%x)\n", gc->ctx);

    if (!gc->isAttached) {
        attach(gc, glPriv, glPriv->xOrigin, glPriv->yOrigin, glPriv->width, glPriv->height);
    }
#endif
    return aglSetCurrentContext(gc->ctx);
}

static GLboolean glAquaShareContext(__GLcontext *gc, __GLcontext *gcShare)
{
  GLAQUA_DEBUG_MSG("glAquaShareContext unimplemented\n");

  return GL_TRUE;
}


static GLboolean glAquaCopyContext(__GLcontext *dst, const __GLcontext *src,
                                   GLuint mask)
{
  GLAQUA_DEBUG_MSG("glAquaCopyContext\n");
  aglCopyContext(src->ctx, dst->ctx, mask);
  return GL_TRUE;
}

static GLboolean glAquaForceCurrent(__GLcontext *gc)
{
    //     GLAQUA_DEBUG_MSG("glAquaForceCurrent (ctx 0x%x)\n", gc->ctx);
    return aglSetCurrentContext(gc->ctx);
}

/* Drawing surface notification callbacks */

static GLboolean glAquaNotifyResize(__GLcontext *gc)
{
    GLAQUA_DEBUG_MSG("unimplemented glAquaNotifyResize");
    return GL_TRUE;
}

static void glAquaNotifyDestroy(__GLcontext *gc)
{
    GLAQUA_DEBUG_MSG("unimplemented glAquaNotifyDestroy");
}

static void glAquaNotifySwapBuffers(__GLcontext *gc)
{
    GLAQUA_DEBUG_MSG("unimplemented glAquaNotifySwapBuffers");
}

/* Dispatch table override control for external agents like libGLS */
static struct __GLdispatchStateRec* glAquaDispatchExec(__GLcontext *gc)
{
    GLAQUA_DEBUG_MSG("unimplemented glAquaDispatchExec");
    return NULL;
}

static void glAquaBeginDispatchOverride(__GLcontext *gc)
{
    GLAQUA_DEBUG_MSG("unimplemented glAquaBeginDispatchOverride");
}

static void glAquaEndDispatchOverride(__GLcontext *gc)
{
    GLAQUA_DEBUG_MSG("unimplemented glAquaEndDispatchOverride");
}


static AGLPixelFormat makeFormat(__GLcontextModes *mode)
{
    int i;
    GLint attr[64]; // currently uses max of 30
    AGLPixelFormat result;

    GLAQUA_DEBUG_MSG("makeFormat\n");

    i = 0;

    // attr [i++] = AGL_ACCELERATED; // require hwaccel - BAD for multiscreen
    // attr [i++] = AGL_NO_RECOVERY; // disable fallback renderers - BAD

    if (mode->stereoMode) {
        attr[i++] = AGL_STEREO;
    }
    if (mode->doubleBufferMode) {
        attr[i++] = AGL_DOUBLEBUFFER;
    }

    if (mode->colorIndexMode) {
        attr[i++] = AGL_BUFFER_SIZE;
        attr[i++] = mode->indexBits;
    }

    if (mode->rgbMode) {
        attr[i++] = AGL_RGBA;
        attr[i++] = AGL_RED_SIZE;
        attr[i++] = mode->redBits;
        attr[i++] = AGL_GREEN_SIZE;
        attr[i++] = mode->greenBits;
        attr[i++] = AGL_BLUE_SIZE;
        attr[i++] = mode->blueBits;
        attr[i++] = AGL_ALPHA_SIZE;
        attr[i++] = mode->alphaBits;
    }

    if (mode->haveAccumBuffer) {
        attr[i++] = AGL_ACCUM_RED_SIZE;
        attr[i++] = mode->accumRedBits;
        attr[i++] = AGL_ACCUM_GREEN_SIZE;
        attr[i++] = mode->accumGreenBits;
        attr[i++] = AGL_ACCUM_BLUE_SIZE;
        attr[i++] = mode->accumBlueBits;
        attr[i++] = AGL_ACCUM_ALPHA_SIZE;
        attr[i++] = mode->accumAlphaBits;
    }
    if (mode->haveDepthBuffer) {
        attr[i++] = AGL_DEPTH_SIZE;
        attr[i++] = mode->depthBits;
    }
    if (mode->haveStencilBuffer) {
        attr[i++] = AGL_STENCIL_SIZE;
        attr[i++] = mode->stencilBits;
    }

    attr[i++] = AGL_AUX_BUFFERS;
    attr[i++] = mode->numAuxBuffers;

    attr[i++] = AGL_LEVEL;
    attr[i++] = mode->level;

    // mode->pixmapMode ?

    attr[i++] = AGL_NONE; // end of option list

    GLAQUA_DEBUG_MSG("makeFormat almost done\n");
    result = aglChoosePixelFormat(NULL, 0, attr);
    GLAQUA_DEBUG_MSG("makeFormat done (0x%x)\n", result);
    return result;
}

static __GLinterface *glAquaCreateContext(__GLimports *imports,
					  __GLcontextModes *mode,
					  __GLinterface *shareGC)
{
    __GLcontext *result;
    __GLcontext *sharectx = (__GLcontext *)shareGC;
    GLint value;

    GLAQUA_DEBUG_MSG("glAquaCreateContext\n");

    result = (__GLcontext *)malloc(sizeof(__GLcontext));
    if (!result) return NULL;

    result->interface.imports = *imports;
    result->interface.exports = glAquaExports;

    result->pixelFormat = makeFormat(mode);
    if (!result->pixelFormat) {
        free(result);
        return NULL;
    }

    result->ctx = aglCreateContext(result->pixelFormat,
                        (sharectx && sharectx->ctx) ? sharectx->ctx : NULL);

    if (!result->ctx) {
        aglDestroyPixelFormat(result->pixelFormat);
        free(result);
        return NULL;
    }

    result->isAttached = FALSE;

    // Tell aglSwapBuffers to wait for vertical retrace
    value = 1;
    aglSetInteger(result->ctx, AGL_SWAP_INTERVAL, &value);

    GLAQUA_DEBUG_MSG("glAquaCreateContext done\n");
    return (__GLinterface *)result;
}


Bool
glAquaRealizeWindow(WindowPtr pWin)
{
    // If this window has GL contexts, tell them to reattach
    Bool result;
    ScreenPtr pScreen = pWin->drawable.pScreen;
    glAquaScreenRec *screenPriv = &glAquaScreens[pScreen->myNum];
    __GLXdrawablePrivate *glxPriv;

    GLAQUA_DEBUG_MSG("glAquaRealizeWindow\n");

    // Allow the window to be created (RootlessRealizeWindow is inside our wrap)
    pScreen->RealizeWindow = screenPriv->RealizeWindow;
    result = pScreen->RealizeWindow(pWin);
    pScreen->RealizeWindow = glAquaRealizeWindow;

    // The Aqua window will already have been created (windows are
    // realized from top down)

    // Re-attach this window's GL contexts, if any.
    glxPriv = __glXFindDrawablePrivate(pWin->drawable.id);
    if (glxPriv) {
        __GLXcontext *gx;
        __GLcontext *gc;
        __GLdrawablePrivate *glPriv = &glxPriv->glPriv;
        GLAQUA_DEBUG_MSG("glAquaRealizeWindow is GL drawable!\n");

        // GL contexts bound to this window for drawing
        for (gx = glxPriv->drawGlxc; gx != NULL; gx = gx->next) {
            gc = (__GLcontext *)gx->gc;
            attach(gc, glPriv, glxPriv->xorigin, glxPriv->yorigin,
                   glxPriv->width, glxPriv->height);
        }

        // GL contexts bound to this window for reading
        for (gx = glxPriv->readGlxc; gx != NULL; gx = gx->next) {
            gc = (__GLcontext *)gx->gc;
            attach(gc, glPriv, glxPriv->xorigin, glxPriv->yorigin,
                   glxPriv->width, glxPriv->height);
        }
    }

    return result;
}

Bool
glAquaUnrealizeWindow(WindowPtr pWin)
{
    // If this window has GL contexts, tell them to unattach
    Bool result;
    ScreenPtr pScreen = pWin->drawable.pScreen;
    glAquaScreenRec *screenPriv = &glAquaScreens[pScreen->myNum];
    __GLXdrawablePrivate *glxPriv;

    GLAQUA_DEBUG_MSG("glAquaUnrealizeWindow\n");

    // The Aqua window may have already been destroyed (windows
    // are unrealized from top down)

    // Unattach this window's GL contexts, if any.
    glxPriv = __glXFindDrawablePrivate(pWin->drawable.id);
    if (glxPriv) {
        __GLXcontext *gx;
        __GLcontext *gc;
        GLAQUA_DEBUG_MSG("glAquaUnealizeWindow is GL drawable!\n");

        // GL contexts bound to this window for drawing
        for (gx = glxPriv->drawGlxc; gx != NULL; gx = gx->next) {
            gc = (__GLcontext *)gx->gc;
            attach(gc, NULL, 0, 0, 0, 0);
        }

        // GL contexts bound to this window for reading
        for (gx = glxPriv->readGlxc; gx != NULL; gx = gx->next) {
            gc = (__GLcontext *)gx->gc;
            attach(gc, NULL, 0, 0, 0, 0);
        }
    }

    pScreen->UnrealizeWindow = screenPriv->UnrealizeWindow;
    result = pScreen->UnrealizeWindow(pWin);
    pScreen->UnrealizeWindow = glAquaUnrealizeWindow;

    return result;
}






/*
 * In the case the driver has no GLX visuals we'll use these.
 * [0] = RGB, double buffered
 * [1] = RGB, double buffered, stencil, accum
 */
// Originally copied from Mesa

static int                 numConfigs     = 0;
static __GLXvisualConfig  *visualConfigs  = NULL;
static void              **visualPrivates = NULL;

#define NUM_FALLBACK_CONFIGS 2
static __GLXvisualConfig FallbackConfigs[NUM_FALLBACK_CONFIGS] = {
  {
    -1,                 /* vid */
    -1,                 /* class */
    True,               /* rgba */
    -1, -1, -1, 0,      /* rgba sizes */
    -1, -1, -1, 0,      /* rgba masks */
     0,  0,  0, 0,      /* rgba accum sizes */
    True,               /* doubleBuffer */
    False,              /* stereo */
    -1,                 /* bufferSize */
    16,                 /* depthSize */
    0,                  /* stencilSize */
    0,                  /* auxBuffers */
    0,                  /* level */
    GLX_NONE_EXT,       /* visualRating */
    0,                  /* transparentPixel */
    0, 0, 0, 0,         /* transparent rgba color (floats scaled to ints) */
    0                   /* transparentIndex */
  },
  {
    -1,                 /* vid */
    -1,                 /* class */
    True,               /* rgba */
    -1, -1, -1, 0,      /* rgba sizes */
    -1, -1, -1, 0,      /* rgba masks */
    16, 16, 16, 0,      /* rgba accum sizes */
    True,               /* doubleBuffer */
    False,              /* stereo */
    -1,                 /* bufferSize */
    16,                 /* depthSize */
    8,                  /* stencilSize */
    0,                  /* auxBuffers */
    0,                  /* level */
    GLX_NONE_EXT,       /* visualRating */
    0,                  /* transparentPixel */
    0, 0, 0, 0,         /* transparent rgba color (floats scaled to ints) */
    0                   /* transparentIndex */
  }
};



static int count_bits(unsigned int n)
{
   int bits = 0;

   while (n > 0) {
      if (n & 1) bits++;
      n >>= 1;
   }
   return bits;
}


// Mostly copied from Mesa's xf86glx.c
static Bool init_visuals(int *nvisualp, VisualPtr *visualp,
			 VisualID *defaultVisp,
			 int ndepth, DepthPtr pdepth,
			 int rootDepth)
{
    int numRGBconfigs;
    int numCIconfigs;
    int numVisuals = *nvisualp;
    int numNewVisuals;
    int numNewConfigs;
    VisualPtr pVisual = *visualp;
    VisualPtr pVisualNew = NULL;
    VisualID *orig_vid = NULL;
    __GLXvisualConfig *glXVisualPtr = NULL;
    __GLXvisualConfig *pNewVisualConfigs = NULL;
    void **glXVisualPriv;
    void **pNewVisualPriv;
    int found_default;
    int i, j, k;

    GLAQUA_DEBUG_MSG("init_visuals\n");

    if (numConfigs > 0)
        numNewConfigs = numConfigs;
    else
        numNewConfigs = NUM_FALLBACK_CONFIGS;

    /* Alloc space for the list of new GLX visuals */
    pNewVisualConfigs = (__GLXvisualConfig *)
                     __glXMalloc(numNewConfigs * sizeof(__GLXvisualConfig));
    if (!pNewVisualConfigs) {
	return FALSE;
    }

    /* Alloc space for the list of new GLX visual privates */
    pNewVisualPriv = (void **) __glXMalloc(numNewConfigs * sizeof(void *));
    if (!pNewVisualPriv) {
	__glXFree(pNewVisualConfigs);
	return FALSE;
    }

    /*
    ** If SetVisualConfigs was not called, then use default GLX
    ** visual configs.
    */
    if (numConfigs == 0) {
	memcpy(pNewVisualConfigs, FallbackConfigs,
               NUM_FALLBACK_CONFIGS * sizeof(__GLXvisualConfig));
	memset(pNewVisualPriv, 0, NUM_FALLBACK_CONFIGS * sizeof(void *));
    }
    else {
        /* copy driver's visual config info */
        for (i = 0; i < numConfigs; i++) {
            pNewVisualConfigs[i] = visualConfigs[i];
            pNewVisualPriv[i] = visualPrivates[i];
        }
    }

    /* Count the number of RGB and CI visual configs */
    numRGBconfigs = 0;
    numCIconfigs = 0;
    for (i = 0; i < numNewConfigs; i++) {
	if (pNewVisualConfigs[i].rgba)
	    numRGBconfigs++;
	else
	    numCIconfigs++;
    }

    /* Count the total number of visuals to compute */
    numNewVisuals = 0;
    for (i = 0; i < numVisuals; i++) {
        numNewVisuals +=
	    (pVisual[i].class == TrueColor || pVisual[i].class == DirectColor)
	    ? numRGBconfigs : numCIconfigs;
    }

    /* Reset variables for use with the next screen/driver's visual configs */
    visualConfigs = NULL;
    numConfigs = 0;

    /* Alloc temp space for the list of orig VisualIDs for each new visual */
    orig_vid = (VisualID *)__glXMalloc(numNewVisuals * sizeof(VisualID));
    if (!orig_vid) {
	__glXFree(pNewVisualPriv);
	__glXFree(pNewVisualConfigs);
	return FALSE;
    }

    /* Alloc space for the list of glXVisuals */
    glXVisualPtr = (__GLXvisualConfig *)__glXMalloc(numNewVisuals *
						    sizeof(__GLXvisualConfig));
    if (!glXVisualPtr) {
	__glXFree(orig_vid);
	__glXFree(pNewVisualPriv);
	__glXFree(pNewVisualConfigs);
	return FALSE;
    }

    /* Alloc space for the list of glXVisualPrivates */
    glXVisualPriv = (void **)__glXMalloc(numNewVisuals * sizeof(void *));
    if (!glXVisualPriv) {
	__glXFree(glXVisualPtr);
	__glXFree(orig_vid);
	__glXFree(pNewVisualPriv);
	__glXFree(pNewVisualConfigs);
	return FALSE;
    }

    /* Alloc space for the new list of the X server's visuals */
    pVisualNew = (VisualPtr)__glXMalloc(numNewVisuals * sizeof(VisualRec));
    if (!pVisualNew) {
	__glXFree(glXVisualPriv);
	__glXFree(glXVisualPtr);
	__glXFree(orig_vid);
	__glXFree(pNewVisualPriv);
	__glXFree(pNewVisualConfigs);
	return FALSE;
    }

    /* Initialize the new visuals */
    found_default = FALSE;
    for (i = j = 0; i < numVisuals; i++) {
        int is_rgb = (pVisual[i].class == TrueColor ||
		      pVisual[i].class == DirectColor);

	for (k = 0; k < numNewConfigs; k++) {
	    if (pNewVisualConfigs[k].rgba != is_rgb)
		continue;

	    /* Initialize the new visual */
	    pVisualNew[j] = pVisual[i];
	    pVisualNew[j].vid = FakeClientID(0);

	    /* Check for the default visual */
	    if (!found_default && pVisual[i].vid == *defaultVisp) {
		*defaultVisp = pVisualNew[j].vid;
		found_default = TRUE;
	    }

	    /* Save the old VisualID */
	    orig_vid[j] = pVisual[i].vid;

	    /* Initialize the glXVisual */
	    glXVisualPtr[j] = pNewVisualConfigs[k];
	    glXVisualPtr[j].vid = pVisualNew[j].vid;

	    /*
	     * If the class is -1, then assume the X visual information
	     * is identical to what GLX needs, and take them from the X
	     * visual.  NOTE: if class != -1, then all other fields MUST
	     * be initialized.
	     */
	    if (glXVisualPtr[j].class == -1) {
		glXVisualPtr[j].class      = pVisual[i].class;
		glXVisualPtr[j].redSize    = count_bits(pVisual[i].redMask);
		glXVisualPtr[j].greenSize  = count_bits(pVisual[i].greenMask);
		glXVisualPtr[j].blueSize   = count_bits(pVisual[i].blueMask);
		glXVisualPtr[j].alphaSize  = glXVisualPtr[j].alphaSize;
		glXVisualPtr[j].redMask    = pVisual[i].redMask;
		glXVisualPtr[j].greenMask  = pVisual[i].greenMask;
		glXVisualPtr[j].blueMask   = pVisual[i].blueMask;
		glXVisualPtr[j].alphaMask  = glXVisualPtr[j].alphaMask;
		glXVisualPtr[j].bufferSize = rootDepth;
	    }

	    /* Save the device-dependent private for this visual */
	    glXVisualPriv[j] = pNewVisualPriv[k];

	    j++;
	}
    }

    assert(j <= numNewVisuals);

    /* Save the GLX visuals in the screen structure */
    glAquaScreens[screenInfo.numScreens-1].num_vis = numNewVisuals;
    glAquaScreens[screenInfo.numScreens-1].glx_vis = glXVisualPtr;
    glAquaScreens[screenInfo.numScreens-1].priv = glXVisualPriv;

    /* Set up depth's VisualIDs */
    for (i = 0; i < ndepth; i++) {
	int numVids = 0;
	VisualID *pVids = NULL;
	int k, n = 0;

	/* Count the new number of VisualIDs at this depth */
	for (j = 0; j < pdepth[i].numVids; j++)
	    for (k = 0; k < numNewVisuals; k++)
		if (pdepth[i].vids[j] == orig_vid[k])
		    numVids++;

	/* Allocate a new list of VisualIDs for this depth */
	pVids = (VisualID *)__glXMalloc(numVids * sizeof(VisualID));

	/* Initialize the new list of VisualIDs for this depth */
	for (j = 0; j < pdepth[i].numVids; j++)
	    for (k = 0; k < numNewVisuals; k++)
		if (pdepth[i].vids[j] == orig_vid[k])
		    pVids[n++] = pVisualNew[k].vid;

	/* Update this depth's list of VisualIDs */
	__glXFree(pdepth[i].vids);
	pdepth[i].vids = pVids;
	pdepth[i].numVids = numVids;
    }

    /* Update the X server's visuals */
    *nvisualp = numNewVisuals;
    *visualp = pVisualNew;

    /* Free the old list of the X server's visuals */
    __glXFree(pVisual);

    /* Clean up temporary allocations */
    __glXFree(orig_vid);
    __glXFree(pNewVisualPriv);
    __glXFree(pNewVisualConfigs);

    /* Free the private list created by DDX HW driver */
    if (visualPrivates)
        xfree(visualPrivates);
    visualPrivates = NULL;

    return TRUE;
}

// Copied from Mesa
static void glAquaSetVisualConfigs(int nconfigs, __GLXvisualConfig *configs,
				   void **privates)
{
    GLAQUA_DEBUG_MSG("glAquaSetVisualConfigs\n");

    numConfigs = nconfigs;
    visualConfigs = configs;
    visualPrivates = privates;
}

// Copied from Mesa
static Bool glAquaInitVisuals(VisualPtr *visualp, DepthPtr *depthp,
			      int *nvisualp, int *ndepthp,
			      int *rootDepthp, VisualID *defaultVisp,
			      unsigned long sizes, int bitsPerRGB)
{
    GLAQUA_DEBUG_MSG("glAquaInitVisuals\n");

    /*
     * Setup the visuals supported by this particular screen.
     */
    return init_visuals(nvisualp, visualp, defaultVisp,
			*ndepthp, *depthp, *rootDepthp);
}


static void fixup_visuals(int screen)
{
    ScreenPtr pScreen = screenInfo.screens[screen];
    glAquaScreenRec *pScr = &glAquaScreens[screen];
    __GLXvisualConfig *pGLXVis  = pScr->glx_vis;
    VisualPtr pVis;
    int i, j;

    GLAQUA_DEBUG_MSG("fixup_visuals\n");

    for (i = 0; i < pScr->num_vis; i++, pGLXVis++) {
	pVis = pScreen->visuals;

	/* Find a visual that matches the GLX visual's class and size */
	for (j = 0; j < pScreen->numVisuals; j++, pVis++) {
	    if (pVis->class == pGLXVis->class &&
		pVis->nplanes == pGLXVis->bufferSize) {

		/* Fixup the masks */
		pGLXVis->redMask   = pVis->redMask;
		pGLXVis->greenMask = pVis->greenMask;
		pGLXVis->blueMask  = pVis->blueMask;

		/* Recalc the sizes */
		pGLXVis->redSize   = count_bits(pGLXVis->redMask);
		pGLXVis->greenSize = count_bits(pGLXVis->greenMask);
		pGLXVis->blueSize  = count_bits(pGLXVis->blueMask);
	    }
	}
    }
}

static void init_screen_visuals(int screen)
{
    ScreenPtr pScreen = screenInfo.screens[screen];
    __GLXvisualConfig *pGLXVis = glAquaScreens[screen].glx_vis;
    // XMesaVisual *pXMesaVisual;
    VisualPtr pVis;
    int *used;
    int i, j;

    GLAQUA_DEBUG_MSG("init_screen_visuals\n");

    /* Alloc space for the list of XMesa visuals */
    // pXMesaVisual = (XMesaVisual *)__glXMalloc(MESAScreens[screen].num_vis *
    //					      sizeof(XMesaVisual));
    // __glXMemset(pXMesaVisual, 0,
    //		MESAScreens[screen].num_vis * sizeof(XMesaVisual));

    used = (int *)__glXMalloc(pScreen->numVisuals * sizeof(int));
    __glXMemset(used, 0, pScreen->numVisuals * sizeof(int));

    for (i = 0; i < glAquaScreens[screen].num_vis; i++, pGLXVis++) {
	pVis = pScreen->visuals;
	for (j = 0; j < pScreen->numVisuals; j++, pVis++) {

	    if (pVis->class == pGLXVis->class &&
		pVis->nplanes == pGLXVis->bufferSize &&
		!used[j]) {

		if (pVis->redMask   == pGLXVis->redMask &&
		    pVis->greenMask == pGLXVis->greenMask &&
		    pVis->blueMask  == pGLXVis->blueMask) {

		    /* Create the XMesa visual */
		    /*
		    pXMesaVisual[i] =
                         XMesaCreateVisual(pScreen,
					   pVis,
					   pGLXVis->rgba,
					   (pGLXVis->alphaSize > 0),
					   pGLXVis->doubleBuffer,
					   pGLXVis->stereo,
					   GL_TRUE, // ximage_flag
					   pGLXVis->depthSize,
					   pGLXVis->stencilSize,
					   pGLXVis->accumRedSize,
					   pGLXVis->accumGreenSize,
					   pGLXVis->accumBlueSize,
					   pGLXVis->accumAlphaSize,
                                           0,  // numSamples
					   pGLXVis->level,
                                           pGLXVis->visualRating );
		    */
		    /* Set the VisualID */
		    pGLXVis->vid = pVis->vid;

		    /* Mark this visual used */
		    used[j] = 1;
		    break;
		}
	    }
	}
    }

    __glXFree(used);

    // glAquaScreens[screen].xm_vis = pXMesaVisual;
}

static Bool glAquaScreenProbe(int screen)
{
    ScreenPtr pScreen;
    glAquaScreenRec *screenPriv;

    GLAQUA_DEBUG_MSG("glAquaScreenProbe\n");

    /*
     * Set up the current screen's visuals.
     */
    __glDDXScreenInfo.pGlxVisual = glAquaScreens[screen].glx_vis;
    __glDDXScreenInfo.pVisualPriv = glAquaScreens[screen].priv;
    __glDDXScreenInfo.numVisuals =
	__glDDXScreenInfo.numUsableVisuals = glAquaScreens[screen].num_vis;

    /*
     * Set the current screen's createContext routine.  This could be
     * wrapped by a DDX GLX context creation routine.
     */
    __glDDXScreenInfo.createContext = glAquaCreateContext;

    /*
     * The ordering of the rgb compenents might have been changed by the
     * driver after mi initialized them.
     */
    fixup_visuals(screen);

    /*
     * Find the GLX visuals that are supported by this screen and create
     * XMesa's visuals.
     */
    init_screen_visuals(screen);

    // Wrap RealizeWindow and UnrealizeWindow on this screen
    pScreen = screenInfo.screens[screen];
    screenPriv = &glAquaScreens[screen];
    screenPriv->RealizeWindow = pScreen->RealizeWindow;
    pScreen->RealizeWindow = glAquaRealizeWindow;
    screenPriv->UnrealizeWindow = pScreen->UnrealizeWindow;
    pScreen->UnrealizeWindow = glAquaUnrealizeWindow;

    return TRUE;
}


static GLboolean glAquaResizeBuffers(__GLdrawableBuffer *buffer,
				     GLint x, GLint y,
				     GLuint width, GLuint height,
				     __GLdrawablePrivate *glPriv,
				     GLuint bufferMask)
{
    GLAquaDrawableRec *aquaPriv = (GLAquaDrawableRec *)glPriv->private;
    __GLXcontext *gx;
    __GLcontext *gc;
    __GLXdrawablePrivate *glxPriv = (__GLXdrawablePrivate *)glPriv->other;

    GLAQUA_DEBUG_MSG("glAquaResizeBuffers to (%d %d %d %d)\n", x, y, width, height);

    // update all contexts that point at this drawable for drawing (hack?)
    for (gx = glxPriv->drawGlxc; gx != NULL; gx = gx->next) {
        gc = (__GLcontext *)gx->gc;
        attach(gc, glPriv, x, y, width, height);
    }

    // update all contexts that point at this drawable for reading (hack?)
    for (gx = glxPriv->readGlxc; gx != NULL; gx = gx->next) {
        gc = (__GLcontext *)gx->gc;
        attach(gc, glPriv, x, y, width, height);
    }

    return aquaPriv->resize(buffer, x, y, width, height, glPriv, bufferMask);
}


static GLboolean glAquaSwapBuffers(__GLXdrawablePrivate *glxPriv)
{
    // fixme AGL software renderer will use properties of current QD port (bad)

    // swap buffers on only *one* of the contexts
    // (e.g. the last one for drawing)
    __GLcontext *gc = (__GLcontext *)glxPriv->drawGlxc->gc;
    if (gc && gc->ctx) aglSwapBuffers(gc->ctx);

    return GL_TRUE;
}

static void glAquaDestroyDrawablePrivate(__GLdrawablePrivate *glPriv)
{
    GLAQUA_DEBUG_MSG("glAquaDestroyDrawablePrivate\n");

    free(glPriv->private);
    glPriv->private = NULL;
}

static void glAquaCreateBuffer(__GLXdrawablePrivate *glxPriv)
{
    GLAquaDrawableRec *aquaPriv = malloc(sizeof(GLAquaDrawableRec));
    __GLdrawablePrivate *glPriv = &glxPriv->glPriv;

    GLAQUA_DEBUG_MSG("glAquaCreateBuffer\n");

    // replace swapBuffers (original is never called)
    glxPriv->swapBuffers = glAquaSwapBuffers;

    // wrap front buffer resize
    aquaPriv->resize = glPriv->frontBuffer.resize;
    glPriv->frontBuffer.resize = glAquaResizeBuffers;

    // stash private data
    glPriv->private = aquaPriv;
    glPriv->freePrivate = glAquaDestroyDrawablePrivate;
}


static void glAquaResetExtension(void)
{
    GLAQUA_DEBUG_MSG("glAquaResetExtension\n");
    aglResetLibrary();
}



// Extra goodies for glx

GLint __glEvalComputeK(GLenum target)
{
    switch (target) {
    case GL_MAP1_VERTEX_4:
    case GL_MAP1_COLOR_4:
    case GL_MAP1_TEXTURE_COORD_4:
    case GL_MAP2_VERTEX_4:
    case GL_MAP2_COLOR_4:
    case GL_MAP2_TEXTURE_COORD_4:
	return 4;
    case GL_MAP1_VERTEX_3:
    case GL_MAP1_TEXTURE_COORD_3:
    case GL_MAP1_NORMAL:
    case GL_MAP2_VERTEX_3:
    case GL_MAP2_TEXTURE_COORD_3:
    case GL_MAP2_NORMAL:
	return 3;
    case GL_MAP1_TEXTURE_COORD_2:
    case GL_MAP2_TEXTURE_COORD_2:
	return 2;
    case GL_MAP1_TEXTURE_COORD_1:
    case GL_MAP2_TEXTURE_COORD_1:
    case GL_MAP1_INDEX:
    case GL_MAP2_INDEX:
	return 1;
    default:
	return 0;
    }
}

GLuint __glFloorLog2(GLuint val)
{
    int c = 0;

    while (val > 1) {
	c++;
	val >>= 1;
    }
    return c;
}
