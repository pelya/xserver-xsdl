/*
 * GLX implementation that uses Apple's OpenGL.framework
 * (Indirect rendering path)
 */
/*
 * Copyright (c) 2002 Greg Parker. All Rights Reserved.
 * Copyright (c) 2002 Apple Computer, Inc.
 * Copyright (c) 2004 Torrey T. Lyons. All Rights Reserved.
 *
 * Portions of this file are copied from Mesa's xf86glx.c,
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
 */
/* $XFree86: xc/programs/Xserver/GL/apple/indirect.c,v 1.2 2003/09/16 00:36:11 torrey Exp $ */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "dri.h"
#include "quartz.h"

#include <CoreGraphics/CoreGraphics.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/CGLContext.h>

// X11 and X11's glx
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
#include "x-hash.h"
#include "x-list.h"

#include "glcontextmodes.h"

// ggs: needed to call back to glx with visual configs
extern void GlxSetVisualConfigs(int nconfigs, __GLXvisualConfig *configs, void **configprivs);


// Write debugging output, or not
#ifdef GLAQUA_DEBUG
#define GLAQUA_DEBUG_MSG ErrorF
#else
#define GLAQUA_DEBUG_MSG(a, ...)
#endif


// The following GL functions don't have an EXT suffix in OpenGL.framework.
GLboolean glAreTexturesResidentEXT(GLsizei a, const GLuint *b, GLboolean *c) {
    return glAreTexturesResident(a, b, c);
}
void glDeleteTexturesEXT(GLsizei d, const GLuint *e) {
    glDeleteTextures(d, e);
}
void glGenTexturesEXT(GLsizei f, GLuint *g) {
    glGenTextures(f, g);
}
GLboolean glIsTextureEXT(GLuint h) {
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
 * struct.  In particular, the contextCreate, modes, numVisuals,
 * and numUsableVisuals fields must be initialized.
 */
static __GLXscreenInfo __glDDXScreenInfo = {
    glAquaScreenProbe,   /* Must be generic and handle all screens */
    glAquaCreateContext, /* Substitute screen's createContext routine */
    glAquaCreateBuffer,  /* Substitute screen's createBuffer routine */
    NULL,                 /* Set up modes in probe */
    NULL,                 /* Set up pVisualPriv in probe */
    0,                    /* Set up numVisuals in probe */
    0,                    /* Set up numUsableVisuals in probe */
    "Vendor String",      /* GLXvendor is overwritten by __glXScreenInit */
    "Version String",     /* GLXversion is overwritten by __glXScreenInit */
    "Extensions String",  /* GLXextensions is overwritten by __glXScreenInit */
    NULL                  /* WrappedPositionWindow is overwritten */
};

void *__glXglDDXScreenInfo(void) {
    return &__glDDXScreenInfo;
}

static __GLXextensionInfo __glDDXExtensionInfo = {
    GL_CORE_APPLE,
    glAquaResetExtension,
    glAquaInitVisuals,
    glAquaSetVisualConfigs
};

void *__glXglDDXExtensionInfo(void) {
    return &__glDDXExtensionInfo;
}

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
    __GLcontextModes *modes;
    void **priv;

    // wrapped screen functions
    RealizeWindowProcPtr RealizeWindow;
    UnrealizeWindowProcPtr UnrealizeWindow;
} glAquaScreenRec;

static glAquaScreenRec glAquaScreens[MAXSCREENS];

// __GLdrawablePrivate->private
typedef struct {
    DrawablePtr pDraw;
    xp_surface_id sid;
} GLAquaDrawableRec;

struct __GLcontextRec {
    struct __GLinterfaceRec interface; // required to be first

    CGLContextObj ctx;
    CGLPixelFormatObj pixelFormat;

    /* set when attached */
    xp_surface_id sid;

    unsigned isAttached :1;
};

/* maps from surface id -> list of __GLcontext */
static x_hash_table *surface_hash;


