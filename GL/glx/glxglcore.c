/**************************************************************************

Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Kevin E. Martin <kevin@precisioninsight.com>
 *   Brian E. Paul <brian@precisioninsight.com>
 *
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <string.h>

#include <GL/xmesa.h>
#include <GL/internal/glcore.h>
#include <glxserver.h>
#include <glxscreens.h>
#include <glxdrawable.h>
#include <glxcontext.h>
#include <glxutil.h>

#include "glcontextmodes.h"
#include "os.h"

typedef struct __GLXMESAscreen   __GLXMESAscreen;
typedef struct __GLXMESAcontext  __GLXMESAcontext;
typedef struct __GLXMESAdrawable __GLXMESAdrawable;

struct __GLXMESAscreen {
    __GLXscreen   base;
    int           index;
    int		  num_vis;
    XMesaVisual  *xm_vis;
};

struct __GLXMESAcontext {
    __GLXcontext base;
    XMesaContext xmesa;
};

struct __GLXMESAdrawable {
    __GLXdrawable base;
    XMesaBuffer   xm_buf;
};

static XMesaVisual find_mesa_visual(__GLXscreen *screen, XID fbconfigID);


static void
__glXMesaDrawableDestroy(__GLXdrawable *base)
{
    __GLXMESAdrawable *glxPriv = (__GLXMESAdrawable *) base;

    if (glxPriv->xm_buf != NULL)
      XMesaDestroyBuffer(glxPriv->xm_buf);
    xfree(glxPriv);
}

static GLboolean
__glXMesaDrawableResize(__GLXdrawable *base)
{
    __GLXMESAdrawable *glxPriv = (__GLXMESAdrawable *) base;

    XMesaResizeBuffers(glxPriv->xm_buf);

    return GL_TRUE;
}

static GLboolean
__glXMesaDrawableSwapBuffers(__GLXdrawable *base)
{
    __GLXMESAdrawable *glxPriv = (__GLXMESAdrawable *) base;

    /* This is terrifying: XMesaSwapBuffers() ends up calling CopyArea
     * to do the buffer swap, but this assumes that the server holds
     * the lock and has its context visible.  If another screen uses a
     * DRI driver, that will have installed the DRI enter/leave server
     * functions, which lifts the lock during GLX dispatch.  This is
     * why we need to re-take the lock and swap in the server context
     * before calling XMesaSwapBuffers() here.  /me shakes head. */

    __glXenterServer(GL_FALSE);

    XMesaSwapBuffers(glxPriv->xm_buf);

    __glXleaveServer(GL_FALSE);

    return GL_TRUE;
}


static __GLXdrawable *
__glXMesaScreenCreateDrawable(__GLXscreen *screen,
			      DrawablePtr pDraw, int type,
			      XID drawId,
			      __GLcontextModes *modes)
{
    __GLXMESAdrawable *glxPriv;
    XMesaVisual xm_vis;

    glxPriv = xalloc(sizeof *glxPriv);
    if (glxPriv == NULL)
	return NULL;

    memset(glxPriv, 0, sizeof *glxPriv);

    if (!__glXDrawableInit(&glxPriv->base, screen,
			   pDraw, type, drawId, modes)) {
        xfree(glxPriv);
	return NULL;
    }

    glxPriv->base.destroy     = __glXMesaDrawableDestroy;
    glxPriv->base.resize      = __glXMesaDrawableResize;
    glxPriv->base.swapBuffers = __glXMesaDrawableSwapBuffers;

    xm_vis = find_mesa_visual(screen, modes->fbconfigID);
    if (xm_vis == NULL) {
	ErrorF("find_mesa_visual returned NULL for visualID = 0x%04x\n",
	       modes->visualID);
	xfree(glxPriv);
	return NULL;
    }

    if (glxPriv->base.type == DRAWABLE_WINDOW) {
	glxPriv->xm_buf = XMesaCreateWindowBuffer(xm_vis, (WindowPtr)pDraw);
    } else {
	glxPriv->xm_buf = XMesaCreatePixmapBuffer(xm_vis, (PixmapPtr)pDraw, 0);
    }

    if (glxPriv->xm_buf == NULL) {
	xfree(glxPriv);
	return NULL;
    }

    return &glxPriv->base;
}

static void
__glXMesaContextDestroy(__GLXcontext *baseContext)
{
    __GLXMESAcontext *context = (__GLXMESAcontext *) baseContext;

    XMesaDestroyContext(context->xmesa);
    __glXContextDestroy(&context->base);
    xfree(context);
}

