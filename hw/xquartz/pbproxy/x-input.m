/* x-input.m -- event handling
 $Id: x-input.m,v 1.26 2007-04-07 20:39:03 jharper Exp $
 
 Copyright (c) 2002 Apple Computer, Inc. All rights reserved. */

#include "pbproxy.h"
#import "x-selection.h"

#include <CoreFoundation/CFSocket.h>
#include <CoreFoundation/CFRunLoop.h>

#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/extensions/applewm.h>

#include <unistd.h>

/* FIXME: .. */
CFRunLoopSourceRef x_dpy_source;

/* Timestamp when the X server last told us it's active */
static Time last_activation_time;

static void x_event_apple_wm_notify(XAppleWMNotifyEvent *e) {

    switch (e->type - x_apple_wm_event_base) {              
        case AppleWMActivationNotify:
            switch (e->kind) {
                case AppleWMIsActive:
                    last_activation_time = e->time;
                    x_set_is_active (YES);
                    [x_selection_object () x_active:e->time];
                    break;
                    
                case AppleWMIsInactive:
                    x_set_is_active (NO);
                    [x_selection_object () x_inactive:e->time];
                    break;
            }
            break;
            
        case AppleWMPasteboardNotify:
            switch (e->kind) {
                case AppleWMCopyToPasteboard:
                    [x_selection_object () x_copy:e->time];
            }
            break;
    }
}

void x_input_run (void) {

    while (XPending (x_dpy) != 0) {
        XEvent e;       

        XNextEvent (x_dpy, &e);
        
        switch (e.type) {                
            case SelectionClear:
	        [x_selection_object () clear_event:&e.xselectionclear];
                break;
                
            case SelectionRequest:
                [x_selection_object () request_event:&e.xselectionrequest];
                break;
                
            case SelectionNotify:
                [x_selection_object () notify_event:&e.xselection];
                break;
                
	    case PropertyNotify:
		[x_selection_object () property_event:&e.xproperty];
		break;

            default:
                if (e.type - x_apple_wm_event_base >= 0
                    && e.type - x_apple_wm_event_base < AppleWMNumberEvents) {
                    x_event_apple_wm_notify ((XAppleWMNotifyEvent *) &e);
                }
                break;
        }

	XFlush(x_dpy);
    }
}

static int add_input_socket (int sock, CFOptionFlags callback_types,
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
    x_input_run ();
}

void x_input_register(void) {
    if (!add_input_socket (ConnectionNumber (x_dpy), kCFSocketReadCallBack,
                           x_input_callback, NULL, &x_dpy_source)) {
        exit (1);
    }
}