// Context manipulation; return GL_FALSE on failure
static GLboolean glAquaDestroyContext(__GLcontext *gc)
{
    x_list *lst;

    GLAQUA_DEBUG_MSG("glAquaDestroyContext (ctx 0x%x)\n",
                     (unsigned int) gc->ctx);

    if (gc != NULL)
    {
        if (gc->sid != 0 && surface_hash != NULL)
        {
            lst = x_hash_table_lookup(surface_hash, (void *) gc->sid, NULL);
            lst = x_list_remove(lst, gc);
            x_hash_table_insert(surface_hash, (void *) gc->sid, lst);
        }

        if (gc->ctx != NULL)
            CGLDestroyContext(gc->ctx);

        if (gc->pixelFormat != NULL)
            CGLDestroyPixelFormat(gc->pixelFormat);

        free(gc);
    }

    return GL_TRUE;
}

static GLboolean glAquaLoseCurrent(__GLcontext *gc)
{
    CGLError gl_err;

    GLAQUA_DEBUG_MSG("glAquaLoseCurrent (ctx 0x%x)\n", (unsigned int) gc->ctx);

    gl_err = CGLSetCurrentContext(NULL);
    if (gl_err != 0)
        ErrorF("CGLSetCurrentContext error: %s\n", CGLErrorString(gl_err));

    __glXLastContext = NULL; // Mesa does this; why?

    return GL_TRUE;
}

/* Called when a surface is destroyed as a side effect of destroying
   the window it's attached to. */
static void surface_notify(void *_arg, void *data)
{
    DRISurfaceNotifyArg *arg = _arg;
    GLAquaDrawableRec *aquaPriv = data;
    __GLcontext *gc;
    x_list *lst;

    switch (arg->kind)
    {
    case AppleDRISurfaceNotifyDestroyed:
        if (surface_hash != NULL)
            x_hash_table_remove(surface_hash, (void *) arg->id);

        aquaPriv->pDraw = NULL;
        aquaPriv->sid = 0;
        break;

    case AppleDRISurfaceNotifyChanged:
        if (surface_hash != NULL)
        {
            lst = x_hash_table_lookup(surface_hash, (void *) arg->id, NULL);
            for (; lst != NULL; lst = lst->next)
            {
                gc = lst->data;
                xp_update_gl_context(gc->ctx);
            }
        }
        break;
    }
}

static void unattach(__GLcontext *gc)
{
    x_list *lst;

    if (gc->isAttached)
    {
        GLAQUA_DEBUG_MSG("unattaching\n");

        if (surface_hash != NULL)
        {
            lst = x_hash_table_lookup(surface_hash, (void *) gc->sid, NULL);
            lst = x_list_remove(lst, gc);
            x_hash_table_insert(surface_hash, (void *) gc->sid, lst);
        }

        CGLClearDrawable(gc->ctx);
        gc->isAttached = FALSE;
        gc->sid = 0;
    }
}

static void attach(__GLcontext *gc, __GLdrawablePrivate *glPriv)
{
    __GLXdrawablePrivate *glxPriv;
    GLAquaDrawableRec *aquaPriv;
    DrawablePtr pDraw;

    glxPriv = (__GLXdrawablePrivate *)glPriv->other;
    aquaPriv = (GLAquaDrawableRec *)glPriv->private;
    pDraw = glxPriv->pDraw;

    if (aquaPriv->sid == 0)
    {
        if (!quartzProcs->CreateSurface(pDraw->pScreen, pDraw->id, pDraw,
                                        0, &aquaPriv->sid, NULL,
                                        surface_notify, aquaPriv))
        {
            return;
        }
        aquaPriv->pDraw = pDraw;
    }

    if (!gc->isAttached || gc->sid != aquaPriv->sid)
    {
        x_list *lst;

        if (xp_attach_gl_context(gc->ctx, aquaPriv->sid) != Success)
        {
            quartzProcs->DestroySurface(pDraw->pScreen, pDraw->id, pDraw,
                                        surface_notify, aquaPriv);
            if (surface_hash != NULL)
                x_hash_table_remove(surface_hash, (void *) aquaPriv->sid);

            aquaPriv->sid = 0;
            return;
        }

        gc->isAttached = TRUE;
        gc->sid = aquaPriv->sid;

        if (surface_hash == NULL)
            surface_hash = x_hash_table_new(NULL, NULL, NULL, NULL);

        lst = x_hash_table_lookup(surface_hash, (void *) gc->sid, NULL);
        if (x_list_find(lst, gc) == NULL)
        {
            lst = x_list_prepend(lst, gc);
            x_hash_table_insert(surface_hash, (void *) gc->sid, lst);
        }

        GLAQUA_DEBUG_MSG("attached 0x%x to 0x%x\n", (unsigned int) pDraw->id,
                         (unsigned int) aquaPriv->sid);
    }
}

