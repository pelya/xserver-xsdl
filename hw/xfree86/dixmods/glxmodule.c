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
 *
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "xf86Module.h"
#include "xf86Priv.h"
#include "xf86.h"
#include "colormap.h"
#include "micmap.h"
#include "globals.h"

typedef struct __GLXscreen __GLXscreen;
typedef struct __GLXprovider __GLXprovider;
struct __GLXprovider {
    __GLXscreen *(*screenProbe)(ScreenPtr pScreen);
    const char    *name;
    __GLXprovider *next;
};

extern void GlxPushProvider(__GLXprovider *provider);
extern void GlxExtensionInit(void);
extern void GlxWrapInitVisuals(miInitVisualsProcPtr *);

static MODULESETUPPROTO(glxSetup);

static const char *initdeps[] = { "DOUBLE-BUFFER", NULL };

static ExtensionModule GLXExt =
{
    GlxExtensionInit,
    "GLX",
    &noGlxExtension,
    NULL,
    initdeps
};

static XF86ModuleVersionInfo VersRec =
{
        "glx",
        MODULEVENDORSTRING,
        MODINFOSTRING1,
        MODINFOSTRING2,
        XORG_VERSION_CURRENT,
        1, 0, 0,
        ABI_CLASS_EXTENSION,
        ABI_EXTENSION_VERSION,
        MOD_CLASS_NONE,
        {0,0,0,0}
};

_X_EXPORT XF86ModuleData glxModuleData = { &VersRec, glxSetup, NULL };

/* We do a little proxy dance here, so we can avoid loading GLcore
 * unless we really need to.*/

static pointer glxModule;

static __GLXscreen *
__glXMesaProxyScreenProbe(ScreenPtr pScreen)
{
  pointer GLcore;
  static __GLXprovider *provider;

  if (provider == NULL) {
    GLcore = LoadSubModule(glxModule, "GLcore", NULL, NULL, NULL, NULL, 
			   NULL, NULL);
    if (GLcore == NULL)
      return NULL;

    provider = LoaderSymbol("__glXMesaProvider");
    if (provider == NULL)
      return NULL;
  }

  return provider->screenProbe(pScreen);
}

static __GLXprovider __glXMesaProxyProvider = {
    __glXMesaProxyScreenProbe,
    "MESA-PROXY",
    NULL
};


static pointer
glxSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
    static Bool setupDone = FALSE;
    __GLXprovider *provider;

    if (setupDone) {
	if (errmaj) *errmaj = LDR_ONCEONLY;
	return NULL;
    } 

    setupDone = TRUE;

    glxModule = module;
    GlxPushProvider(&__glXMesaProxyProvider);

    xf86Msg(xf86Info.aiglxFrom, "AIGLX %s\n", 
	    xf86Info.aiglx ? "enabled" : "disabled");
    if (xf86Info.aiglx) {
      provider = LoaderSymbol("__glXDRIProvider");
      if (provider)
	GlxPushProvider(provider);
    }

    LoadExtension(&GLXExt, FALSE);
    /* Wrap the init visuals routine in micmap.c */
    GlxWrapInitVisuals(&miInitVisualsProc);
    /* Make sure this gets wrapped each time InitVisualWrap is called */
    miHookInitVisuals(NULL, GlxWrapInitVisuals);

 bail:
    return module;
}
