/*
 * Xplugin rootless implementation screen functions
 */
/*
 * Copyright (c) 2002 Apple Computer, Inc. All Rights Reserved.
 * Copyright (c) 2003 Torrey T. Lyons. All Rights Reserved.
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
/* $XFree86: xc/programs/Xserver/hw/darwin/quartz/xpr/xprScreen.c,v 1.9 2003/11/27 01:59:53 torrey Exp $ */

#include "quartzCommon.h"
#include "quartz.h"
#include "xpr.h"
#include "pseudoramiX.h"
#include "darwin.h"
#include "rootless.h"
#include "safeAlpha.h"
#include "dri.h"
#include "globals.h"
#include "Xplugin.h"
#include "applewmExt.h"

// Name of GLX bundle for native OpenGL
static const char *xprOpenGLBundle = "glxCGL.bundle";


/*
 * eventHandler
 *  Callback handler for Xplugin events.
 */
static void
eventHandler(unsigned int type, const void *arg,
             unsigned int arg_size, void *data)
{
    switch (type)
    {
    case XP_EVENT_DISPLAY_CHANGED:
	QuartzMessageServerThread(kXDarwinDisplayChanged, 0);
	break;

    case XP_EVENT_WINDOW_STATE_CHANGED:
	if (arg_size >= sizeof(xp_window_state_event))
        {
	    const xp_window_state_event *ws_arg = arg;
	    QuartzMessageServerThread(kXDarwinWindowState, 2,
                                      ws_arg->id, ws_arg->state);
	}
	break;

    case XP_EVENT_WINDOW_MOVED:
	if (arg_size == sizeof(xp_window_id))
	{
	    xp_window_id id = * (xp_window_id *) arg;

	    QuartzMessageServerThread(kXDarwinWindowMoved, 1, id);
	}
	break;

    case XP_EVENT_SURFACE_DESTROYED:
    case XP_EVENT_SURFACE_CHANGED:
	if (arg_size == sizeof(xp_surface_id))
	{
	    int kind;

	    if (type == XP_EVENT_SURFACE_DESTROYED)
		kind = AppleDRISurfaceNotifyDestroyed;
	    else
		kind = AppleDRISurfaceNotifyChanged;

	    DRISurfaceNotify(*(xp_surface_id *) arg, kind);
	}
	break;
    }
}


/*
 * displayScreenBounds
 *  Return the display ID for a particular display index.
 */
static CGDirectDisplayID
displayAtIndex(int index)
{
    CGError err;
    CGDisplayCount cnt;
    CGDirectDisplayID dpy[index+1];

    err = CGGetActiveDisplayList(index + 1, dpy, &cnt);
    if (err == kCGErrorSuccess && cnt == index + 1)
        return dpy[index];
    else
        return kCGNullDirectDisplay;
}


/*
 * displayScreenBounds
 *  Return the bounds of a particular display.
 */
static CGRect
displayScreenBounds(CGDirectDisplayID id)
{
    CGRect frame;

    frame = CGDisplayBounds(id);

    /* Remove menubar to help standard X11 window managers. */

    if (frame.origin.x == 0 && frame.origin.y == 0)
    {
        frame.origin.y += aquaMenuBarHeight;
        frame.size.height -= aquaMenuBarHeight;
    }

    return frame;
}


/*
 * addPseudoramiXScreens
 *  Add a physical screen with PseudoramiX.
 */
static void
addPseudoramiXScreens(int *x, int *y, int *width, int *height)
{
    CGDisplayCount i, displayCount;
    CGDirectDisplayID *displayList = NULL;
    CGRect unionRect = CGRectNull, frame;

    // Find all the CoreGraphics displays
    CGGetActiveDisplayList(0, NULL, &displayCount);
    displayList = xalloc(displayCount * sizeof(CGDirectDisplayID));
    CGGetActiveDisplayList(displayCount, displayList, &displayCount);

    /* Get the union of all screens */
    for (i = 0; i < displayCount; i++)
    {
        CGDirectDisplayID dpy = displayList[i];
        frame = displayScreenBounds(dpy);
        unionRect = CGRectUnion(unionRect, frame);
    }

    /* Use unionRect as the screen size for the X server. */
    *x = unionRect.origin.x;
    *y = unionRect.origin.y;
    *width = unionRect.size.width;
    *height = unionRect.size.height;

    /* Tell PseudoramiX about the real screens. */
    for (i = 0; i < displayCount; i++)
    {
        CGDirectDisplayID dpy = displayList[i];

        frame = displayScreenBounds(dpy);

        ErrorF("PseudoramiX screen %d added: %dx%d @ (%d,%d).\n", i,
               (int)frame.size.width, (int)frame.size.height,
               (int)frame.origin.x, (int)frame.origin.y);

        frame.origin.x -= unionRect.origin.x;
        frame.origin.y -= unionRect.origin.y;

        ErrorF("PseudoramiX screen %d placed at X11 coordinate (%d,%d).\n",
               i, (int)frame.origin.x, (int)frame.origin.y);

        PseudoramiXAddScreen(frame.origin.x, frame.origin.y,
                             frame.size.width, frame.size.height);
    }

    xfree(displayList);
}


/*
 * xprDisplayInit
 *  Find number of CoreGraphics displays and initialize Xplugin.
 */
