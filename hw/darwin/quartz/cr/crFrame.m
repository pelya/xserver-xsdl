/*
 * Cocoa rootless implementation frame functions
 */
/*
 * Copyright (c) 2001 Greg Parker. All Rights Reserved.
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
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written authorization.
 */
/* $XFree86: xc/programs/Xserver/hw/darwin/quartz/cr/crFrame.m,v 1.8 2003/12/05 07:00:10 torrey Exp $ */

#include "quartzCommon.h"
#include "cr.h"

#undef BOOL
#define BOOL xBOOL
#include "rootless.h"
#undef BOOL

WindowPtr nextWindowToFrame = NULL;
unsigned int nextWindowStyle = 0;

static void CRReshapeFrame(RootlessFrameID wid, RegionPtr pShape);


/*
 * CRCreateFrame
 *  Create a new physical window.
 *  Rootless windows must not autodisplay! Autodisplay can cause a deadlock.
 *    Event thread - autodisplay: locks view hierarchy, then window
 *    X Server thread - window resize: locks window, then view hierarchy
 *  Deadlock occurs if each thread gets one lock and waits for the other.
 */
static Bool
CRCreateFrame(RootlessWindowPtr pFrame, ScreenPtr pScreen,
              int newX, int newY, RegionPtr pShape)
{
    CRWindowPtr crWinPtr;
    NSRect bounds;
    NSWindow *theWindow;
    XView *theView;
    unsigned int theStyleMask = NSBorderlessWindowMask;

    crWinPtr = (CRWindowPtr) xalloc(sizeof(CRWindowRec));

    bounds = NSMakeRect(newX,
                        NSHeight([[NSScreen mainScreen] frame]) -
                        newY - pFrame->height,
                        pFrame->width, pFrame->height);

    // Check if AppleWM has specified a style for this window
    if (pFrame->win == nextWindowToFrame) {
        theStyleMask = nextWindowStyle;
    }
    nextWindowToFrame = NULL;

    // Create an NSWindow for the new X11 window
    theWindow = [[NSWindow alloc] initWithContentRect:bounds
                                  styleMask:theStyleMask
                                  backing:NSBackingStoreBuffered
                                  defer:NO];
    if (!theWindow) return FALSE;

    [theWindow setBackgroundColor:[NSColor clearColor]];  // erase transparent
    [theWindow setAlphaValue:1.0];       // draw opaque
    [theWindow setOpaque:YES];           // changed when window is shaped

    [theWindow useOptimizedDrawing:YES]; // Has no overlapping sub-views
    [theWindow setAutodisplay:NO];       // See comment above
    [theWindow disableFlushWindow];      // We do all the flushing manually
    [theWindow setHasShadow:YES];        // All windows have shadows
    [theWindow setReleasedWhenClosed:YES]; // Default, but we want to be sure

    theView = [[XView alloc] initWithFrame:bounds];
    [theWindow setContentView:theView];
    [theWindow setInitialFirstResponder:theView];

    [theWindow setAcceptsMouseMovedEvents:YES];

    crWinPtr->window = theWindow;
    crWinPtr->view = theView;

    [theView lockFocus];
    // Fill the window with white to make sure alpha channel is set
    NSEraseRect(bounds);
    crWinPtr->port = [theView qdPort];
    crWinPtr->context = [[NSGraphicsContext currentContext] graphicsPort];
    // CreateCGContextForPort(crWinPtr->port, &crWinPtr->context);
    [theView unlockFocus];

    // Store the implementation private frame ID
    pFrame->wid = (RootlessFrameID) crWinPtr;

    // Reshape the frame if it was created shaped.
    if (pShape != NULL)
        CRReshapeFrame(pFrame->wid, pShape);

    return TRUE;
}


/*
 * CRDestroyFrame
 *  Destroy a frame.
 */
static void
CRDestroyFrame(RootlessFrameID wid)
{
    CRWindowPtr crWinPtr = (CRWindowPtr) wid;

    [crWinPtr->window orderOut:nil];
    [crWinPtr->window close];
    [crWinPtr->view release];
    free(crWinPtr);
}


