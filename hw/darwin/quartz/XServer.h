//
//  XServer.h
//
/*
 * Copyright (c) 2001 Andreas Monitzer. All Rights Reserved.
 * Copyright (c) 2002-2003 Torrey T. Lyons. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the
 * sale, use or other dealings in this Software without prior written
 * authorization.
 */
/* $XFree86: xc/programs/Xserver/hw/darwin/quartz/XServer.h,v 1.8 2003/01/23 00:34:26 torrey Exp $ */

#define BOOL xBOOL
#include "Xproto.h"
#undef BOOL

#import <Cocoa/Cocoa.h>

@interface XServer : NSObject {
    // server state
    int serverState;
    NSRecursiveLock *serverLock;
    BOOL serverVisible;
    BOOL rootlessMenuBarVisible;
    BOOL queueShowServer;
    UInt32 mouseState;
    Class windowClass;

    // server event queue
    BOOL sendServerEvents;
    int eventWriteFD;

    // Aqua interface
    IBOutlet NSWindow *modeWindow;
    IBOutlet NSButton *startupModeButton;
    IBOutlet NSButton *startFullScreenButton;
    IBOutlet NSButton *startRootlessButton;
    IBOutlet NSWindow *helpWindow;
    IBOutlet NSButton *startupHelpButton;
    IBOutlet NSPanel *switchWindow;
}

- (id)init;

- (BOOL)translateEvent:(NSEvent *)anEvent;
- (BOOL)getMousePosition:(xEvent *)xe fromEvent:(NSEvent *)anEvent;

+ (void)append:(NSString *)value toEnv:(NSString *)name;

- (void)startX;
- (void)finishStartX;
- (BOOL)startXClients;
- (void)run;
- (void)toggle;
- (void)showServer:(BOOL)show;
- (void)forceShowServer:(BOOL)show;
- (void)setRootClip:(BOOL)enable;
- (void)readPasteboard;
- (void)writePasteboard;
- (void)quitServer;
- (void)sendXEvent:(xEvent *)xe;
- (void)sendShowHide:(BOOL)show;
- (void)clientProcessDone:(int)clientStatus;

// Aqua interface actions
- (IBAction)startFullScreen:(id)sender;
- (IBAction)startRootless:(id)sender;
- (IBAction)closeHelpAndShow:(id)sender;
- (IBAction)showAction:(id)sender;

// NSApplication delegate
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender;
- (void)applicationDidFinishLaunching:(NSNotification *)aNotification;
- (BOOL)applicationShouldHandleReopen:(NSApplication *)theApplication hasVisibleWindows:(BOOL)flag;
- (void)applicationWillResignActive:(NSNotification *)aNotification;
- (void)applicationWillBecomeActive:(NSNotification *)aNotification;

// NSPort delegate
- (void)handlePortMessage:(NSPortMessage *)portMessage;

@end

// X server states
enum {
    server_NotStarted,
    server_Starting,
    server_Running,
    server_Quitting,
    server_Done
};

