/*
 *
 * Quartz-specific support for the XRandR extension
 *
 * Copyright (c) 2001-2004 Greg Parker and Torrey T. Lyons,
 *               2010      Jan Hauffa.
 *                 All Rights Reserved.
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

#include "sanitizedCarbon.h"

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "quartzCommon.h"
#include "quartzRandR.h"
#include "quartz.h"

#include <X11/extensions/randr.h>
#include <randrstr.h>
#include <IOKit/graphics/IOGraphicsTypes.h>


#define DEFAULT_REFRESH  60
#define kDisplayModeUsableFlags  (kDisplayModeValidFlag | kDisplayModeSafeFlag)

typedef Bool (*QuartzModeCallback)
    (ScreenPtr, CGDirectDisplayID, QuartzModeInfoPtr, void *);


#if defined(USE_DEPRECATED_CG_API)

static long getDictLong (CFDictionaryRef dictRef, CFStringRef key) {
    long value;

    CFNumberRef numRef = (CFNumberRef) CFDictionaryGetValue(dictRef, key);
    if (!numRef)
        return 0;

    if (!CFNumberGetValue(numRef, kCFNumberLongType, &value))
        return 0;
    return value;
}

static double getDictDouble (CFDictionaryRef dictRef, CFStringRef key) {
    double value;

    CFNumberRef numRef = (CFNumberRef) CFDictionaryGetValue(dictRef, key);
    if (!numRef)
        return 0.0;

    if (!CFNumberGetValue(numRef, kCFNumberDoubleType, &value))
        return 0.0;
    return value;
}

static void QuartzRandRGetModeInfo (CFDictionaryRef modeRef,
                                    QuartzModeInfoPtr pMode) {
    pMode->width = (size_t) getDictLong(modeRef, kCGDisplayWidth);
    pMode->height = (size_t) getDictLong(modeRef, kCGDisplayHeight);
    pMode->refresh = (int)(getDictDouble(modeRef, kCGDisplayRefreshRate) + 0.5);
    if (pMode->refresh == 0)
        pMode->refresh = DEFAULT_REFRESH;
    pMode->ref = NULL;
}

static Bool QuartzRandRGetCurrentModeInfo (CGDirectDisplayID screenId,
                                           QuartzModeInfoPtr pMode) {
    CFDictionaryRef curModeRef = CGDisplayCurrentMode(screenId);
    if (!curModeRef)
        return FALSE;

    QuartzRandRGetModeInfo(curModeRef, pMode);
    return TRUE;
}

static Bool QuartzRandRSetMode (CGDirectDisplayID screenId,
                                QuartzModeInfoPtr pMode) {
    CFDictionaryRef modeRef = (CFDictionaryRef) pMode->ref;
    return (CGDisplaySwitchToMode(screenId, modeRef) != kCGErrorSuccess);
}

static Bool QuartzRandREnumerateModes (ScreenPtr pScreen,
                                       CGDirectDisplayID screenId,
                                       QuartzModeCallback callback,
                                       void *data) {
    CFDictionaryRef curModeRef, modeRef;
    long curBpp;
    CFArrayRef modes;
    QuartzModeInfo modeInfo;
    int i;

    curModeRef = CGDisplayCurrentMode(screenId);
    if (!curModeRef)
        return FALSE;
    curBpp = getDictLong(curModeRef, kCGDisplayBitsPerPixel);

    modes = CGDisplayAvailableModes(screenId);
    if (!modes)
        return FALSE;
    for (i = 0; i < CFArrayGetCount(modes); i++) {
        modeRef = (CFDictionaryRef) CFArrayGetValueAtIndex(modes, i);

        /* Skip modes that are not usable on the current display or have a
           different pixel encoding than the current mode. */
        if (((unsigned long) getDictLong(modeRef, kCGDisplayIOFlags) &
             kDisplayModeUsableFlags) != kDisplayModeUsableFlags)
            continue;
        if (getDictLong(modeRef, kCGDisplayBitsPerPixel) != curBpp)
            continue;

        QuartzRandRGetModeInfo(modeRef, &modeInfo);
        modeInfo.ref = modeRef;
        if (!callback(pScreen, screenId, &modeInfo, data))
            break;
    }
    return TRUE;
}

#else  /* defined(USE_DEPRECATED_CG_API) */

static void QuartzRandRGetModeInfo (CGDisplayModeRef modeRef,
                                    QuartzModeInfoPtr pMode) {
    pMode->width = CGDisplayModeGetWidth(modeRef);
    pMode->height = CGDisplayModeGetHeight(modeRef);
    pMode->refresh = (int) (CGDisplayModeGetRefreshRate(modeRef) + 0.5);
    if (pMode->refresh == 0)
        pMode->refresh = DEFAULT_REFRESH;
    pMode->ref = NULL;
}