/*
 * CRMoveFrame
 *  Move a frame on screen.
 */
static void
CRMoveFrame(RootlessFrameID wid, ScreenPtr pScreen, int newX, int newY)
{
    CRWindowPtr crWinPtr = (CRWindowPtr) wid;
    NSPoint topLeft;

    topLeft = NSMakePoint(newX,
                          NSHeight([[NSScreen mainScreen] frame]) - newY);

    [crWinPtr->window setFrameTopLeftPoint:topLeft];
}


/*
 * CRResizeFrame
 *  Move and resize a frame.
 */
static void
CRResizeFrame(RootlessFrameID wid, ScreenPtr pScreen,
              int newX, int newY, unsigned int newW, unsigned int newH,
              unsigned int gravity)
{
    CRWindowPtr crWinPtr = (CRWindowPtr) wid;
    NSRect bounds = NSMakeRect(newX, NSHeight([[NSScreen mainScreen] frame]) -
                               newY - newH, newW, newH);

    [crWinPtr->window setFrame:bounds display:NO];
}


/*
 * CRRestackFrame
 *  Change the frame order. Put the frame behind nextWid or on top if
 *  it is NULL. Unmapped frames are mapped by restacking them.
 */
static void
CRRestackFrame(RootlessFrameID wid, RootlessFrameID nextWid)
{
    CRWindowPtr crWinPtr = (CRWindowPtr) wid;
    CRWindowPtr crNextWinPtr = (CRWindowPtr) nextWid;

    if (crNextWinPtr) {
        int upperNum = [crNextWinPtr->window windowNumber];

        [crWinPtr->window orderWindow:NSWindowBelow relativeTo:upperNum];
    } else {
        [crWinPtr->window makeKeyAndOrderFront:nil];
    }
}


/*
 * CRReshapeFrame
 *  Set the shape of a frame.
 */
static void
CRReshapeFrame(RootlessFrameID wid, RegionPtr pShape)
{
    CRWindowPtr crWinPtr = (CRWindowPtr) wid;
    NSRect bounds = [crWinPtr->view frame];
    int winHeight = NSHeight(bounds);
    BoxRec localBox = {0, 0, NSWidth(bounds), winHeight};

    [crWinPtr->view lockFocus];

    if (pShape != NULL) {
        // Calculate the region outside the new shape.
        miInverse(pShape, pShape, &localBox);
    }

    // If window is currently shaped we need to undo the previous shape.
    if (![crWinPtr->window isOpaque]) {
        [[NSColor whiteColor] set];
        NSRectFillUsingOperation(bounds, NSCompositeDestinationAtop);
    }

    if (pShape != NULL) {
        int count = REGION_NUM_RECTS(pShape);
        BoxRec *extRects = REGION_RECTS(pShape);
        BoxRec *rects, *end;

        // Make transparent if window is now shaped.
        [crWinPtr->window setOpaque:NO];

        // Clear the areas outside the window shape
        [[NSColor clearColor] set];
        for (rects = extRects, end = extRects+count; rects < end; rects++) {
            int rectHeight = rects->y2 - rects->y1;
            NSRectFill( NSMakeRect(rects->x1,
                                   winHeight - rects->y1 - rectHeight,
                                   rects->x2 - rects->x1, rectHeight) );
        }
        [[NSGraphicsContext currentContext] flushGraphics];

        // force update of window shadow
        [crWinPtr->window setHasShadow:NO];
        [crWinPtr->window setHasShadow:YES];

    } else {
        [crWinPtr->window setOpaque:YES];
        [[NSGraphicsContext currentContext] flushGraphics];
    }

    [crWinPtr->view unlockFocus];
}


/*
 * CRUnmapFrame
 *  Unmap a frame.
 */
static void
CRUnmapFrame(RootlessFrameID wid)
{
    CRWindowPtr crWinPtr = (CRWindowPtr) wid;

    [crWinPtr->window orderOut:nil];
}


/*
 * CRStartDrawing
 *  When a window's buffer is not being drawn to, the CoreGraphics
 *  window server may compress or move it. Call this routine
 *  to lock down the buffer during direct drawing. It returns
 *  a pointer to the backing buffer.
 */
