/*
 * Screen routines for full screen Quartz mode
 *
 * Copyright (c) 2002-2003 Torrey T. Lyons. All Rights Reserved.
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
 * TORREY T. LYONS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written authorization.
 */
/* $XFree86: xc/programs/Xserver/hw/darwin/quartz/fullscreen/fullscreen.c,v 1.4 2003/12/09 04:41:27 torrey Exp $ */

#include "quartzCommon.h"
#include "darwin.h"
#include "quartz.h"
#include "quartzCursor.h"
#include "colormapst.h"
#include "scrnintstr.h"
#include "micmap.h"
#include "shadow.h"

// Full screen specific per screen storage structure
typedef struct {
    CGDirectDisplayID   displayID;
    CFDictionaryRef     xDisplayMode;
    CFDictionaryRef     aquaDisplayMode;
    CGDirectPaletteRef  xPalette;
    CGDirectPaletteRef  aquaPalette;
    unsigned char      *framebuffer;
    unsigned char      *shadowPtr;
} FSScreenRec, *FSScreenPtr;

#define FULLSCREEN_PRIV(pScreen) \
    ((FSScreenPtr)pScreen->devPrivates[fsScreenIndex].ptr)

static int                  fsScreenIndex;
static CGDirectDisplayID   *quartzDisplayList = NULL;
static int                  quartzNumScreens = 0;
static FSScreenPtr          quartzScreens[MAXSCREENS];

static int                  darwinCmapPrivateIndex = -1;
static unsigned long        darwinCmapGeneration = 0;

#define CMAP_PRIV(pCmap) \
    ((CGDirectPaletteRef) (pCmap)->devPrivates[darwinCmapPrivateIndex].ptr)

/*
 =============================================================================

 Colormap handling

 =============================================================================
*/

/*
 * FSInitCmapPrivates
 *  Colormap privates may be allocated after the default colormap has
 *  already been created for some screens.  This initialization procedure
 *  is called for each default colormap that is found.
 */
static Bool
FSInitCmapPrivates(
    ColormapPtr         pCmap)
{
    return TRUE;
}


/*
 * FSCreateColormap
 *  This is a callback from X after a new colormap is created.
 *  We allocate a new CoreGraphics pallete for each colormap.
 */
static Bool
FSCreateColormap(
    ColormapPtr         pCmap)
{
    CGDirectPaletteRef  pallete;

    // Allocate private storage for the hardware dependent colormap info.
    if (darwinCmapGeneration != serverGeneration) {
        if ((darwinCmapPrivateIndex =
                AllocateColormapPrivateIndex(FSInitCmapPrivates)) < 0)
        {
            return FALSE;
        }
        darwinCmapGeneration = serverGeneration;
    }

    pallete = CGPaletteCreateDefaultColorPalette();
    if (!pallete) return FALSE;

    CMAP_PRIV(pCmap) = pallete;
    return TRUE;
}


/*
 * FSDestroyColormap
 *  This is called by DIX FreeColormap after it has uninstalled a colormap
 *  and notified all interested parties. We deallocated the corresponding
 *  CoreGraphics pallete.
 */
static void
FSDestroyColormap(
    ColormapPtr         pCmap)
{
    CGPaletteRelease( CMAP_PRIV(pCmap) );
}


/*
 * FSInstallColormap
 *  Set the current CoreGraphics pallete to the pallete corresponding
 *  to the provided colormap.
 */
static void
FSInstallColormap(
    ColormapPtr         pCmap)
{
    CGDirectPaletteRef  palette = CMAP_PRIV(pCmap);
    ScreenPtr           pScreen = pCmap->pScreen;
    FSScreenPtr         fsDisplayInfo = FULLSCREEN_PRIV(pScreen);

    // Inform all interested parties that the map is being changed.
    miInstallColormap(pCmap);

    if (quartzServerVisible)
        CGDisplaySetPalette(fsDisplayInfo->displayID, palette);

    fsDisplayInfo->xPalette = palette;
}


/*
 * FSStoreColors
 *  This is a callback from X to change the hardware colormap
 *  when using PsuedoColor in full screen mode.
 */
