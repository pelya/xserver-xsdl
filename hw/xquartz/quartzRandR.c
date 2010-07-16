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

#if defined(FAKE_RANDR)
#include "scrnintstr.h"
#include "windowstr.h"
#else
#include <X11/extensions/randr.h>
#include <randrstr.h>
#include <IOKit/graphics/IOGraphicsTypes.h>
#endif


#if defined(FAKE_RANDR)

static const int padlength[4] = {0, 3, 2, 1};

void
RREditConnectionInfo (ScreenPtr pScreen)
{
    xConnSetup	    *connSetup;
    char	    *vendor;
    xPixmapFormat   *formats;
    xWindowRoot	    *root;
    xDepth	    *depth;
    xVisualType	    *visual;
    int		    screen = 0;
    int		    d;

    connSetup = (xConnSetup *) ConnectionInfo;
    vendor = (char *) connSetup + sizeof (xConnSetup);
    formats = (xPixmapFormat *) ((char *) vendor +
				 connSetup->nbytesVendor +
				 padlength[connSetup->nbytesVendor & 3]);
    root = (xWindowRoot *) ((char *) formats +
			    sizeof (xPixmapFormat) * screenInfo.numPixmapFormats);
    while (screen != pScreen->myNum)
    {
	depth = (xDepth *) ((char *) root + 
			    sizeof (xWindowRoot));
	for (d = 0; d < root->nDepths; d++)
	{
	    visual = (xVisualType *) ((char *) depth +
				      sizeof (xDepth));
	    depth = (xDepth *) ((char *) visual +
				depth->nVisuals * sizeof (xVisualType));
	}
	root = (xWindowRoot *) ((char *) depth);
	screen++;
    }
    root->pixWidth = pScreen->width;
    root->pixHeight = pScreen->height;
    root->mmWidth = pScreen->mmWidth;
    root->mmHeight = pScreen->mmHeight;
}

#else  /* defined(FAKE_RANDR) */

#define DEFAULT_REFRESH  60
#define kDisplayModeUsableFlags  (kDisplayModeValidFlag | kDisplayModeSafeFlag)

typedef struct {
    size_t width, height;
    int refresh;
    const void *ref;
} QuartzModeInfo, *QuartzModeInfoPtr;

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

static Bool QuartzRandRGetModeCallback (ScreenPtr pScreen,
                                        CGDirectDisplayID screenId,
                                        QuartzModeInfoPtr pMode,
                                        void *data) {
    QuartzModeInfoPtr pCurMode = (QuartzModeInfoPtr) data;

    RRScreenSizePtr pSize = RRRegisterSize(pScreen,
        pMode->width, pMode->height, pScreen->mmWidth, pScreen->mmHeight);
    if (pSize) {
        RRRegisterRate(pScreen, pSize, pMode->refresh);

        if (QuartzRandRModesEqual(pMode, pCurMode))
            RRSetCurrentConfig(pScreen, RR_Rotate_0, pMode->refresh, pSize);
    }
    return TRUE;
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
           multiple CG displays. Create a single entry for the current virtual
           resolution. */
        RRScreenSizePtr pSize = RRRegisterSize(pScreen, pScreen->width,
            pScreen->height, pScreen->mmWidth, pScreen->mmHeight);
        if (pSize) {
            RRRegisterRate(pScreen, pSize, DEFAULT_REFRESH);
            RRSetCurrentConfig(pScreen, RR_Rotate_0, DEFAULT_REFRESH, pSize);
        }
        return TRUE;
    }
    screenId = pQuartzScreen->displayIDs[0];

    if (!QuartzRandRGetCurrentModeInfo(screenId, &curMode))
        return FALSE;
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

    if (pQuartzScreen->displayCount == 0)
        return FALSE;
    if (pQuartzScreen->displayCount > 1) {
        /* RandR operations are not well-defined for an X11 screen spanning
           multiple CG displays. Do not accept any configuations that differ
           from the current configuration. */
        return ((pSize->width == pScreen->width) &&
                (pSize->height == pScreen->height));
    }
    screenId = pQuartzScreen->displayIDs[0];

    reqMode.width = pSize->width;
    reqMode.height = pSize->height;
    reqMode.refresh = rate;

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
    
    if (!RRScreenInit (pScreen)) return FALSE;

    pScrPriv = rrGetScrPriv(pScreen);
    pScrPriv->rrGetInfo = QuartzRandRGetInfo;
    pScrPriv->rrSetConfig = QuartzRandRSetConfig;
    return TRUE;
}

#endif  /* defined(FAKE_RANDR) */