static void
CRStartDrawing(RootlessFrameID wid, char **pixelData, int *bytesPerRow)
{
    CRWindowPtr crWinPtr = (CRWindowPtr) wid;
    PixMapHandle pix;

    [crWinPtr->view lockFocus];
    crWinPtr->port = [crWinPtr->view qdPort];
    LockPortBits(crWinPtr->port);
    [crWinPtr->view unlockFocus];
    pix = GetPortPixMap(crWinPtr->port);

    *pixelData = GetPixBaseAddr(pix);
    *bytesPerRow = GetPixRowBytes(pix) & 0x3fff; // fixme is mask needed?
}


/*
 * CRStopDrawing
 *  When direct access to a window's buffer is no longer needed, this
 *  routine should be called to allow CoreGraphics to compress or
 *  move it.
 */
static void
CRStopDrawing(RootlessFrameID wid, Bool flush)
{
    CRWindowPtr crWinPtr = (CRWindowPtr) wid;

    UnlockPortBits(crWinPtr->port);

    if (flush) {
        QDFlushPortBuffer(crWinPtr->port, NULL);
    }
}


/*
 * CRUpdateRegion
 *  Flush a region from a window's backing buffer to the screen.
 */
static void
CRUpdateRegion(RootlessFrameID wid, RegionPtr pDamage)
{
    CRWindowPtr crWinPtr = (CRWindowPtr) wid;

#ifdef ROOTLESS_TRACK_DAMAGE
    int count = REGION_NUM_RECTS(pDamage);
    BoxRec *rects = REGION_RECTS(pDamage);
    BoxRec *end;

    static RgnHandle rgn = NULL;
    static RgnHandle box = NULL;

    if (!rgn) rgn = NewRgn();
    if (!box) box = NewRgn();

    for (end = rects+count; rects < end; rects++) {
        Rect qdRect;
        qdRect.left = rects->x1;
        qdRect.top = rects->y1;
        qdRect.right = rects->x2;
        qdRect.bottom = rects->y2;

        RectRgn(box, &qdRect);
        UnionRgn(rgn, box, rgn);
    }

    QDFlushPortBuffer(crWinPtr->port, rgn);

    SetEmptyRgn(rgn);
    SetEmptyRgn(box);

#else	/* !ROOTLESS_TRACK_DAMAGE */
    QDFlushPortBuffer(crWinPtr->port, NULL);
#endif
}


/*
 * CRDamageRects
 *  Mark damaged rectangles as requiring redisplay to screen.
 */
static void
CRDamageRects(RootlessFrameID wid, int count, const BoxRec *rects,
               int shift_x, int shift_y)
{
    CRWindowPtr crWinPtr = (CRWindowPtr) wid;
    const BoxRec *end;

    for (end = rects + count; rects < end; rects++) {
        Rect qdRect;
        qdRect.left = rects->x1 + shift_x;
        qdRect.top = rects->y1 + shift_y;
        qdRect.right = rects->x2 + shift_x;
        qdRect.bottom = rects->y2 + shift_y;

        QDAddRectToDirtyRegion(crWinPtr->port, &qdRect);
    }
}


static RootlessFrameProcsRec CRRootlessProcs = {
    CRCreateFrame,
    CRDestroyFrame,
    CRMoveFrame,
    CRResizeFrame,
    CRRestackFrame,
    CRReshapeFrame,
    CRUnmapFrame,
    CRStartDrawing,
    CRStopDrawing,
    CRUpdateRegion,
    CRDamageRects,
    NULL,
    NULL,
    NULL,
    NULL
};


/*
 * Initialize CR implementation
 */
Bool
CRInit(ScreenPtr pScreen)
{
    RootlessInit(pScreen, &CRRootlessProcs);

    rootless_CopyBytes_threshold = 0;
    rootless_FillBytes_threshold = 0;
    rootless_CompositePixels_threshold = 0;
    rootless_CopyWindow_threshold = 0;

    return TRUE;
}
