/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86DPMS.c,v 1.8 2003/02/13 02:41:09 dawes Exp $ */

/*
 * Copyright (c) 1997-1998 by The XFree86 Project, Inc.
 */

/*
 * This file contains the DPMS functions required by the extension.
 */

#include "X.h"
#include "os.h"
#include "globals.h"
#include "xf86.h"
#include "xf86Priv.h"
#ifdef DPMSExtension
#include "dpmsproc.h"
#endif


#ifdef DPMSExtension
static int DPMSGeneration = 0;
static int DPMSIndex = -1;
static Bool DPMSClose(int i, ScreenPtr pScreen);
static int DPMSCount = 0;
#endif


Bool
xf86DPMSInit(ScreenPtr pScreen, DPMSSetProcPtr set, int flags)
{
#ifdef DPMSExtension
    DPMSPtr pDPMS;
    pointer DPMSOpt;

    if (serverGeneration != DPMSGeneration) {
	if ((DPMSIndex = AllocateScreenPrivateIndex()) < 0)
	    return FALSE;
	DPMSGeneration = serverGeneration;
    }

    if (DPMSDisabledSwitch)
	DPMSEnabled = FALSE;
    if (!(pScreen->devPrivates[DPMSIndex].ptr = xcalloc(sizeof(DPMSRec), 1)))
	return FALSE;

    pDPMS = (DPMSPtr)pScreen->devPrivates[DPMSIndex].ptr;
    pDPMS->Set = set;
    pDPMS->Flags = flags;
    DPMSOpt = xf86FindOption(xf86Screens[pScreen->myNum]->options, "dpms");
    if (DPMSOpt) {
	if ((pDPMS->Enabled
	    = xf86SetBoolOption(xf86Screens[pScreen->myNum]->options,
				"dpms",FALSE))
	    && !DPMSDisabledSwitch)
	    DPMSEnabled = TRUE;
	xf86MarkOptionUsed(DPMSOpt);
	xf86DrvMsg(pScreen->myNum, X_CONFIG, "DPMS enabled\n");
    } else if (DPMSEnabledSwitch) {
	if (!DPMSDisabledSwitch)
	    DPMSEnabled = TRUE;
	pDPMS->Enabled = TRUE;
    }  
    else {
	pDPMS->Enabled = FALSE;
    }
    pDPMS->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = DPMSClose;
    DPMSCount++;
    return TRUE;
#else
    return FALSE;
#endif
}


#ifdef DPMSExtension

static Bool
DPMSClose(int i, ScreenPtr pScreen)
{
    DPMSPtr pDPMS;

    /* This shouldn't happen */
    if (DPMSIndex < 0)
	return FALSE;

    pDPMS = (DPMSPtr)pScreen->devPrivates[DPMSIndex].ptr;

    /* This shouldn't happen */
    if (!pDPMS)
	return FALSE;

    pScreen->CloseScreen = pDPMS->CloseScreen;

    xfree((pointer)pDPMS);
    pScreen->devPrivates[DPMSIndex].ptr = NULL;
    if (--DPMSCount == 0)
	DPMSIndex = -1;
    return pScreen->CloseScreen(i, pScreen);
}


/*
 * DPMSSet --
 *	Device dependent DPMS mode setting hook.  This is called whenever
 *	the DPMS mode is to be changed.
 */
void
DPMSSet(int level)
{
    int i;
    DPMSPtr pDPMS;

    DPMSPowerLevel = level;

    if (DPMSIndex < 0)
	return;

    /* For each screen, set the DPMS level */
    for (i = 0; i < xf86NumScreens; i++) {
	pDPMS = (DPMSPtr)screenInfo.screens[i]->devPrivates[DPMSIndex].ptr;
	if (pDPMS && pDPMS->Set && pDPMS->Enabled && xf86Screens[i]->vtSema) {
	    xf86EnableAccess(xf86Screens[i]);
	    pDPMS->Set(xf86Screens[i], level, 0);
	}
    }
}


/*
 * DPMSSupported --
 *	Return TRUE if any screen supports DPMS.
 */
Bool
DPMSSupported(void)
{
    int i;
    DPMSPtr pDPMS;

    if (DPMSIndex < 0) {
	return FALSE;
    }

    /* For each screen, check if DPMS is supported */
    for (i = 0; i < xf86NumScreens; i++) {
	pDPMS = (DPMSPtr)screenInfo.screens[i]->devPrivates[DPMSIndex].ptr;
	if (pDPMS && pDPMS->Set)
	    return TRUE;
    }
    return FALSE;
}


/*
 * DPMSGet --
 *	Device dependent DPMS mode getting hook.  This returns the current
 *	DPMS mode, or -1 if DPMS is not supported.
 *
 *	This should hook in to the appropriate driver-level function, which
 *	will be added to the ScrnInfoRec.
 *
 *	NOTES:
 *	 1. the calling interface should be changed to specify which
 *	    screen to check.
 *	 2. It isn't clear that this function is ever used or what it should
 *	    return.
 */
int
DPMSGet(int *level)
{
    return DPMSPowerLevel;
}

#endif /* DPMSExtension */