static void
FSStoreColors(
    ColormapPtr         pCmap,
    int                 numEntries,
    xColorItem          *pdefs)
{
    CGDirectPaletteRef  palette = CMAP_PRIV(pCmap);
    ScreenPtr           pScreen = pCmap->pScreen;
    FSScreenPtr         fsDisplayInfo = FULLSCREEN_PRIV(pScreen);
    CGDeviceColor       color;
    int                 i;

    if (! palette)
        return;

    for (i = 0; i < numEntries; i++) {
        color.red   = pdefs[i].red   / 65535.0;
        color.green = pdefs[i].green / 65535.0;
        color.blue  = pdefs[i].blue  / 65535.0;
        CGPaletteSetColorAtIndex(palette, color, pdefs[i].pixel);
    }

    // Update hardware colormap
    if (quartzServerVisible)
        CGDisplaySetPalette(fsDisplayInfo->displayID, palette);
}


/*
 =============================================================================

 Switching between Aqua and X

 =============================================================================
*/

/*
 * FSCapture
 *  Capture the screen so we can draw. Called directly from the main thread
 *  to synchronize with hiding the menubar.
 */
static void FSCapture(void)
{
    int i;

    if (quartzRootless) return;

    for (i = 0; i < quartzNumScreens; i++) {
        FSScreenPtr fsDisplayInfo = quartzScreens[i];
        CGDirectDisplayID cgID = fsDisplayInfo->displayID;

        if (!CGDisplayIsCaptured(cgID)) {
            CGDisplayCapture(cgID);
            fsDisplayInfo->aquaDisplayMode = CGDisplayCurrentMode(cgID);
            if (fsDisplayInfo->xDisplayMode != fsDisplayInfo->aquaDisplayMode)
                CGDisplaySwitchToMode(cgID, fsDisplayInfo->xDisplayMode);
            if (fsDisplayInfo->xPalette)
                CGDisplaySetPalette(cgID, fsDisplayInfo->xPalette);
        }
    }
}


/*
 * FSRelease
 *  Release the screen so others can draw.
 */
static void FSRelease(void)
{
    int i;

    if (quartzRootless) return;

    for (i = 0; i < quartzNumScreens; i++) {
        FSScreenPtr fsDisplayInfo = quartzScreens[i];
        CGDirectDisplayID cgID = fsDisplayInfo->displayID;

        if (CGDisplayIsCaptured(cgID)) {
            if (fsDisplayInfo->xDisplayMode != fsDisplayInfo->aquaDisplayMode)
                CGDisplaySwitchToMode(cgID, fsDisplayInfo->aquaDisplayMode);
            if (fsDisplayInfo->aquaPalette)
                CGDisplaySetPalette(cgID, fsDisplayInfo->aquaPalette);
            CGDisplayRelease(cgID);
        }
    }
}


/*
 * FSSuspendScreen
 *  Suspend X11 cursor and drawing to the screen.
 */
static void FSSuspendScreen(
    ScreenPtr pScreen)
{
    QuartzSuspendXCursor(pScreen);
    xf86SetRootClip(pScreen, FALSE);
}


/*
 * FSResumeScreen
 *  Resume X11 cursor and drawing to the screen.
 */
static void FSResumeScreen(
    ScreenPtr pScreen,
    int x,			// cursor location
    int y )
{
    QuartzResumeXCursor(pScreen, x, y);
    xf86SetRootClip(pScreen, TRUE);
}


/*
 =============================================================================

 Screen initialization

 =============================================================================
*/

/*
 * FSDisplayInit
 *  Full screen specific initialization called from InitOutput.
 */
static void FSDisplayInit(void)
{
    static unsigned long generation = 0;
    CGDisplayCount quartzDisplayCount = 0;

    ErrorF("Display mode: Full screen Quartz -- Direct Display\n");

    // Allocate private storage for each screen's mode specific info
    if (generation != serverGeneration) {
        fsScreenIndex = AllocateScreenPrivateIndex();
        generation = serverGeneration;
    }

    // Find all the CoreGraphics displays
    CGGetActiveDisplayList(0, NULL, &quartzDisplayCount);
    quartzDisplayList = xalloc(quartzDisplayCount * sizeof(CGDirectDisplayID));
    CGGetActiveDisplayList(quartzDisplayCount, quartzDisplayList,
                           &quartzDisplayCount);

    darwinScreensFound = quartzDisplayCount;
    atexit(FSRelease);
}


/*
 * FSFindDisplayMode
 *  Find the appropriate display mode to use in full screen mode.
 *  If display mode is not the same as the current Aqua mode, switch
 *  to the new mode.
 */
