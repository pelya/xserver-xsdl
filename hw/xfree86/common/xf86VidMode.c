/*
 * Copyright (c) 1999-2003 by The XFree86 Project, Inc.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

/*
 * This file contains the VidMode functions required by the extension.
 * These have been added to avoid the need for the higher level extension
 * code to access the private XFree86 data structures directly. Wherever
 * possible this code uses the functions in xf86Mode.c to do the work,
 * so that two version of code that do similar things don't have to be
 * maintained.
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <X11/X.h>
#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"

#ifdef XF86VIDMODE
#include "vidmodeproc.h"
#include "xf86cmap.h"

static DevPrivateKeyRec VidModeKeyRec;
#define VidModeKey (&VidModeKeyRec)

#define VMPTR(p) ((VidModePtr)dixLookupPrivate(&(p)->devPrivates, VidModeKey))

#endif

#ifdef XF86VIDMODE

static Bool
xf86VidModeAvailable(ScreenPtr pScreen)
{
    if (pScreen == NULL) {
        DebugF("pScreen == NULL\n");
        return FALSE;
    }

    if (VMPTR(pScreen))
        return TRUE;
    else {
        DebugF("pVidMode == NULL\n");
        return FALSE;
    }
}

vidMonitorValue
xf86VidModeGetMonitorValue(ScreenPtr pScreen, int valtyp, int indx)
{
    vidMonitorValue ret = { NULL, };
    MonPtr monitor;
    ScrnInfoPtr pScrn;

    if (!xf86VidModeAvailable(pScreen))
        return ret;

    pScrn = xf86ScreenToScrn(pScreen);
    monitor = pScrn->monitor;

    switch (valtyp) {
    case VIDMODE_MON_VENDOR:
        ret.ptr = monitor->vendor;
        break;
    case VIDMODE_MON_MODEL:
        ret.ptr = monitor->model;
        break;
    case VIDMODE_MON_NHSYNC:
        ret.i = monitor->nHsync;
        break;
    case VIDMODE_MON_NVREFRESH:
        ret.i = monitor->nVrefresh;
        break;
    case VIDMODE_MON_HSYNC_LO:
        ret.f = (100.0 * monitor->hsync[indx].lo);
        break;
    case VIDMODE_MON_HSYNC_HI:
        ret.f = (100.0 * monitor->hsync[indx].hi);
        break;
    case VIDMODE_MON_VREFRESH_LO:
        ret.f = (100.0 * monitor->vrefresh[indx].lo);
        break;
    case VIDMODE_MON_VREFRESH_HI:
        ret.f = (100.0 * monitor->vrefresh[indx].hi);
        break;
    }
    return ret;
}

Bool
xf86VidModeGetCurrentModeline(ScreenPtr pScreen, DisplayModePtr *mode, int *dotClock)
{
    ScrnInfoPtr pScrn;

    if (!xf86VidModeAvailable(pScreen))
        return FALSE;

    pScrn = xf86ScreenToScrn(pScreen);

    if (pScrn->currentMode) {
        *mode = pScrn->currentMode;
        *dotClock = pScrn->currentMode->Clock;

        return TRUE;
    }
    return FALSE;
}

int
xf86VidModeGetDotClock(ScreenPtr pScreen, int Clock)
{
    ScrnInfoPtr pScrn;

    if (!xf86VidModeAvailable(pScreen))
        return 0;

    pScrn = xf86ScreenToScrn(pScreen);
    if ((pScrn->progClock) || (Clock >= MAXCLOCKS))
        return Clock;
    else
        return pScrn->clock[Clock];
}

int
xf86VidModeGetNumOfClocks(ScreenPtr pScreen, Bool *progClock)
{
    ScrnInfoPtr pScrn;

    if (!xf86VidModeAvailable(pScreen))
        return 0;

    pScrn = xf86ScreenToScrn(pScreen);
    if (pScrn->progClock) {
        *progClock = TRUE;
        return 0;
    }
    else {
        *progClock = FALSE;
        return pScrn->numClocks;
    }
}

Bool
xf86VidModeGetClocks(ScreenPtr pScreen, int *Clocks)
{
    ScrnInfoPtr pScrn;
    int i;

    if (!xf86VidModeAvailable(pScreen))
        return FALSE;

    pScrn = xf86ScreenToScrn(pScreen);

    if (pScrn->progClock)
        return FALSE;

    for (i = 0; i < pScrn->numClocks; i++)
        *Clocks++ = pScrn->clock[i];

    return TRUE;
}

Bool
xf86VidModeGetNextModeline(ScreenPtr pScreen, DisplayModePtr *mode, int *dotClock)
{
    VidModePtr pVidMode;
    DisplayModePtr p;

    if (!xf86VidModeAvailable(pScreen))
        return FALSE;

    pVidMode = VMPTR(pScreen);

    for (p = pVidMode->Next; p != NULL && p != pVidMode->First; p = p->next) {
        if (p->status == MODE_OK) {
            pVidMode->Next = p->next;
            *mode = p;
            *dotClock = xf86VidModeGetDotClock(pScreen, p->Clock);
            return TRUE;
        }
    }

    return FALSE;
}

Bool
xf86VidModeGetFirstModeline(ScreenPtr pScreen, DisplayModePtr *mode, int *dotClock)
{
    ScrnInfoPtr pScrn;
    VidModePtr pVidMode;

    if (!xf86VidModeAvailable(pScreen))
        return FALSE;

    pScrn = xf86ScreenToScrn(pScreen);
    if (pScrn->modes == NULL)
        return FALSE;

    pVidMode = VMPTR(pScreen);
    pVidMode->First = pScrn->modes;
    pVidMode->Next = pVidMode->First->next;

    if (pVidMode->First->status == MODE_OK) {
        *mode = pVidMode->First;
        *dotClock = xf86VidModeGetDotClock(pScreen, pVidMode->First->Clock);
        return TRUE;
    }

    return xf86VidModeGetNextModeline(pScreen, mode, dotClock);
}

Bool
xf86VidModeDeleteModeline(ScreenPtr pScreen, DisplayModePtr mode)
{
    ScrnInfoPtr pScrn;

    if ((mode == NULL) || (!xf86VidModeAvailable(pScreen)))
        return FALSE;

    pScrn = xf86ScreenToScrn(pScreen);
    xf86DeleteMode(&(pScrn->modes), mode);
    return TRUE;
}

Bool
xf86VidModeZoomViewport(ScreenPtr pScreen, int zoom)
{
    if (!xf86VidModeAvailable(pScreen))
        return FALSE;

    xf86ZoomViewport(pScreen, zoom);
    return TRUE;
}

Bool
xf86VidModeSetViewPort(ScreenPtr pScreen, int x, int y)
{
    ScrnInfoPtr pScrn;

    if (!xf86VidModeAvailable(pScreen))
        return FALSE;

    pScrn = xf86ScreenToScrn(pScreen);
    pScrn->frameX0 = min(max(x, 0),
                         pScrn->virtualX - pScrn->currentMode->HDisplay);
    pScrn->frameX1 = pScrn->frameX0 + pScrn->currentMode->HDisplay - 1;
    pScrn->frameY0 = min(max(y, 0),
                         pScrn->virtualY - pScrn->currentMode->VDisplay);
    pScrn->frameY1 = pScrn->frameY0 + pScrn->currentMode->VDisplay - 1;
    if (pScrn->AdjustFrame != NULL)
        (pScrn->AdjustFrame) (pScrn, pScrn->frameX0, pScrn->frameY0);

    return TRUE;
}

Bool
xf86VidModeGetViewPort(ScreenPtr pScreen, int *x, int *y)
{
    ScrnInfoPtr pScrn;

    if (!xf86VidModeAvailable(pScreen))
        return FALSE;

    pScrn = xf86ScreenToScrn(pScreen);
    *x = pScrn->frameX0;
    *y = pScrn->frameY0;
    return TRUE;
}

Bool
xf86VidModeSwitchMode(ScreenPtr pScreen, DisplayModePtr mode)
{
    ScrnInfoPtr pScrn;
    DisplayModePtr pTmpMode;
    Bool retval;

    if (!xf86VidModeAvailable(pScreen))
        return FALSE;

    pScrn = xf86ScreenToScrn(pScreen);
    /* save in case we fail */
    pTmpMode = pScrn->currentMode;
    /* Force a mode switch */
    pScrn->currentMode = NULL;
    retval = xf86SwitchMode(pScrn->pScreen, mode);
    /* we failed: restore it */
    if (retval == FALSE)
        pScrn->currentMode = pTmpMode;
    return retval;
}

