/*
 * $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86RandR.c,v 1.4 2003/02/13 10:49:38 eich Exp $
 *
 * Copyright © 2002 Keith Packard, member of The XFree86 Project, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "X.h"
#include "os.h"
#include "mibank.h"
#include "globals.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86DDC.h"
#include "mipointer.h"
#include <randrstr.h>

typedef struct _xf86RandRInfo {
    CreateScreenResourcesProcPtr    CreateScreenResources;
    CloseScreenProcPtr		    CloseScreen;
    int				    virtualX;
    int				    virtualY;
} XF86RandRInfoRec, *XF86RandRInfoPtr;
    
static int	    xf86RandRIndex;
static int	    xf86RandRGeneration;

#define XF86RANDRINFO(p)    ((XF86RandRInfoPtr) (p)->devPrivates[xf86RandRIndex].ptr)

static int
xf86RandRModeRefresh (DisplayModePtr mode)
{
    if (mode->VRefresh)
	return (int) mode->VRefresh;
    else
	return (int) (mode->Clock * 1000.0 / mode->HTotal / mode->VTotal);
}

static Bool
xf86RandRGetInfo (ScreenPtr pScreen, Rotation *rotations)
{
    RRScreenSizePtr	    pSize;
    ScrnInfoPtr		    scrp = XF86SCRNINFO(pScreen);
    XF86RandRInfoPtr	    randrp = XF86RANDRINFO(pScreen);
    DisplayModePtr	    mode;
    Bool		    reportVirtual = TRUE;
    int			    refresh0 = 60;
    
    *rotations = RR_Rotate_0;

    for (mode = scrp->modes; ; mode = mode->next)
    {
	int refresh = xf86RandRModeRefresh (mode);
	if (mode == scrp->modes)
	    refresh0 = refresh;
	pSize = RRRegisterSize (pScreen,
				mode->HDisplay, mode->VDisplay,
				pScreen->mmWidth, pScreen->mmHeight);
	if (!pSize)
	    return FALSE;
	if (mode->HDisplay == randrp->virtualX && 
	    mode->VDisplay == randrp->virtualY)
	    reportVirtual = FALSE;
	RRRegisterRate (pScreen, pSize, refresh);
	if (mode == scrp->currentMode &&
	    mode->HDisplay == pScreen->width && mode->VDisplay == pScreen->height)
	    RRSetCurrentConfig (pScreen, RR_Rotate_0, refresh, pSize);
	if (mode->next == scrp->modes)
	    break;
    }
    if (reportVirtual)
    {
	mode = scrp->modes;
	pSize = RRRegisterSize (pScreen,
				randrp->virtualX, randrp->virtualY,
				pScreen->mmWidth * randrp->virtualX / mode->HDisplay,
				pScreen->mmHeight * randrp->virtualY / mode->VDisplay);
	if (!pSize)
	    return FALSE;
	RRRegisterRate (pScreen, pSize, refresh0);
	if (pScreen->width == randrp->virtualX && 
	    pScreen->height == randrp->virtualY)
	{
	    RRSetCurrentConfig (pScreen, RR_Rotate_0, refresh0, pSize);
	}
    }
    return TRUE;
}

static Bool
xf86RandRSetMode (ScreenPtr	    pScreen,
		  DisplayModePtr    mode,
		  Bool		    useVirtual)
{
    ScrnInfoPtr		scrp = XF86SCRNINFO(pScreen);
    XF86RandRInfoPtr	randrp = XF86RANDRINFO(pScreen);
    int			oldWidth = pScreen->width;
    int			oldHeight = pScreen->height;
    WindowPtr		pRoot = WindowTable[pScreen->myNum];
    
    if (pRoot)
	xf86EnableDisableFBAccess (pScreen->myNum, FALSE);
    if (useVirtual)
    {
	scrp->virtualX = randrp->virtualX;
	scrp->virtualY = randrp->virtualY;
    }
    else
    {
	scrp->virtualX = mode->HDisplay;
	scrp->virtualY = mode->VDisplay;
    }
    pScreen->width = scrp->virtualX;
    pScreen->height = scrp->virtualY;
    if (!xf86SwitchMode (pScreen, mode))
    {
	scrp->virtualX = pScreen->width = oldWidth;
	scrp->virtualY = pScreen->height = oldHeight;
	return FALSE;
    }
    /*
     * Make sure the layout is correct
     */
    xf86ReconfigureLayout();

    /*
     * Make sure the whole screen is visible
     */
    xf86SetViewport (pScreen, pScreen->width, pScreen->height);
    xf86SetViewport (pScreen, 0, 0);
    if (pRoot)
	xf86EnableDisableFBAccess (pScreen->myNum, TRUE);
    return TRUE;
}