static int
__glXMesaContextMakeCurrent(__GLXcontext *baseContext)

{
    __GLXMESAcontext *context = (__GLXMESAcontext *) baseContext;
    __GLXMESAdrawable *drawPriv = (__GLXMESAdrawable *) context->base.drawPriv;
    __GLXMESAdrawable *readPriv = (__GLXMESAdrawable *) context->base.readPriv;

    return XMesaMakeCurrent2(context->xmesa,
			     drawPriv->xm_buf,
			     readPriv->xm_buf);
}

static int
__glXMesaContextLoseCurrent(__GLXcontext *baseContext)
{
    __GLXMESAcontext *context = (__GLXMESAcontext *) baseContext;

    return XMesaLoseCurrent(context->xmesa);
}

static int
__glXMesaContextCopy(__GLXcontext *baseDst,
		     __GLXcontext *baseSrc,
		     unsigned long mask)
{
    __GLXMESAcontext *dst = (__GLXMESAcontext *) baseDst;
    __GLXMESAcontext *src = (__GLXMESAcontext *) baseSrc;

    return XMesaCopyContext(src->xmesa, dst->xmesa, mask);
}

static int
__glXMesaContextForceCurrent(__GLXcontext *baseContext)
{
    __GLXMESAcontext *context = (__GLXMESAcontext *) baseContext;

    /* GlxSetRenderTables() call for XGL moved in XMesaForceCurrent() */

    return XMesaForceCurrent(context->xmesa);
}

static __GLXcontext *
__glXMesaScreenCreateContext(__GLXscreen *screen,
			     __GLcontextModes *modes,
			     __GLXcontext *baseShareContext)
{
    __GLXMESAcontext *context;
    __GLXMESAcontext *shareContext = (__GLXMESAcontext *) baseShareContext;
    XMesaVisual xm_vis;
    XMesaContext xm_share;

    context = xalloc (sizeof (__GLXMESAcontext));
    if (context == NULL)
	return NULL;

    memset(context, 0, sizeof *context);

    context->base.pGlxScreen = screen;
    context->base.modes      = modes;

    context->base.destroy        = __glXMesaContextDestroy;
    context->base.makeCurrent    = __glXMesaContextMakeCurrent;
    context->base.loseCurrent    = __glXMesaContextLoseCurrent;
    context->base.copy           = __glXMesaContextCopy;
    context->base.forceCurrent   = __glXMesaContextForceCurrent;

    xm_vis = find_mesa_visual(screen, modes->fbconfigID);
    if (!xm_vis) {
	ErrorF("find_mesa_visual returned NULL for visualID = 0x%04x\n",
	       modes->visualID);
	xfree(context);
	return NULL;
    }

    xm_share = shareContext ? shareContext->xmesa : NULL;
    context->xmesa = XMesaCreateContext(xm_vis, xm_share);
    if (!context->xmesa) {
	xfree(context);
	return NULL;
    }

    return &context->base;
}

static void
__glXMesaScreenDestroy(__GLXscreen *screen)
{
    __GLXMESAscreen *mesaScreen = (__GLXMESAscreen *) screen;
    int i;

    if (mesaScreen->xm_vis) {
	for (i = 0; i < mesaScreen->num_vis; i++) {
	    if (mesaScreen->xm_vis[i])
		XMesaDestroyVisual(mesaScreen->xm_vis[i]);
	}

	xfree(mesaScreen->xm_vis);
    }

    __glXScreenDestroy(screen);

    xfree(screen);
}

static XMesaVisual
find_mesa_visual(__GLXscreen *screen, XID fbconfigID)
{
    __GLXMESAscreen *mesaScreen = (__GLXMESAscreen *) screen;
    const __GLcontextModes *modes;
    unsigned i = 0;

    for (modes = screen->fbconfigs; modes != NULL; modes = modes->next) {
 	if (modes->fbconfigID == fbconfigID)
	    return mesaScreen->xm_vis[i];
	i++;
    }

    return NULL;
}

const static int numBack = 2;
const static int numDepth = 2;
const static int numStencil = 2;