static Bool QuartzRandRGetCurrentModeInfo (CGDirectDisplayID screenId,
                                           QuartzModeInfoPtr pMode) {
    CGDisplayModeRef curModeRef = CGDisplayCopyDisplayMode(screenId);
    if (!curModeRef)
        return FALSE;

    QuartzRandRGetModeInfo(curModeRef, pMode);
    CGDisplayModeRelease(curModeRef);
    return TRUE;
}

static Bool QuartzRandRSetMode (CGDirectDisplayID screenId,
                                QuartzModeInfoPtr pMode) {
    CGDisplayModeRef modeRef = (CGDisplayModeRef) pMode->ref;
    if (!modeRef)
        return FALSE;

    return (CGDisplaySetDisplayMode(screenId, modeRef, NULL) !=
            kCGErrorSuccess);
}

static Bool QuartzRandREnumerateModes (ScreenPtr pScreen,
                                       CGDirectDisplayID screenId,
                                       QuartzModeCallback callback,
                                       void *data) {
    CGDisplayModeRef curModeRef, modeRef;
    CFStringRef curPixelEnc, pixelEnc;
    CFComparisonResult pixelEncEqual;
    CFArrayRef modes;
    QuartzModeInfo modeInfo;
    int i;

    curModeRef = CGDisplayCopyDisplayMode(screenId);
    if (!curModeRef)
        return FALSE;
    curPixelEnc = CGDisplayModeCopyPixelEncoding(curModeRef);
    CGDisplayModeRelease(curModeRef);

    modes = CGDisplayCopyAllDisplayModes(screenId, NULL);
    if (!modes) {
        CFRelease(curPixelEnc);
        return FALSE;
    }
    for (i = 0; i < CFArrayGetCount(modes); i++) {
        modeRef = (CGDisplayModeRef) CFArrayGetValueAtIndex(modes, i);

        /* Skip modes that are not usable on the current display or have a
           different pixel encoding than the current mode. */
        if ((CGDisplayModeGetIOFlags(modeRef) & kDisplayModeUsableFlags) !=
            kDisplayModeUsableFlags)
            continue;
        pixelEnc = CGDisplayModeCopyPixelEncoding(modeRef);
        pixelEncEqual = CFStringCompare(pixelEnc, curPixelEnc, 0);
        CFRelease(pixelEnc);
        if (pixelEncEqual != kCFCompareEqualTo)
            continue;

        QuartzRandRGetModeInfo(modeRef, &modeInfo);
        modeInfo.ref = modeRef;
        if (!callback(pScreen, screenId, &modeInfo, data))
            break;
    }
    CFRelease(modes);

    CFRelease(curPixelEnc);
    return TRUE;
}

#endif  /* defined(USE_DEPRECATED_CG_API) */


static Bool QuartzRandRModesEqual (QuartzModeInfoPtr pMode1,
                                   QuartzModeInfoPtr pMode2) {
    if (pMode1->width != pMode2->width)
        return FALSE;
    if (pMode1->height != pMode2->height)
        return FALSE;
    if (pMode1->refresh != pMode2->refresh)
        return FALSE;
    return TRUE;
}

static Bool QuartzRandRRegisterMode (ScreenPtr pScreen,
                                     QuartzModeInfoPtr pMode,
                                     Bool isCurrentMode) {
    RRScreenSizePtr pSize = RRRegisterSize(pScreen,
        pMode->width, pMode->height, pScreen->mmWidth, pScreen->mmHeight);
    if (pSize) {
        RRRegisterRate(pScreen, pSize, pMode->refresh);

        if (isCurrentMode)
            RRSetCurrentConfig(pScreen, RR_Rotate_0, pMode->refresh, pSize);

        return TRUE;
    }
    return FALSE;
}

static Bool QuartzRandRGetModeCallback (ScreenPtr pScreen,
                                        CGDirectDisplayID screenId,
                                        QuartzModeInfoPtr pMode,
                                        void *data) {
    QuartzModeInfoPtr pCurMode = (QuartzModeInfoPtr) data;

    return QuartzRandRRegisterMode(pScreen, pMode,
        QuartzRandRModesEqual(pMode, pCurMode));
}

static Bool QuartzRandRSetModeCallback (ScreenPtr pScreen,
                                        CGDirectDisplayID screenId,
                                        QuartzModeInfoPtr pMode,
                                        void *data) {
    QuartzModeInfoPtr pReqMode = (QuartzModeInfoPtr) data;

    if (!QuartzRandRModesEqual(pMode, pReqMode))
        return TRUE;  /* continue enumeration */

    return QuartzRandRSetMode(screenId, pMode);
}

