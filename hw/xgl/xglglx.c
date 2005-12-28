/*
 * Copyright © 2005 Novell, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Novell, Inc. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Novell, Inc. makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * NOVELL, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL NOVELL, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: David Reveman <davidr@novell.com>
 */

#include "xgl.h"

#ifdef GLXEXT

#include "glxserver.h"
#include "glxscreens.h"
#include "glxext.h"

typedef struct _xglGLXFunc {
    void (*extensionInit)     (void);
    void (*setVisualConfigs)  (int		    nconfigs,
			       __GLXvisualConfig    *configs,
			       void                 **privates);
    void (*wrapInitVisuals)   (miInitVisualsProcPtr *initVisuals);
    int  (*initVisuals)	      (VisualPtr	    *visualp,
			       DepthPtr		    *depthp,
			       int		    *nvisualp,
			       int		    *ndepthp,
			       int		    *rootDepthp,
			       VisualID		    *defaultVisp,
			       unsigned long	    sizes,
			       int		    bitsPerRGB,
			       int		    preferredVis);

    void (*flushContextCache) (void);
    void (*setRenderTables)   (__glProcTable	    *table,
			       __glProcTableEXT	    *tableEXT);
} xglGLXFuncRec;

static xglGLXFuncRec __glXFunc;

static void *glXHandle = 0;
static void *glCoreHandle = 0;

#define SYM(ptr, name) { (void **) &(ptr), (name) }

__GLXextensionInfo __glDDXExtensionInfo = {
    GL_CORE_MESA,
    NULL,
    NULL,
    NULL
};

__GLXscreenInfo __glDDXScreenInfo = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    0,
    "Vendor String",
    "Version String",
    "Extensions String",
    NULL
};

void
GlxSetVisualConfigs (int	       nconfigs,
		     __GLXvisualConfig *configs,
		     void              **privates)
{
    if (glXHandle && glCoreHandle)
	(*__glXFunc.setVisualConfigs) (nconfigs, configs, privates);
}

void
GlxExtensionInit (void)
{
    if (glXHandle && glCoreHandle)
	(*__glXFunc.extensionInit) ();
}

void
GlxWrapInitVisuals (miInitVisualsProcPtr *initVisuals)
{
    if (glXHandle && glCoreHandle)
	(*__glXFunc.wrapInitVisuals) (initVisuals);
}

int
GlxInitVisuals (VisualPtr     *visualp,
		DepthPtr      *depthp,
		int	      *nvisualp,
		int	      *ndepthp,
		int	      *rootDepthp,
		VisualID      *defaultVisp,
		unsigned long sizes,
		int	      bitsPerRGB,
		int	      preferredVis)
{
    if (glXHandle && glCoreHandle)
	return (*__glXFunc.initVisuals) (visualp, depthp, nvisualp, ndepthp,
					 rootDepthp, defaultVisp, sizes,
					 bitsPerRGB, preferredVis);

    return 0;
}

void
GlxFlushContextCache (void)
{
    (*__glXFunc.flushContextCache) ();
}

void
GlxSetRenderTables (__glProcTable    *table,
		    __glProcTableEXT *tableEXT)
{
    (*__glXFunc.setRenderTables) (table, tableEXT);
}

Bool
xglLoadGLXModules (void)
{

#ifdef XGL_MODULAR
    if (!glXHandle)
    {
	xglSymbolRec sym[] = {
	    SYM (__glXFunc.extensionInit,     "GlxExtensionInit"),
	    SYM (__glXFunc.setVisualConfigs,  "GlxSetVisualConfigs"),
	    SYM (__glXFunc.wrapInitVisuals,   "GlxWrapInitVisuals"),
	    SYM (__glXFunc.initVisuals,	      "GlxInitVisuals"),
	    SYM (__glXFunc.flushContextCache, "GlxFlushContextCache"),
	    SYM (__glXFunc.setRenderTables,   "GlxSetRenderTables")
	};

	glXHandle = xglLoadModule ("glx");
	if (!glXHandle)
	    return FALSE;

	if (!xglLookupSymbols (glXHandle, sym, sizeof (sym) / sizeof (sym[0])))
	{
	    xglUnloadModule (glXHandle);
	    glXHandle = 0;

	    return FALSE;
	}
    }

    if (!glCoreHandle)
    {
	xglSymbolRec sym[] = {
	    SYM (__glDDXScreenInfo.screenProbe,   "__MESA_screenProbe"),
	    SYM (__glDDXScreenInfo.createContext, "__MESA_createContext"),
	    SYM (__glDDXScreenInfo.createBuffer,  "__MESA_createBuffer"),

	    SYM (__glDDXExtensionInfo.resetExtension,   "__MESA_resetExtension"),
	    SYM (__glDDXExtensionInfo.initVisuals,      "__MESA_initVisuals"),
	    SYM (__glDDXExtensionInfo.setVisualConfigs,
		 "__MESA_setVisualConfigs")
	};

	glCoreHandle = xglLoadModule ("glcore");
	if (!glCoreHandle)
	    return FALSE;

	if (!xglLookupSymbols (glCoreHandle, sym, sizeof (sym) / sizeof (sym[0])))
	{
	    xglUnloadModule (glCoreHandle);
	    glCoreHandle = 0;

	    return FALSE;
	}

	if (!xglLoadHashFuncs (glCoreHandle))
	{
	    xglUnloadModule (glCoreHandle);
	    glCoreHandle = 0;
	}
    }

    return TRUE;
#else
    return FALSE;
#endif

}

void
xglUnloadGLXModules (void)
{

#ifdef XGL_MODULAR
    if (glXHandle)
    {
	xglUnloadModule (glXHandle);
	glXHandle = 0;
    }

    if (glCoreHandle)
    {
	xglUnloadModule (glCoreHandle);
	glCoreHandle = 0;
    }
#endif

}

#endif
