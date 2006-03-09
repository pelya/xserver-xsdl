/*
 * Copyright © 2006 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <anholt@FreeBSD.org>
 *
 */

#ifdef HAVE_CONFIG_H
#include <xorg-config.h>
#endif

#include "exa_priv.h"

#include "xf86str.h"
#include "xf86.h"

typedef struct _ExaXorgScreenPrivRec {
    CloseScreenProcPtr 		 SavedCloseScreen;
    EnableDisableFBAccessProcPtr SavedEnableDisableFBAccess;
} ExaXorgScreenPrivRec, *ExaXorgScreenPrivPtr;

static int exaXorgServerGeneration;
static int exaXorgScreenPrivateIndex;

static Bool
exaXorgCloseScreen (int i, ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = XF86SCRNINFO(pScreen);
    ExaXorgScreenPrivPtr pScreenPriv =
	pScreen->devPrivates[exaXorgScreenPrivateIndex].ptr;

    pScreen->CloseScreen = pScreenPriv->SavedCloseScreen;

    pScrn->EnableDisableFBAccess = pScreenPriv->SavedEnableDisableFBAccess;

    xfree (pScreenPriv);

    return pScreen->CloseScreen (i, pScreen);
}

static void
exaXorgEnableDisableFBAccess (int index, Bool enable)
{
    ScreenPtr pScreen = screenInfo.screens[index];
    ExaXorgScreenPrivPtr pScreenPriv =
	pScreen->devPrivates[exaXorgScreenPrivateIndex].ptr;

    if (!enable)
	exaEnableDisableFBAccess (index, enable);

    if (pScreenPriv->SavedEnableDisableFBAccess)
       pScreenPriv->SavedEnableDisableFBAccess (index, enable);

    if (enable)
	exaEnableDisableFBAccess (index, enable);
}

/**
 * This will be called during exaDriverInit, giving us the chance to set options
 * and hook in our EnableDisableFBAccess.
 */
void
exaDDXDriverInit(ScreenPtr pScreen)
{
    /* Do NOT use XF86SCRNINFO macro here!! */
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    ExaXorgScreenPrivPtr pScreenPriv;

    if (exaXorgServerGeneration != serverGeneration) {
	exaXorgScreenPrivateIndex = AllocateScreenPrivateIndex();
	exaXorgServerGeneration = serverGeneration;
    }

    pScreenPriv = xcalloc (1, sizeof(ExaXorgScreenPrivRec));
    if (pScreenPriv == NULL)
	return;

    pScreen->devPrivates[exaXorgScreenPrivateIndex].ptr = pScreenPriv;

    pScreenPriv->SavedEnableDisableFBAccess = pScrn->EnableDisableFBAccess;
    pScrn->EnableDisableFBAccess = exaXorgEnableDisableFBAccess;
    
    pScreenPriv->SavedCloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = exaXorgCloseScreen;
    
}

static MODULESETUPPROTO(exaSetup);

static const OptionInfoRec EXAOptions[] = {
    { -1,				NULL,
      OPTV_NONE,	{0}, FALSE }
};

/*ARGSUSED*/
static const OptionInfoRec *
EXAAvailableOptions(void *unused)
{
    return (EXAOptions);
}

static XF86ModuleVersionInfo exaVersRec =
{
	"exa",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XORG_VERSION_CURRENT,
	EXA_VERSION_MAJOR, EXA_VERSION_MINOR, EXA_VERSION_RELEASE,
	ABI_CLASS_VIDEODRV,		/* requires the video driver ABI */
	ABI_VIDEODRV_VERSION,
	MOD_CLASS_NONE,
	{0,0,0,0}
};

XF86ModuleData exaModuleData = { &exaVersRec, exaSetup, NULL };

ModuleInfoRec EXA = {
    1,
    "EXA",
    NULL,
    0,
    EXAAvailableOptions,
};

/*ARGSUSED*/
static pointer
exaSetup(pointer Module, pointer Options, int *ErrorMajor, int *ErrorMinor)
{
    static Bool Initialised = FALSE;

    if (!Initialised) {
	Initialised = TRUE;
#ifndef REMOVE_LOADER_CHECK_MODULE_INFO
	if (xf86LoaderCheckSymbol("xf86AddModuleInfo"))
#endif
	xf86AddModuleInfo(&EXA, Module);
    }

    return (pointer)TRUE;
}
