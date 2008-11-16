/* x-input.m -- event handling
 $Id: x-input.m,v 1.26 2007-04-07 20:39:03 jharper Exp $
 
 Copyright (c) 2002, 2008 Apple Computer, Inc. All rights reserved. */

#include "pbproxy.h"
#import "x-selection.h"

#include <CoreFoundation/CFSocket.h>
#include <CoreFoundation/CFRunLoop.h>

#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/extensions/applewm.h>

#include <unistd.h>

static CFRunLoopSourceRef xpbproxy_dpy_source;

#ifdef STANDALONE_XPBPROXY
BOOL xpbproxy_prefs_reload = NO;
#endif

/* Timestamp when the X server last told us it's active */
static Time last_activation_time;

static void x_event_apple_wm_notify(XAppleWMNotifyEvent *e) {
    int type = e->type - xpbproxy_apple_wm_event_base;
    int kind = e->kind;

    /* We want to reload prefs even if we're not active */
    if(type == AppleWMActivationNotify &&
       kind == AppleWMReloadPreferences)
        [xpbproxy_selection_object() reload_preferences];

    if(![xpbproxy_selection_object() is_active])
        return;

    switch (type) {              
        case AppleWMActivationNotify:
            switch (kind) {
                case AppleWMIsActive:
                    last_activation_time = e->time;
                    [xpbproxy_selection_object() x_active:e->time];
                    break;
                    
                case AppleWMIsInactive:
                    [xpbproxy_selection_object() x_inactive:e->time];
                    break;
            }
            break;
            
        case AppleWMPasteboardNotify:
            switch (kind) {
                case AppleWMCopyToPasteboard:
                    [xpbproxy_selection_object() x_copy:e->time];
            }
            break;
    }
}

void xpbproxy_input_run (void) {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    
    if (nil == pool) 
    {
        fprintf(stderr, "unable to allocate/init auto release pool!\n");
        return;
    }
    
    while (XPending (xpbproxy_dpy) != 0) {
        XEvent e;
        XNextEvent (xpbproxy_dpy, &e);
        
        switch (e.type) {                
            case SelectionClear:
                if([xpbproxy_selection_object() is_active])
                    [xpbproxy_selection_object () clear_event:&e.xselectionclear];
                break;
                
            case SelectionRequest:
                [xpbproxy_selection_object () request_event:&e.xselectionrequest];
                break;
                
            case SelectionNotify:
                [xpbproxy_selection_object () notify_event:&e.xselection];
                break;
                
            case PropertyNotify:
                [xpbproxy_selection_object () property_event:&e.xproperty];
                break;
                
            default:
                if(e.type >= xpbproxy_apple_wm_event_base &&
                   e.type < xpbproxy_apple_wm_event_base + AppleWMNumberEvents) {
                    x_event_apple_wm_notify((XAppleWMNotifyEvent *) &e);
                } else if(e.type == xpbproxy_xfixes_event_base + XFixesSelectionNotify) {
                    [xpbproxy_selection_object() xfixes_selection_notify:(XFixesSelectionNotifyEvent *)&e];
                }
                break;
        }
        
        XFlush(xpbproxy_dpy);
    }
    
    [pool release];
}

static BOOL add_input_socket (int sock, CFOptionFlags callback_types,
                             CFSocketCallBack callback, const CFSocketContext *ctx,
                             CFRunLoopSourceRef *cf_source) {
    CFSocketRef cf_sock;
    
    cf_sock = CFSocketCreateWithNative (kCFAllocatorDefault, sock,
                                        callback_types, callback, ctx);
    if (cf_sock == NULL) {
        close (sock);
        return FALSE;
    }
    
    *cf_source = CFSocketCreateRunLoopSource (kCFAllocatorDefault,
                                              cf_sock, 0);
    CFRelease (cf_sock);
    
    if (*cf_source == NULL)
        return FALSE;
    
    CFRunLoopAddSource (CFRunLoopGetCurrent (),
                        *cf_source, kCFRunLoopDefaultMode);
    return TRUE;
}

static void x_input_callback (CFSocketRef sock, CFSocketCallBackType type,
                              CFDataRef address, const void *data, void *info) {

#ifdef STANDALONE_XPBPROXY
    if(xpbproxy_prefs_reload) {
        [xpbproxy_selection_object() reload_preferences];
        xpbproxy_prefs_reload = NO;
    }
#endif

    xpbproxy_input_run();
}

BOOL xpbproxy_input_register(void) {
    return add_input_socket(ConnectionNumber(xpbproxy_dpy), kCFSocketReadCallBack,
                            x_input_callback, NULL, &xpbproxy_dpy_source);
}