static GLboolean glAquaMakeCurrent(__GLcontext *gc)
{
    __GLdrawablePrivate *glPriv = gc->interface.imports.getDrawablePrivate(gc);
    CGLError gl_err;

    GLAQUA_DEBUG_MSG("glAquaMakeCurrent (ctx 0x%x)\n", (unsigned int) gc->ctx);

    attach(gc, glPriv);

    gl_err = CGLSetCurrentContext(gc->ctx);
    if (gl_err != 0)
        ErrorF("CGLSetCurrentContext error: %s\n", CGLErrorString(gl_err));

    return gl_err == 0;
}

static GLboolean glAquaShareContext(__GLcontext *gc, __GLcontext *gcShare)
{
  GLAQUA_DEBUG_MSG("glAquaShareContext unimplemented\n");

  return GL_TRUE;
}

static GLboolean glAquaCopyContext(__GLcontext *dst, const __GLcontext *src,
                                   GLuint mask)
{
    CGLError gl_err;

    GLAQUA_DEBUG_MSG("glAquaCopyContext\n");

    gl_err = CGLCopyContext(src->ctx, dst->ctx, mask);
    if (gl_err != 0)
        ErrorF("CGLCopyContext error: %s\n", CGLErrorString(gl_err));

    return gl_err == 0;
}

static GLboolean glAquaForceCurrent(__GLcontext *gc)
{
    CGLError gl_err;

    GLAQUA_DEBUG_MSG("glAquaForceCurrent (ctx 0x%x)\n",
                     (unsigned int) gc->ctx);

    gl_err = CGLSetCurrentContext(gc->ctx);
    if (gl_err != 0)
        ErrorF("CGLSetCurrentContext error: %s\n", CGLErrorString(gl_err));

    return gl_err == 0;
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

static CGLPixelFormatObj makeFormat(__GLcontextModes *mode)
{
    int i;
    CGLPixelFormatAttribute attr[64]; // currently uses max of 30
    CGLPixelFormatObj result;
    long n_formats;
    CGLError gl_err;
    
    GLAQUA_DEBUG_MSG("makeFormat\n");

    if (!mode->rgbMode)
        return NULL;

    i = 0;

    // attr [i++] = kCGLPFAAcelerated; // require hwaccel - BAD for multiscreen
    // attr [i++] = kCGLPFANoRecovery; // disable fallback renderers - BAD

    if (mode->stereoMode) {
        attr[i++] = kCGLPFAStereo;
    }
    if (mode->doubleBufferMode) {
        attr[i++] = kCGLPFADoubleBuffer;
    }

    if (mode->colorIndexMode) {
        /* ignored */
    }

    if (mode->rgbMode) {
        attr[i++] = kCGLPFAColorSize;
        attr[i++] = mode->redBits + mode->greenBits + mode->blueBits;
        attr[i++] = kCGLPFAAlphaSize;
        attr[i++] = 1; /* FIXME: ignoring mode->alphaBits which is always 0 */
    }

    if (mode->haveAccumBuffer) {
        attr[i++] = kCGLPFAAccumSize;
        attr[i++] = mode->accumRedBits + mode->accumGreenBits
                    + mode->accumBlueBits + mode->accumAlphaBits;
    }
    if (mode->haveDepthBuffer) {
        attr[i++] = kCGLPFADepthSize;
        attr[i++] = mode->depthBits;
    }
    if (mode->haveStencilBuffer) {
        attr[i++] = kCGLPFAStencilSize;
        attr[i++] = mode->stencilBits;
    }

    attr[i++] = kCGLPFAAuxBuffers;
    attr[i++] = mode->numAuxBuffers;

    /* mode->level ignored */

    /* mode->pixmapMode ? */

    attr[i++] = 0;

    GLAQUA_DEBUG_MSG("makeFormat almost done\n");

    result = NULL;
    gl_err = CGLChoosePixelFormat(attr, &result, &n_formats);
    if (gl_err != 0)
        ErrorF("CGLChoosePixelFormat error: %s\n", CGLErrorString(gl_err));

    GLAQUA_DEBUG_MSG("makeFormat done (0x%x)\n", (unsigned int) result);

    return result;
}

static __GLinterface *glAquaCreateContext(__GLimports *imports,
                                          __GLcontextModes *mode,
                                          __GLinterface *shareGC)
{
    __GLcontext *result;
    __GLcontext *sharectx = (__GLcontext *)shareGC;
    CGLError gl_err;

    GLAQUA_DEBUG_MSG("glAquaCreateContext\n");

    result = (__GLcontext *)calloc(1, sizeof(__GLcontext));
    if (!result) return NULL;

    result->interface.imports = *imports;
    result->interface.exports = glAquaExports;

    result->pixelFormat = makeFormat(mode);
    if (!result->pixelFormat) {
        free(result);
        return NULL;
    }

    result->ctx = NULL;
    gl_err = CGLCreateContext(result->pixelFormat,
                              sharectx ? sharectx->ctx : NULL,
                              &result->ctx);

    if (gl_err != 0) {
        ErrorF("CGLCreateContext error: %s\n", CGLErrorString(gl_err));
        CGLDestroyPixelFormat(result->pixelFormat);
        free(result);
        return NULL;
    }

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
            attach(gc, glPriv);
        }

        // GL contexts bound to this window for reading
        for (gx = glxPriv->readGlxc; gx != NULL; gx = gx->next) {
            gc = (__GLcontext *)gx->gc;
            attach(gc, glPriv);
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
            unattach(gc);
        }

        // GL contexts bound to this window for reading
        for (gx = glxPriv->readGlxc; gx != NULL; gx = gx->next) {
            gc = (__GLcontext *)gx->gc;
            unattach(gc);
        }
    }

    pScreen->UnrealizeWindow = screenPriv->UnrealizeWindow;
    result = pScreen->UnrealizeWindow(pWin);
    pScreen->UnrealizeWindow = glAquaUnrealizeWindow;

    return result;
}


