/*
 * Copyright (c) 1993-2003 by The XFree86 Project, Inc.
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

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <X11/X.h>
#include "input.h"
#include "scrnintstr.h"

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "xf86OSpriv.h"

/*
 * This file contains the common part of the video memory mapping functions
 */

/*
 * Get a piece of the ScrnInfoRec.  At the moment, this is only used to hold
 * the MTRR option information, but it is likely to be expanded if we do
 * auto unmapping of memory at VT switch.
 *
 */

typedef struct {
    Bool mtrrEnabled;
    MessageType mtrrFrom;
    Bool mtrrOptChecked;
    ScrnInfoPtr pScrn;
} VidMapRec, *VidMapPtr;

static int vidMapIndex = -1;

#define VIDMAPPTR(p) ((VidMapPtr)((p)->privates[vidMapIndex].ptr))

static VidMemInfo vidMemInfo = { FALSE, };
static VidMapRec vidMapRec = { TRUE, X_DEFAULT, FALSE, NULL };

static VidMapPtr
getVidMapRec(int scrnIndex)
{
    VidMapPtr vp;
    ScrnInfoPtr pScrn;

    if ((scrnIndex < 0) || !(pScrn = xf86Screens[scrnIndex]))
        return &vidMapRec;

    if (vidMapIndex < 0)
        vidMapIndex = xf86AllocateScrnInfoPrivateIndex();

    if (VIDMAPPTR(pScrn) != NULL)
        return VIDMAPPTR(pScrn);

    vp = pScrn->privates[vidMapIndex].ptr = xnfcalloc(sizeof(VidMapRec), 1);
    vp->mtrrEnabled = TRUE;     /* default to enabled */
    vp->mtrrFrom = X_DEFAULT;
    vp->mtrrOptChecked = FALSE;
    vp->pScrn = pScrn;
    return vp;
}

enum { OPTION_MTRR };

static const OptionInfoRec opts[] = {
    {OPTION_MTRR, "mtrr", OPTV_BOOLEAN, {0}, FALSE},
    {-1, NULL, OPTV_NONE, {0}, FALSE}
};

static void
checkMtrrOption(VidMapPtr vp)
{
    if (!vp->mtrrOptChecked && vp->pScrn && vp->pScrn->options != NULL) {
        OptionInfoPtr options;

        options = xnfalloc(sizeof(opts));
        (void) memcpy(options, opts, sizeof(opts));
        xf86ProcessOptions(vp->pScrn->scrnIndex, vp->pScrn->options, options);
        if (xf86GetOptValBool(options, OPTION_MTRR, &vp->mtrrEnabled))
            vp->mtrrFrom = X_CONFIG;
        free(options);
        vp->mtrrOptChecked = TRUE;
    }
}

void
xf86InitVidMem(void)
{
    if (!vidMemInfo.initialised) {
        memset(&vidMemInfo, 0, sizeof(VidMemInfo));
        xf86OSInitVidMem(&vidMemInfo);
    }
}

Bool
xf86CheckMTRR(int ScreenNum)
{
    VidMapPtr vp = getVidMapRec(ScreenNum);

    /*
     * Check the "mtrr" option even when MTRR isn't supported to avoid
     * warnings about unrecognised options.
     */
    checkMtrrOption(vp);

    if (vp->mtrrEnabled)
        return TRUE;

    return FALSE;
}

Bool
xf86LinearVidMem(void)
{
    xf86InitVidMem();
    return vidMemInfo.linearSupported;
}