static void
xprDisplayInit(void)
{
    CGDisplayCount displayCount;

    ErrorF("Display mode: Rootless Quartz -- Xplugin implementation\n");

    CGGetActiveDisplayList(0, NULL, &displayCount);

    /* With PseudoramiX, the X server only sees one screen; only PseudoramiX
       itself knows about all of the screens. */

    if (noPseudoramiXExtension)
        darwinScreensFound = displayCount;
    else
        darwinScreensFound =  1;

    if (xp_init(XP_IN_BACKGROUND) != Success)
    {
        FatalError("Could not initialize the Xplugin library.");
    }

    xp_select_events(XP_EVENT_DISPLAY_CHANGED
                     | XP_EVENT_WINDOW_STATE_CHANGED
                     | XP_EVENT_WINDOW_MOVED
                     | XP_EVENT_SURFACE_CHANGED
                     | XP_EVENT_SURFACE_DESTROYED,
                     eventHandler, NULL);

    AppleDRIExtensionInit();
    xprAppleWMInit();
}


/*
 * xprAddScreen
 *  Init the framebuffer and record pixmap parameters for the screen.
 */
static Bool
xprAddScreen(int index, ScreenPtr pScreen)
{
    DarwinFramebufferPtr dfb = SCREEN_PRIV(pScreen);

    /* If no specific depth chosen, look for the depth of the main display.
       Else if 16bpp specified, use that. Else use 32bpp. */

    dfb->colorType = TrueColor;
    dfb->bitsPerComponent = 8;
    dfb->bitsPerPixel = 32;
    dfb->colorBitsPerPixel = 24;

    if (darwinDesiredDepth == -1)
    {
        dfb->bitsPerComponent = CGDisplayBitsPerSample(kCGDirectMainDisplay);
        dfb->bitsPerPixel = CGDisplayBitsPerPixel(kCGDirectMainDisplay);
        dfb->colorBitsPerPixel =
                CGDisplaySamplesPerPixel(kCGDirectMainDisplay) *
                dfb->bitsPerComponent;
    }
    else if (darwinDesiredDepth == 15)
    {
        dfb->bitsPerComponent = 5;
        dfb->bitsPerPixel = 16;
        dfb->colorBitsPerPixel = 15;
    }
    else if (darwinDesiredDepth == 8)
    {
        dfb->colorType = PseudoColor;
        dfb->bitsPerComponent = 8;
        dfb->bitsPerPixel = 8;
        dfb->colorBitsPerPixel = 8;
    }

    if (noPseudoramiXExtension)
    {
        CGDirectDisplayID dpy;
        CGRect frame;

        dpy = displayAtIndex(index);

        frame = displayScreenBounds(dpy);

        dfb->x = frame.origin.x;
        dfb->y = frame.origin.y;
        dfb->width =  frame.size.width;
        dfb->height = frame.size.height;
    }
    else
    {
        addPseudoramiXScreens(&dfb->x, &dfb->y, &dfb->width, &dfb->height);
    }

    /* Passing zero width (pitch) makes miCreateScreenResources set the
       screen pixmap to the framebuffer pointer, i.e. NULL. The generic
       rootless code takes care of making this work. */
    dfb->pitch = 0;
    dfb->framebuffer = NULL;

    DRIScreenInit(pScreen);

    return TRUE;
}


/*
 * xprSetupScreen
 *  Setup the screen for rootless access.
 */
static Bool
xprSetupScreen(int index, ScreenPtr pScreen)
{
    // Add alpha protecting replacements for fb screen functions
    pScreen->PaintWindowBackground = SafeAlphaPaintWindow;
    pScreen->PaintWindowBorder = SafeAlphaPaintWindow;

#ifdef RENDER
    {
        PictureScreenPtr ps = GetPictureScreen(pScreen);
        ps->Composite = SafeAlphaComposite;
    }
#endif /* RENDER */

    // Initialize generic rootless code
    if (!xprInit(pScreen))
        return FALSE;

    return DRIFinishScreenInit(pScreen);
}


/*
 * xprInitInput
 *  Finalize xpr specific setup.
 */
static void
xprInitInput(int argc, char **argv)
{
    int i;

    rootlessGlobalOffsetX = darwinMainScreenX;
    rootlessGlobalOffsetY = darwinMainScreenY;

    for (i = 0; i < screenInfo.numScreens; i++)
        AppleWMSetScreenOrigin(WindowTable[i]);
}


/*
 * Quartz display mode function list.
 */
static QuartzModeProcsRec xprModeProcs = {
    xprDisplayInit,
    xprAddScreen,
    xprSetupScreen,
    xprInitInput,
    QuartzInitCursor,
    NULL,               // No need to update cursor
    QuartzSuspendXCursor,
    QuartzResumeXCursor,
    NULL,               // No capture or release in rootless mode
    NULL,
    xprIsX11Window,
    xprHideWindows,
    RootlessFrameForWindow,
    TopLevelParent,
    DRICreateSurface,
    DRIDestroySurface
};


/*
 * QuartzModeBundleInit
 *  Initialize the display mode bundle after loading.
 */
Bool
QuartzModeBundleInit(void)
{
    quartzProcs = &xprModeProcs;
    quartzOpenGLBundle = xprOpenGLBundle;
    return TRUE;
}