Bool
xf86VidModeLockZoom(ScreenPtr pScreen, Bool lock)
{
    if (!xf86VidModeAvailable(pScreen))
        return FALSE;

    if (xf86Info.dontZoom)
        return FALSE;

    xf86LockZoom(pScreen, lock);
    return TRUE;
}

ModeStatus
xf86VidModeCheckModeForMonitor(ScreenPtr pScreen, DisplayModePtr mode)
{
    ScrnInfoPtr pScrn;

    if ((mode == NULL) || (!xf86VidModeAvailable(pScreen)))
        return MODE_ERROR;

    pScrn = xf86ScreenToScrn(pScreen);

    return xf86CheckModeForMonitor(mode, pScrn->monitor);
}

ModeStatus
xf86VidModeCheckModeForDriver(ScreenPtr pScreen, DisplayModePtr mode)
{
    ScrnInfoPtr pScrn;

    if ((mode == NULL) || (!xf86VidModeAvailable(pScreen)))
        return MODE_ERROR;

    pScrn = xf86ScreenToScrn(pScreen);

    return xf86CheckModeForDriver(pScrn, mode, 0);
}

void
xf86VidModeSetCrtcForMode(ScreenPtr pScreen, DisplayModePtr mode)
{
    ScrnInfoPtr pScrn;
    DisplayModePtr ScreenModes;

    if ((mode == NULL) || (!xf86VidModeAvailable(pScreen)))
        return;

    /* Ugly hack so that the xf86Mode.c function can be used without change */
    pScrn = xf86ScreenToScrn(pScreen);
    ScreenModes = pScrn->modes;
    pScrn->modes = mode;

    xf86SetCrtcForModes(pScrn, pScrn->adjustFlags);
    pScrn->modes = ScreenModes;
    return;
}