static __GLcontextModes *
createFBConfigsForVisual(__GLXscreen *screen, ScreenPtr pScreen,
			 VisualPtr visual, __GLcontextModes *config)
{
    int back, depth, stencil;

    /* FIXME: Ok, I'm making all this up... anybody has a better idea? */

    for (back = numBack - 1; back >= 0; back--)
	for (depth = 0; depth < numDepth; depth++)
	    for (stencil = 0; stencil < numStencil; stencil++) {
		config->visualType = _gl_convert_from_x_visual_type(visual->class);
		config->xRenderable = GL_TRUE;
		config->drawableType = GLX_WINDOW_BIT | GLX_PIXMAP_BIT;
		config->rgbMode = (visual->class >= TrueColor);
		config->colorIndexMode = !config->rgbMode;
		config->renderType =
		    (config->rgbMode) ? GLX_RGBA_BIT : GLX_COLOR_INDEX_BIT;
		config->doubleBufferMode = back;
		config->haveDepthBuffer = depth;
		config->depthBits = depth ? visual->nplanes : 0;
		config->haveStencilBuffer = stencil;
		config->stencilBits = stencil ? visual->bitsPerRGBValue : 0;
		config->haveAccumBuffer = 0;

		config->redBits = Ones(visual->redMask);
		config->greenBits = Ones(visual->greenMask);
		config->blueBits = Ones(visual->blueMask);
		config->alphaBits = 0;
		config->redMask = visual->redMask;
		config->greenMask = visual->greenMask;
		config->blueMask = visual->blueMask;
		config->alphaMask = 0;
		config->rgbBits = config->rgbMode ? visual->nplanes : 0;
		config->indexBits = config->colorIndexMode ? visual->nplanes : 0;
		config = config->next;
	    }

    return config;
}

static void
createFBConfigs(__GLXscreen *pGlxScreen, ScreenPtr pScreen)
{
    __GLcontextModes *configs;
    int i;

    /* We assume here that each existing visual correspond to a
     * different visual class.  Note, this runs before COMPOSITE adds
     * its visual, so it's not entirely crazy. */
    pGlxScreen->numFBConfigs = pScreen->numVisuals * numBack * numDepth * numStencil;
    pGlxScreen->fbconfigs = _gl_context_modes_create(pGlxScreen->numFBConfigs,
						     sizeof *configs);

    configs = pGlxScreen->fbconfigs;
    for (i = 0; i < pScreen->numVisuals; i++)
	configs = createFBConfigsForVisual(pGlxScreen, pScreen,
					   &pScreen->visuals[i], configs);
}

static void
createMesaVisuals(__GLXMESAscreen *pMesaScreen)
{
    __GLcontextModes *config;
    ScreenPtr pScreen;
    VisualPtr visual;
    int i, j;

    i = 0;
    pScreen = pMesaScreen->base.pScreen;
    pMesaScreen->xm_vis =
	xcalloc(pMesaScreen->base.numFBConfigs, sizeof (XMesaVisual));
    for (config = pMesaScreen->base.fbconfigs; config != NULL; config = config->next) {
	for (j = 0; j < pScreen->numVisuals; j++)
	    if (pScreen->visuals[j].vid == config->visualID) {
		visual = &pScreen->visuals[j];
		break;
	    }

	pMesaScreen->xm_vis[i++] =
	    XMesaCreateVisual(pScreen,
			      visual,
			      config->rgbMode,
			      (config->alphaBits > 0),
			      config->doubleBufferMode,
			      config->stereoMode,
			      GL_TRUE, /* ximage_flag */
			      config->depthBits,
			      config->stencilBits,
			      config->accumRedBits,
			      config->accumGreenBits,
			      config->accumBlueBits,
			      config->accumAlphaBits,
			      config->samples,
			      config->level,
			      config->visualRating);
    }
}

static __GLXscreen *
__glXMesaScreenProbe(ScreenPtr pScreen)
{
    __GLXMESAscreen *screen;

    screen = xalloc(sizeof *screen);
    if (screen == NULL)
	return NULL;

    /*
     * Find the GLX visuals that are supported by this screen and create
     * XMesa's visuals.
     */
    createFBConfigs(&screen->base, pScreen);

    __glXScreenInit(&screen->base, pScreen);

    /* Now that GLX has created the corresponding X visual, create the mesa visuals. */
    createMesaVisuals(screen);

    screen->base.destroy        = __glXMesaScreenDestroy;
    screen->base.createContext  = __glXMesaScreenCreateContext;
    screen->base.createDrawable = __glXMesaScreenCreateDrawable;
    screen->base.pScreen       = pScreen;

    return &screen->base;
}

__GLXprovider __glXMesaProvider = {
    __glXMesaScreenProbe,
    "MESA",
    NULL
};

__GLXprovider *
GlxGetMesaProvider (void)
{
    return &__glXMesaProvider;
}
