/*
 * quartzRandR.h
 *
 * Copyright (c) 2010 Jan Hauffa.
 *               2010 Apple Inc.
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

#ifndef _QUARTZRANDR_H_
#define _QUARTZRANDR_H_

typedef struct {
    size_t width, height;
    int refresh;
    const void *ref;
} QuartzModeInfo, *QuartzModeInfoPtr;

// Quartz specific per screen storage structure
typedef struct {
    // List of CoreGraphics displays that this X11 screen covers.
    // This is more than one CG display for video mirroring and
    // rootless PseudoramiX mode.
    // No CG display will be covered by more than one X11 screen.
    int displayCount;
    CGDirectDisplayID *displayIDs;
    QuartzModeInfo rootlessMode, fullScreenMode, currentMode;
} QuartzScreenRec, *QuartzScreenPtr;

#define QUARTZ_PRIV(pScreen) \
    ((QuartzScreenPtr)dixLookupPrivate(&pScreen->devPrivates, quartzScreenKey))

void QuartzCopyDisplayIDs(ScreenPtr pScreen,
                          int displayCount, CGDirectDisplayID *displayIDs);

Bool QuartzRandRUpdateFakeModes (BOOL force_update);
Bool QuartzRandRInit (ScreenPtr pScreen);

#endif