// Originally copied from Mesa

static int                 numConfigs     = 0;
static __GLXvisualConfig  *visualConfigs  = NULL;
static void              **visualPrivates = NULL;

/*
 * In the case the driver defines no GLX visuals we'll use these.
 * Note that for TrueColor and DirectColor visuals, bufferSize is the 
 * sum of redSize, greenSize, blueSize and alphaSize, which may be larger 
 * than the nplanes/rootDepth of the server's X11 visuals
 */
#define NUM_FALLBACK_CONFIGS 5
static __GLXvisualConfig FallbackConfigs[NUM_FALLBACK_CONFIGS] = {
  /* [0] = RGB, double buffered, Z */
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
    GLX_NONE,           /* visualRating */
    GLX_NONE,           /* transparentPixel */
    0, 0, 0, 0,         /* transparent rgba color (floats scaled to ints) */
    0                   /* transparentIndex */
  },
  /* [1] = RGB, double buffered, Z, stencil, accum */
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
    GLX_NONE,           /* visualRating */
    GLX_NONE,           /* transparentPixel */
    0, 0, 0, 0,         /* transparent rgba color (floats scaled to ints) */
    0                   /* transparentIndex */
  },
  /* [2] = RGB+Alpha, double buffered, Z, stencil, accum */
  {
    -1,                 /* vid */
    -1,                 /* class */
    True,               /* rgba */
    -1, -1, -1, 8,      /* rgba sizes */
    -1, -1, -1, -1,     /* rgba masks */
    16, 16, 16, 16,     /* rgba accum sizes */
    True,               /* doubleBuffer */
    False,              /* stereo */
    -1,                 /* bufferSize */
    16,                 /* depthSize */
    8,                  /* stencilSize */
    0,                  /* auxBuffers */
    0,                  /* level */
    GLX_NONE,           /* visualRating */
    GLX_NONE,           /* transparentPixel */
    0, 0, 0, 0,         /* transparent rgba color (floats scaled to ints) */
    0                   /* transparentIndex */
  },
  /* [3] = RGB+Alpha, single buffered, Z, stencil, accum */
  {
    -1,                 /* vid */
    -1,                 /* class */
    True,               /* rgba */
    -1, -1, -1, 8,      /* rgba sizes */
    -1, -1, -1, -1,     /* rgba masks */
    16, 16, 16, 16,     /* rgba accum sizes */
    False,              /* doubleBuffer */
    False,              /* stereo */
    -1,                 /* bufferSize */
    16,                 /* depthSize */
    8,                  /* stencilSize */
    0,                  /* auxBuffers */
    0,                  /* level */
    GLX_NONE,           /* visualRating */
    GLX_NONE,           /* transparentPixel */
    0, 0, 0, 0,         /* transparent rgba color (floats scaled to ints) */
    0                   /* transparentIndex */
  },
  /* [4] = CI, double buffered, Z */
  {
    -1,                 /* vid */
    -1,                 /* class */
    False,              /* rgba? (false = color index) */
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
    GLX_NONE,           /* visualRating */
    GLX_NONE,           /* transparentPixel */
    0, 0, 0, 0,         /* transparent rgba color (floats scaled to ints) */
    0                   /* transparentIndex */
  },
};