Bool
xf86VidModeAddModeline(ScreenPtr pScreen, DisplayModePtr mode)
{
    ScrnInfoPtr pScrn;

    if ((mode == NULL) || (!xf86VidModeAvailable(pScreen)))
        return FALSE;

    pScrn = xf86ScreenToScrn(pScreen);

    mode->name = strdup(""); /* freed by deletemode */
    mode->status = MODE_OK;
    mode->next = pScrn->modes->next;
    mode->prev = pScrn->modes;
    pScrn->modes->next = mode;
    if (mode->next != NULL)
        mode->next->prev = mode;

    return TRUE;
}

int
xf86VidModeGetNumOfModes(ScreenPtr pScreen)
{
    DisplayModePtr mode = NULL;
    int dotClock = 0, nummodes = 0;

    if (!xf86VidModeGetFirstModeline(pScreen, &mode, &dotClock))
        return nummodes;

    do {
        nummodes++;
        if (!xf86VidModeGetNextModeline(pScreen, &mode, &dotClock))
            return nummodes;
    } while (TRUE);
}

Bool
xf86VidModeSetGamma(ScreenPtr pScreen, float red, float green, float blue)
{
    Gamma gamma;

    if (!xf86VidModeAvailable(pScreen))
        return FALSE;

    gamma.red = red;
    gamma.green = green;
    gamma.blue = blue;
    if (xf86ChangeGamma(pScreen, gamma) != Success)
        return FALSE;
    else
        return TRUE;
}

Bool
xf86VidModeGetGamma(ScreenPtr pScreen, float *red, float *green, float *blue)
{
    ScrnInfoPtr pScrn;

    if (!xf86VidModeAvailable(pScreen))
        return FALSE;

    pScrn = xf86ScreenToScrn(pScreen);
    *red = pScrn->gamma.red;
    *green = pScrn->gamma.green;
    *blue = pScrn->gamma.blue;
    return TRUE;
}

Bool
xf86VidModeSetGammaRamp(ScreenPtr pScreen, int size, CARD16 *r, CARD16 *g, CARD16 *b)
{
    if (!xf86VidModeAvailable(pScreen))
        return FALSE;

    xf86ChangeGammaRamp(pScreen, size, r, g, b);
    return TRUE;
}

Bool
xf86VidModeGetGammaRamp(ScreenPtr pScreen, int size, CARD16 *r, CARD16 *g, CARD16 *b)
{
    if (!xf86VidModeAvailable(pScreen))
        return FALSE;

    xf86GetGammaRamp(pScreen, size, r, g, b);
    return TRUE;
}

int
xf86VidModeGetGammaRampSize(ScreenPtr pScreen)
{
    if (!xf86VidModeAvailable(pScreen))
        return 0;

    return xf86GetGammaRampSize(pScreen);
}

#endif                          /* XF86VIDMODE */

Bool
xf86VidModeExtensionInit(ScreenPtr pScreen)
{
#ifdef XF86VIDMODE
    VidModePtr pVidMode;

    if (!xf86GetVidModeEnabled()) {
        DebugF("!xf86GetVidModeEnabled()\n");
        return FALSE;
    }

    if (!dixRegisterPrivateKey(&VidModeKeyRec, PRIVATE_SCREEN, sizeof(VidModeRec)))
        return FALSE;

    pVidMode = VMPTR(pScreen);

    pVidMode->Flags = 0;
    pVidMode->Next = NULL;

    return TRUE;
#else
    DebugF("no vidmode extension\n");
    return FALSE;
#endif
}

