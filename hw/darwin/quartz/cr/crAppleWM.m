/*
 * Cocoa rootless implementation functions for AppleWM extension
 */
/*
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
/* $XFree86: xc/programs/Xserver/hw/darwin/quartz/cr/crAppleWM.m,v 1.1 2003/09/16 00:36:14 torrey Exp $ */

#include "quartzCommon.h"
#include "cr.h"

#undef BOOL
#define BOOL xBOOL
#include "rootless.h"
#include "X.h"
#define _APPLEWM_SERVER_
#include "applewm.h"
#include "applewmExt.h"
#undef BOOL

#define StdDocumentStyleMask (NSTitledWindowMask | \
                              NSClosableWindowMask | \
                              NSMiniaturizableWindowMask | \
                              NSResizableWindowMask)

static int
CRDisableUpdate(void)
{
    return Success;
}


static int
CREnableUpdate(void)
{
    return Success;
}


static int CRSetWindowLevel(
    WindowPtr pWin,
    int level)
{
    CRWindowPtr crWinPtr;

    crWinPtr = (CRWindowPtr) RootlessFrameForWindow(pWin, TRUE);
    if (crWinPtr == 0)
        return BadWindow;

    RootlessStopDrawing(pWin, FALSE);

    [crWinPtr->window setLevel:level];

    return Success;
}


static int CRFrameGetRect(
    int type,
    int class,
    const BoxRec *outer,
    const BoxRec *inner,
    BoxRec *ret)
{
    return Success;
}


static int CRFrameHitTest(
    int class,
    int x,
    int y,
    const BoxRec *outer,
    const BoxRec *inner,
    int *ret)
{
    return 0;
}


static int CRFrameDraw(
    WindowPtr pWin,
    int class,
    unsigned int attr,
    const BoxRec *outer,
    const BoxRec *inner,
    unsigned int title_len,
    const unsigned char *title_bytes)
{
    CRWindowPtr crWinPtr;
    NSWindow *window;
    Bool hasResizeIndicator;

    /* We assume the window has not yet been framed so
       RootlessFrameForWindow() will cause it to be. Record the window
       style so that the appropriate one will be used when it is framed.
       If the window is already framed, we can't change the window
       style and the following will have no effect. */

    nextWindowToFrame = pWin;
    if (class == AppleWMFrameClassDocument)
        nextWindowStyle = StdDocumentStyleMask;
    else
        nextWindowStyle = NSBorderlessWindowMask;

    crWinPtr = (CRWindowPtr) RootlessFrameForWindow(pWin, TRUE);
    if (crWinPtr == 0)
        return BadWindow;

    window = crWinPtr->window;

    [window setTitle:[NSString stringWithCString:title_bytes
                               length:title_len]];

    hasResizeIndicator = (attr & AppleWMFrameGrowBox) ? YES : NO;
    [window setShowsResizeIndicator:hasResizeIndicator];

    return Success;
}


static AppleWMProcsRec crAppleWMProcs = {
    CRDisableUpdate,
    CREnableUpdate,
    CRSetWindowLevel,
    CRFrameGetRect,
    CRFrameHitTest,
    CRFrameDraw
};


void CRAppleWMInit(void)
{
    AppleWMExtensionInit(&crAppleWMProcs);
}