static Bool QuartzRandRGetInfo (ScreenPtr pScreen, Rotation *rotations) {
    QuartzScreenPtr pQuartzScreen = QUARTZ_PRIV(pScreen);
    CGDirectDisplayID screenId;
    QuartzModeInfo curMode;

    *rotations = RR_Rotate_0;  /* TODO: support rotation */

    if (pQuartzScreen->displayCount == 0)
        return FALSE;
    if (pQuartzScreen->displayCount > 1) {
        /* RandR operations are not well-defined for an X11 screen spanning
           multiple CG displays. Create two entries for the current virtual
           resolution including/excluding the menu bar. */
        QuartzRandRRegisterMode(pScreen, &pQuartzScreen->fakeMode,
            !quartzHasRoot);
        QuartzRandRRegisterMode(pScreen, &pQuartzScreen->originalMode,
            quartzHasRoot);
        return TRUE;
    }
    screenId = pQuartzScreen->displayIDs[0];

    if (!QuartzRandRGetCurrentModeInfo(screenId, &curMode))
        return FALSE;

    /* Add a fake mode corresponding to the original resolution excluding the
       height of the menu bar. */
    if (!quartzHasRoot &&
        QuartzRandRModesEqual(&pQuartzScreen->originalMode, &curMode)) {
        QuartzRandRRegisterMode(pScreen, &pQuartzScreen->fakeMode, TRUE);
        curMode = pQuartzScreen->fakeMode;
    }
    else
        QuartzRandRRegisterMode(pScreen, &pQuartzScreen->fakeMode, FALSE);

    return QuartzRandREnumerateModes(pScreen, screenId,
        QuartzRandRGetModeCallback, &curMode);
}

static Bool QuartzRandRSetConfig (ScreenPtr           pScreen,
			          Rotation            randr,
			          int                 rate,
			          RRScreenSizePtr     pSize) {
    QuartzScreenPtr pQuartzScreen = QUARTZ_PRIV(pScreen);
    CGDirectDisplayID screenId;
    QuartzModeInfo reqMode, curMode;
    Bool rootless = FALSE;

    reqMode.width = pSize->width;
    reqMode.height = pSize->height;
    reqMode.refresh = rate;

    /* If the client requested the fake screen mode, switch to rootless mode.
       Switch to fullscreen mode (root window visible) if a real screen mode was
       requested. */
    if (QuartzRandRModesEqual(&reqMode, &pQuartzScreen->fakeMode)) {
        rootless = TRUE;
        reqMode = pQuartzScreen->originalMode;
    }
    QuartzSetFullscreen(!rootless);
    QuartzSetRootless(rootless);

    if (pQuartzScreen->displayCount == 0)
        return FALSE;
    if (pQuartzScreen->displayCount > 1) {
        /* RandR operations are not well-defined for an X11 screen spanning
           multiple CG displays. Do not accept any configuations that differ
           from the current configuration. */
        return QuartzRandRModesEqual(&reqMode, &pQuartzScreen->originalMode);
    }
    screenId = pQuartzScreen->displayIDs[0];

    /* Do not switch modes if requested mode is equal to current mode. */
    if (!QuartzRandRGetCurrentModeInfo(screenId, &curMode))
        return FALSE;
    if (QuartzRandRModesEqual(&reqMode, &curMode))
        return TRUE;

    return QuartzRandREnumerateModes(pScreen, screenId,
        QuartzRandRSetModeCallback, &reqMode);
}

Bool QuartzRandRInit (ScreenPtr pScreen) {
    rrScrPrivPtr    pScrPriv;
    QuartzScreenPtr pQuartzScreen = QUARTZ_PRIV(pScreen);
    
    if (!RRScreenInit (pScreen)) return FALSE;

    if (pQuartzScreen->displayCount == 1) {
        if (!QuartzRandRGetCurrentModeInfo(pQuartzScreen->displayIDs[0],
                                           &pQuartzScreen->originalMode))
            return FALSE;
    }
    else {
        pQuartzScreen->originalMode.width = pScreen->width;
        pQuartzScreen->originalMode.height = pScreen->height;
        pQuartzScreen->originalMode.refresh = DEFAULT_REFRESH;
    }
    pQuartzScreen->fakeMode = pQuartzScreen->originalMode;
    pQuartzScreen->fakeMode.height -= aquaMenuBarHeight;

    pScrPriv = rrGetScrPriv(pScreen);
    pScrPriv->rrGetInfo = QuartzRandRGetInfo;
    pScrPriv->rrSetConfig = QuartzRandRSetConfig;
    return TRUE;
}