static Bool
xf86RandRSetConfig (ScreenPtr		pScreen,
		    Rotation		rotation,
		    int			rate,
		    RRScreenSizePtr	pSize)
{
    ScrnInfoPtr		    scrp = XF86SCRNINFO(pScreen);
    XF86RandRInfoPtr	    randrp = XF86RANDRINFO(pScreen);
    DisplayModePtr	    mode;
    int			    px, py;
    Bool		    useVirtual = FALSE;

    miPointerPosition (&px, &py);
    for (mode = scrp->modes; ; mode = mode->next)
    {
	if (mode->HDisplay == pSize->width && 
	    mode->VDisplay == pSize->height &&
	    (rate == 0 || xf86RandRModeRefresh (mode) == rate))
	    break;
	if (mode->next == scrp->modes)
	{
	    if (pSize->width == randrp->virtualX &&
		pSize->height == randrp->virtualY)
	    {
		mode = scrp->modes;
		useVirtual = TRUE;
		break;
	    }
	    return FALSE;
	}
    }
    if (!xf86RandRSetMode (pScreen, mode, useVirtual))
	return FALSE;
    /*
     * Move the cursor back where it belongs; SwitchMode repositions it
     */
    if (pScreen == miPointerCurrentScreen ())
    {
	if (px < pSize->width && py < pSize->height)
	    (*pScreen->SetCursorPosition) (pScreen, px, py, FALSE);
    }
    return TRUE;
}

/*
 * Wait until the screen is initialized before whacking the
 * sizes around; otherwise the screen pixmap will be allocated
 * at the current mode size rather than the maximum size
 */
static Bool
xf86RandRCreateScreenResources (ScreenPtr pScreen)
{
    ScrnInfoPtr		    scrp = XF86SCRNINFO(pScreen);
    XF86RandRInfoPtr	    randrp = XF86RANDRINFO(pScreen);
    DisplayModePtr	    mode;

    pScreen->CreateScreenResources = randrp->CreateScreenResources;
    if (!(*pScreen->CreateScreenResources) (pScreen))
	return FALSE;
    
    mode = scrp->currentMode;
    if (mode)
	xf86RandRSetMode (pScreen, mode, TRUE);
    
    return TRUE;
}

/*
 * Reset size back to original
 */
static Bool
xf86RandRCloseScreen (int index, ScreenPtr pScreen)
{
    ScrnInfoPtr		    scrp = XF86SCRNINFO(pScreen);
    XF86RandRInfoPtr	    randrp = XF86RANDRINFO(pScreen);
    
    scrp->virtualX = pScreen->width = randrp->virtualX;
    scrp->virtualY = pScreen->height = randrp->virtualY;
    scrp->currentMode = scrp->modes;
    pScreen->CloseScreen = randrp->CloseScreen;
    xfree (randrp);
    pScreen->devPrivates[xf86RandRIndex].ptr = 0;
    return (*pScreen->CloseScreen) (index, pScreen);
}

Bool
xf86RandRInit (ScreenPtr    pScreen)
{
    rrScrPrivPtr	rp;
    XF86RandRInfoPtr	randrp;
    ScrnInfoPtr		scrp = XF86SCRNINFO(pScreen);
    
#ifdef PANORAMIX
    /* XXX disable RandR when using Xinerama */
    if (!noPanoramiXExtension)
	return TRUE;
#endif
    if (xf86RandRGeneration != serverGeneration)
    {
	xf86RandRIndex = AllocateScreenPrivateIndex();
	xf86RandRGeneration = serverGeneration;
    }
    
    randrp = xalloc (sizeof (XF86RandRInfoRec));
    if (!randrp)
	return FALSE;
			
    if (!RRScreenInit (pScreen))
    {
	xfree (randrp);
	return FALSE;
    }
    rp = rrGetScrPriv(pScreen);
    rp->rrGetInfo = xf86RandRGetInfo;
    rp->rrSetConfig = xf86RandRSetConfig;

    randrp->virtualX = scrp->virtualX;
    randrp->virtualY = scrp->virtualY;
    
    randrp->CreateScreenResources = pScreen->CreateScreenResources;
    pScreen->CreateScreenResources = xf86RandRCreateScreenResources;
    
    randrp->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = xf86RandRCloseScreen;

    pScreen->devPrivates[xf86RandRIndex].ptr = randrp;
    return TRUE;
}