static __GLXvisualConfig NullConfig = {
    -1,                 /* vid */
    -1,                 /* class */
    False,              /* rgba */
    -1, -1, -1, 0,      /* rgba sizes */
    -1, -1, -1, 0,      /* rgba masks */
     0,  0,  0, 0,      /* rgba accum sizes */
    False,              /* doubleBuffer */
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
};


static inline int count_bits(uint32_t x)
{
    x = x - ((x >> 1) & 0x55555555);
    x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
    x = (x + (x >> 4)) & 0x0f0f0f0f;
    x = x + (x >> 8);
    x = x + (x >> 16);
    return x & 63;
}


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
    __GLcontextModes *modes;
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
        int count;

        count = ((pVisual[i].class == TrueColor ||
                  pVisual[i].class == DirectColor)
                ? numRGBconfigs : numCIconfigs);
        if (count == 0)
            count = 1;          /* preserve the existing visual */

        numNewVisuals += count;
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
    modes = _gl_context_modes_create(numNewVisuals, sizeof(__GLcontextModes));
    if (modes == NULL) {
        __glXFree(orig_vid);
        __glXFree(pNewVisualPriv);
        __glXFree(pNewVisualConfigs);
        return FALSE;
    }

    /* Alloc space for the list of glXVisualPrivates */
    glXVisualPriv = (void **)__glXMalloc(numNewVisuals * sizeof(void *));
    if (!glXVisualPriv) {
        _gl_context_modes_destroy( modes );
        __glXFree(orig_vid);
        __glXFree(pNewVisualPriv);
        __glXFree(pNewVisualConfigs);
        return FALSE;
    }

    /* Alloc space for the new list of the X server's visuals */
    pVisualNew = (VisualPtr)__glXMalloc(numNewVisuals * sizeof(VisualRec));
    if (!pVisualNew) {
        __glXFree(glXVisualPriv);
        _gl_context_modes_destroy( modes );
        __glXFree(orig_vid);
        __glXFree(pNewVisualPriv);
        __glXFree(pNewVisualConfigs);
        return FALSE;
    }

    /* Initialize the new visuals */
    found_default = FALSE;
    glAquaScreens[screenInfo.numScreens-1].modes = modes;
    for (i = j = 0; i < numVisuals; i++) {
        int is_rgb = (pVisual[i].class == TrueColor ||
                      pVisual[i].class == DirectColor);

        if (!is_rgb)
        {
            /* We don't support non-rgb visuals for GL. But we don't
               want to remove them either, so just pass them through
               with null glX configs */

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
            _gl_copy_visual_to_context_mode( modes, & NullConfig );
            modes->visualID = pVisualNew[j].vid;

            j++;

            continue;
        }

        for (k = 0; k < numNewConfigs; k++) {
            if (pNewVisualConfigs[k].rgba != is_rgb)
                continue;

            assert( modes != NULL );

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
            _gl_copy_visual_to_context_mode( modes, & pNewVisualConfigs[k] );
            modes->visualID = pVisualNew[j].vid;

            /*
             * If the class is -1, then assume the X visual information
             * is identical to what GLX needs, and take them from the X
             * visual.  NOTE: if class != -1, then all other fields MUST
             * be initialized.
             */
            if (modes->visualType == GLX_NONE) {
                modes->visualType = _gl_convert_from_x_visual_type( pVisual[i].class );
                modes->redBits    = count_bits(pVisual[i].redMask);
                modes->greenBits  = count_bits(pVisual[i].greenMask);
                modes->blueBits   = count_bits(pVisual[i].blueMask);
                modes->alphaBits  = modes->alphaBits;
                modes->redMask    = pVisual[i].redMask;
                modes->greenMask  = pVisual[i].greenMask;
                modes->blueMask   = pVisual[i].blueMask;
                modes->alphaMask  = modes->alphaMask;
                modes->rgbBits = (is_rgb)
                    ? (modes->redBits + modes->greenBits +
                       modes->blueBits + modes->alphaBits)
                    : rootDepth;
            }

            /* Save the device-dependent private for this visual */
            glXVisualPriv[j] = pNewVisualPriv[k];

            j++;
            modes = modes->next;
        }
    }

    assert(j <= numNewVisuals);

    /* Save the GLX visuals in the screen structure */
    glAquaScreens[screenInfo.numScreens-1].num_vis = numNewVisuals;
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