static Bool FSFindDisplayMode(
    FSScreenPtr fsDisplayInfo)
{
    CGDirectDisplayID cgID = fsDisplayInfo->displayID;
    size_t height, width, bpp;
    boolean_t exactMatch;

    fsDisplayInfo->aquaDisplayMode = CGDisplayCurrentMode(cgID);

    // If no user options, use current display mode
    if (darwinDesiredWidth == 0 && darwinDesiredDepth == -1 &&
        darwinDesiredRefresh == -1)
    {
        fsDisplayInfo->xDisplayMode = fsDisplayInfo->aquaDisplayMode;
        return TRUE;
    }

    // If the user has no choice for size, use current
    if (darwinDesiredWidth == 0) {
        width = CGDisplayPixelsWide(cgID);
        height = CGDisplayPixelsHigh(cgID);
    } else {
        width = darwinDesiredWidth;
        height = darwinDesiredHeight;
    }

    switch (darwinDesiredDepth) {
        case 0:
            bpp = 8;
            break;
        case 1:
            bpp = 16;
            break;
        case 2:
            bpp = 32;
            break;
        default:
            bpp = CGDisplayBitsPerPixel(cgID);
    }

    if (darwinDesiredRefresh == -1) {
        fsDisplayInfo->xDisplayMode =
                CGDisplayBestModeForParameters(cgID, bpp, width, height,
                        &exactMatch);
    } else {
        fsDisplayInfo->xDisplayMode =
                CGDisplayBestModeForParametersAndRefreshRate(cgID, bpp,
                        width, height, darwinDesiredRefresh, &exactMatch);
    }
    if (!exactMatch) {
        fsDisplayInfo->xDisplayMode = fsDisplayInfo->aquaDisplayMode;
        return FALSE;
    }

    // Switch to the new display mode
    CGDisplaySwitchToMode(cgID, fsDisplayInfo->xDisplayMode);
    return TRUE;
}


/*
 * FSAddScreen
 *  Do initialization of each screen for Quartz in full screen mode.
 */
static Bool FSAddScreen(
    int index,
    ScreenPtr pScreen)
{
    DarwinFramebufferPtr dfb = SCREEN_PRIV(pScreen);
    QuartzScreenPtr displayInfo = QUARTZ_PRIV(pScreen);
    CGDirectDisplayID cgID = quartzDisplayList[index];
    CGRect bounds;
    FSScreenPtr fsDisplayInfo;

    // Allocate space for private per screen fullscreen specific storage.
    fsDisplayInfo = xalloc(sizeof(FSScreenRec));
    FULLSCREEN_PRIV(pScreen) = fsDisplayInfo;

    displayInfo->displayCount = 1;
    displayInfo->displayIDs = xrealloc(displayInfo->displayIDs,
                                      1 * sizeof(CGDirectDisplayID));
    displayInfo->displayIDs[0] = cgID;

    fsDisplayInfo->displayID = cgID;
    fsDisplayInfo->xDisplayMode = 0;
    fsDisplayInfo->aquaDisplayMode = 0;
    fsDisplayInfo->xPalette = 0;
    fsDisplayInfo->aquaPalette = 0;

    // Capture full screen because X doesn't like read-only framebuffer.
    // We need to do this before we (potentially) switch the display mode.
    CGDisplayCapture(cgID);

    if (! FSFindDisplayMode(fsDisplayInfo)) {
        ErrorF("Could not support specified display mode on screen %i.\n",
               index);
        xfree(fsDisplayInfo);
        return FALSE;
    }

    // Don't need to flip y-coordinate as CoreGraphics treats (0, 0)
    // as the top left of main screen.
    bounds = CGDisplayBounds(cgID);
    dfb->x = bounds.origin.x;
    dfb->y = bounds.origin.y;
    dfb->width  = bounds.size.width;
    dfb->height = bounds.size.height;
    dfb->pitch = CGDisplayBytesPerRow(cgID);
    dfb->bitsPerPixel = CGDisplayBitsPerPixel(cgID);

    if (dfb->bitsPerPixel == 8) {
        if (CGDisplayCanSetPalette(cgID)) {
            dfb->colorType = PseudoColor;
        } else {
            dfb->colorType = StaticColor;
        }
        dfb->bitsPerComponent = 8;
        dfb->colorBitsPerPixel = 8;
    } else {
        dfb->colorType = TrueColor;
        dfb->bitsPerComponent = CGDisplayBitsPerSample(cgID);
        dfb->colorBitsPerPixel = CGDisplaySamplesPerPixel(cgID) *
                                 dfb->bitsPerComponent;
    }

    fsDisplayInfo->framebuffer = CGDisplayBaseAddress(cgID);

    // allocate shadow framebuffer
    fsDisplayInfo->shadowPtr = xalloc(dfb->pitch * dfb->height);
    dfb->framebuffer = fsDisplayInfo->shadowPtr;

    return TRUE;
}


