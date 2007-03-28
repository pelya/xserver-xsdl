/*
 * NSView subclass for Mac OS X rootless X server
 *
 * Each rootless window contains an instance of this class.
 * This class handles events while drawing is handled by Carbon
 * code in the rootless Aqua implementation.
 *
 * Copyright (c) 2001 Greg Parker. All Rights Reserved.
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
/* $XFree86: xc/programs/Xserver/hw/darwin/quartz/cr/XView.m,v 1.1 2003/06/07 05:49:07 torrey Exp $ */
#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif
#import "XView.h"


@implementation XView

- (BOOL)isFlipped
{
    return NO;
}

- (BOOL)isOpaque
{
    return YES;
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (BOOL)acceptsFirstMouse:(NSEvent *)theEvent
{
    return YES;
}

- (BOOL)shouldDelayWindowOrderingForEvent:(NSEvent *)theEvent
{
    return YES;
}

- (void)mouseDown:(NSEvent *)anEvent
{
    // Only X is allowed to restack windows.
    [NSApp preventWindowOrdering];
    if (! [NSApp isActive]) {
        [NSApp activateIgnoringOtherApps:YES];
    }
    [[self nextResponder] mouseDown:anEvent];
}

@end