/* based on code in i830_dri.c
   This ends calling glAquaSetVisualConfigs to set the static
   numconfigs, etc. */
static void
glAquaInitVisualConfigs(void)
{
    int                 lclNumConfigs     = 0;
    __GLXvisualConfig  *lclVisualConfigs  = NULL;
    void              **lclVisualPrivates = NULL;

    int depth, aux, buffers, stencil, accum;
    int i = 0;

    GLAQUA_DEBUG_MSG("glAquaInitVisualConfigs ");
        
    /* count num configs:
        2 Z buffer (0, 24 bit)
        2 AUX buffer (0, 2)
        2 buffers (single, double)
        2 stencil (0, 8 bit)
        2 accum (0, 64 bit)
        = 32 configs */

    lclNumConfigs = 2 * 2 * 2 * 2 * 2; /* 32 */

    /* alloc */
    lclVisualConfigs = xcalloc(sizeof(__GLXvisualConfig), lclNumConfigs);
    lclVisualPrivates = xcalloc(sizeof(void *), lclNumConfigs);

    /* fill in configs */
    if (NULL != lclVisualConfigs) {
        i = 0; /* current buffer */
        for (depth = 0; depth < 2; depth++) {
            for (aux = 0; aux < 2; aux++) {
                for (buffers = 0; buffers < 2; buffers++) {
                    for (stencil = 0; stencil < 2; stencil++) {
                        for (accum = 0; accum < 2; accum++) {
                            lclVisualConfigs[i].vid = -1;
                            lclVisualConfigs[i].class = -1;
                            lclVisualConfigs[i].rgba = TRUE;
                            lclVisualConfigs[i].redSize = -1;
                            lclVisualConfigs[i].greenSize = -1;
                            lclVisualConfigs[i].blueSize = -1;
                            lclVisualConfigs[i].redMask = -1;
                            lclVisualConfigs[i].greenMask = -1;
                            lclVisualConfigs[i].blueMask = -1;
                            lclVisualConfigs[i].alphaMask = 0;
                            if (accum) {
                                lclVisualConfigs[i].accumRedSize = 16;
                                lclVisualConfigs[i].accumGreenSize = 16;
                                lclVisualConfigs[i].accumBlueSize = 16;
                                lclVisualConfigs[i].accumAlphaSize = 16;
                            }
                            else {
                                lclVisualConfigs[i].accumRedSize = 0;
                                lclVisualConfigs[i].accumGreenSize = 0;
                                lclVisualConfigs[i].accumBlueSize = 0;
                                lclVisualConfigs[i].accumAlphaSize = 0;
                            }
                            lclVisualConfigs[i].doubleBuffer = buffers ? TRUE : FALSE;
                            lclVisualConfigs[i].stereo = FALSE;
                            lclVisualConfigs[i].bufferSize = -1;

                            lclVisualConfigs[i].depthSize = depth? 24 : 0;
                            lclVisualConfigs[i].stencilSize = stencil ? 8 : 0;
                            lclVisualConfigs[i].auxBuffers = aux ? 2 : 0;
                            lclVisualConfigs[i].level = 0;
                            lclVisualConfigs[i].visualRating = GLX_NONE_EXT;
                            lclVisualConfigs[i].transparentPixel = 0;
                            lclVisualConfigs[i].transparentRed = 0;
                            lclVisualConfigs[i].transparentGreen = 0;
                            lclVisualConfigs[i].transparentBlue = 0;
                            lclVisualConfigs[i].transparentAlpha = 0;
                            lclVisualConfigs[i].transparentIndex = 0;
                            i++;
                        }
                    }
                }
            }
        }
    }
    if (i != lclNumConfigs)
        GLAQUA_DEBUG_MSG("glAquaInitVisualConfigs failed to alloc visual configs");

    GlxSetVisualConfigs(lclNumConfigs, lclVisualConfigs, lclVisualPrivates);
}