/*
 * FSShadowUpdate
 *  Update the damaged regions of the shadow framebuffer on the display.
 */
static void FSShadowUpdate(
    ScreenPtr pScreen,
    shadowBufPtr pBuf)
{
    DarwinFramebufferPtr dfb = SCREEN_PRIV(pScreen);
    FSScreenPtr fsDisplayInfo = FULLSCREEN_PRIV(pScreen);
    RegionPtr damage = &pBuf->damage;
    int numBox = REGION_NUM_RECTS(damage);
    BoxPtr pBox = REGION_RECTS(damage);
    int pitch = dfb->pitch;
    int bpp = dfb->bitsPerPixel/8;

    // Don't update if the X server is not visible
    if (!quartzServerVisible)
        return;

    // Loop through all the damaged boxes
    while (numBox--) {
        int width, height, offset;
        unsigned char *src, *dst;

        width = (pBox->x2 - pBox->x1) * bpp;
        height = pBox->y2 - pBox->y1;
        offset = (pBox->y1 * pitch) + (pBox->x1 * bpp);
        src = fsDisplayInfo->shadowPtr + offset;
        dst = fsDisplayInfo->framebuffer + offset;

        while (height--) {
            memcpy(dst, src, width);
            dst += pitch;
            src += pitch;
        }

        // Get the next box
        pBox++;
    }
}


/*
 * FSSetupScreen
 *  Finalize full screen specific setup of each screen.
 */
static Bool FSSetupScreen(
    int index,
    ScreenPtr pScreen)
{
    DarwinFramebufferPtr dfb = SCREEN_PRIV(pScreen);
    FSScreenPtr fsDisplayInfo = FULLSCREEN_PRIV(pScreen);
    CGDirectDisplayID cgID = fsDisplayInfo->displayID;

    // Initialize shadow framebuffer support
    if (! shadowInit(pScreen, FSShadowUpdate, NULL)) {
        ErrorF("Failed to initalize shadow framebuffer for screen %i.\n",
               index);
        return FALSE;
    }

    if (dfb->colorType == PseudoColor) {
        // Initialize colormap handling
        size_t aquaBpp;

        // If Aqua is using 8 bits we need to keep track of its pallete.
        CFNumberGetValue(CFDictionaryGetValue(fsDisplayInfo->aquaDisplayMode,
                         kCGDisplayBitsPerPixel), kCFNumberLongType, &aquaBpp);
        if (aquaBpp <= 8)
            fsDisplayInfo->aquaPalette = CGPaletteCreateWithDisplay(cgID);

        pScreen->CreateColormap = FSCreateColormap;
        pScreen->DestroyColormap = FSDestroyColormap;
        pScreen->InstallColormap = FSInstallColormap;
        pScreen->StoreColors = FSStoreColors;

    }

    quartzScreens[quartzNumScreens++] = fsDisplayInfo;
    return TRUE;
}


/*
 * Quartz display mode function list.
 */
static QuartzModeProcsRec fsModeProcs = {
    FSDisplayInit,
    FSAddScreen,
    FSSetupScreen,
    NULL,		// Not needed
    QuartzInitCursor,
    QuartzReallySetCursor,
    FSSuspendScreen,
    FSResumeScreen,
    FSCapture,
    FSRelease,
    NULL,		// No rootless code in fullscreen
    NULL,
    NULL,
    NULL,
    NULL,		// No support for DRI surfaces
    NULL
};


/*
 * QuartzModeBundleInit
 *  Initialize the display mode bundle after loading.
 */
Bool
QuartzModeBundleInit(void)
{
    quartzProcs = &fsModeProcs;
    quartzOpenGLBundle = NULL;	// Only Mesa support for now
    return TRUE;
}