static void glAquaSetVisualConfigs(int nconfigs, __GLXvisualConfig *configs,
                                   void **privates)
{
    GLAQUA_DEBUG_MSG("glAquaSetVisualConfigs\n");

    numConfigs = nconfigs;
    visualConfigs = configs;
    visualPrivates = privates;
}

static Bool glAquaInitVisuals(VisualPtr *visualp, DepthPtr *depthp,
                              int *nvisualp, int *ndepthp,
                              int *rootDepthp, VisualID *defaultVisp,
                              unsigned long sizes, int bitsPerRGB)
{
    GLAQUA_DEBUG_MSG("glAquaInitVisuals\n");
    
    if (numConfigs == 0) /* if no configs */
        glAquaInitVisualConfigs(); /* ensure the visual configs are setup */

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
    int j;
    __GLcontextModes *modes;

    GLAQUA_DEBUG_MSG("fixup_visuals\n");

    for ( modes = pScr->modes ; modes != NULL ; modes = modes->next ) {
        const int vis_class = _gl_convert_to_x_visual_type( modes->visualType );
        const int nplanes = (modes->rgbBits - modes->alphaBits);
        const VisualPtr pVis = pScreen->visuals;

        /* Find a visual that matches the GLX visual's class and size */
        for (j = 0; j < pScreen->numVisuals; j++) {
            if (pVis[j].class == vis_class &&
            pVis[j].nplanes == nplanes) {

            /* Fixup the masks */
            modes->redMask   = pVis[j].redMask;
            modes->greenMask = pVis[j].greenMask;
            modes->blueMask  = pVis[j].blueMask;

            /* Recalc the sizes */
            modes->redBits   = count_bits(modes->redMask);
            modes->greenBits = count_bits(modes->greenMask);
            modes->blueBits  = count_bits(modes->blueMask);
            }
        }
    }
}

static void init_screen_visuals(int screen)
{
    ScreenPtr pScreen = screenInfo.screens[screen];
    __GLcontextModes *modes;
    int *used;
    int i, j;

    GLAQUA_DEBUG_MSG("init_screen_visuals\n");

    /* FIXME: Change 'used' to be a array of bits (rather than of ints),
     * FIXME: create a stack array of 8 or 16 bytes.  If 'numVisuals' is less
     * FIXME: than 64 or 128 the stack array can be used instead of calling
     * FIXME: __glXMalloc / __glXFree.  If nothing else, convert 'used' to
     * FIXME: array of bytes instead of ints!
     */
    used = (int *)__glXMalloc(pScreen->numVisuals * sizeof(int));
    __glXMemset(used, 0, pScreen->numVisuals * sizeof(int));

    i = 0;
    for ( modes = glAquaScreens[screen].modes 
          ; modes != NULL
          ; modes = modes->next ) {
        const int vis_class = _gl_convert_to_x_visual_type( modes->visualType );
        const int nplanes = (modes->rgbBits - modes->alphaBits);
        const VisualPtr pVis = pScreen->visuals;

        for (j = 0; j < pScreen->numVisuals; j++) {
            if (pVis[j].class     == vis_class &&
                pVis[j].nplanes   == nplanes &&
                pVis[j].redMask   == modes->redMask &&
                pVis[j].greenMask == modes->greenMask &&
                pVis[j].blueMask  == modes->blueMask &&
                !used[j]) {

                    /* Set the VisualID */
                    modes->visualID = pVis[j].vid;

                    /* Mark this visual used */
                    used[j] = 1;
                    break;
            }
        }
        if ( j == pScreen->numVisuals ) {
            ErrorF("No matching visual for __GLcontextMode with "
                   "visual class = %d (%d), nplanes = %u\n",
                   vis_class, 
                   (int)modes->visualType,
                   (unsigned int)(modes->rgbBits - modes->alphaBits) );
        }
        else if ( modes->visualID == -1 ) {
            FatalError( "Matching visual found, but visualID still -1!\n" );
        }

        i++;
    }

    __glXFree(used);
}

static Bool glAquaScreenProbe(int screen)
{
    ScreenPtr pScreen;
    glAquaScreenRec *screenPriv;

    GLAQUA_DEBUG_MSG("glAquaScreenProbe\n");

    /*
     * Set up the current screen's visuals.
     */
    __glDDXScreenInfo.modes = glAquaScreens[screen].modes;
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

    /*
     * Wrap RealizeWindow and UnrealizeWindow on this screen
     */
    pScreen = screenInfo.screens[screen];
    screenPriv = &glAquaScreens[screen];
    screenPriv->RealizeWindow = pScreen->RealizeWindow;
    pScreen->RealizeWindow = glAquaRealizeWindow;
    screenPriv->UnrealizeWindow = pScreen->UnrealizeWindow;
    pScreen->UnrealizeWindow = glAquaUnrealizeWindow;

    return TRUE;
}

static GLboolean glAquaSwapBuffers(__GLXdrawablePrivate *glxPriv)
{
    // swap buffers on only *one* of the contexts
    // (e.g. the last one for drawing)
    __GLcontext *gc = (__GLcontext *)glxPriv->drawGlxc->gc;
    CGLError gl_err;

    GLAQUA_DEBUG_MSG("glAquaSwapBuffers\n");

    if (gc != NULL && gc->ctx != NULL)
    {
        gl_err = CGLFlushDrawable(gc->ctx);
        if (gl_err != 0)
            ErrorF("CGLFlushDrawable error: %s\n", CGLErrorString(gl_err));
    }

    return GL_TRUE;
}

static void glAquaDestroyDrawablePrivate(__GLdrawablePrivate *glPriv)
{
    GLAQUA_DEBUG_MSG("glAquaDestroyDrawablePrivate\n");

    /* It doesn't work to call DRIDestroySurface here, the drawable's
       already gone.. But dri.c notices the window destruction and
       frees the surface itself. */

    free(glPriv->private);
    glPriv->private = NULL;
}

static void glAquaCreateBuffer(__GLXdrawablePrivate *glxPriv)
{
    GLAquaDrawableRec *aquaPriv = malloc(sizeof(GLAquaDrawableRec));
    __GLdrawablePrivate *glPriv = &glxPriv->glPriv;

    aquaPriv->sid = 0;
    aquaPriv->pDraw = NULL;

    GLAQUA_DEBUG_MSG("glAquaCreateBuffer\n");

    // replace swapBuffers (original is never called)
    glxPriv->swapBuffers = glAquaSwapBuffers;

    // stash private data
    glPriv->private = aquaPriv;
    glPriv->freePrivate = glAquaDestroyDrawablePrivate;
}

static void glAquaResetExtension(void)
{
    GLAQUA_DEBUG_MSG("glAquaResetExtension\n");
    CGLSetOption(kCGLGOResetLibrary, GL_TRUE);
}

// Extra goodies for glx

GLuint __glFloorLog2(GLuint val)
{
    int c = 0;

    while (val > 1) {
        c++;
        val >>= 1;
    }
    return c;
}
