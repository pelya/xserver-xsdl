/* main.m
 $Id: main.m,v 1.29 2007-04-07 20:39:03 jharper Exp $
 
 Copyright (c) 2002, 2008 Apple Computer, Inc. All rights reserved. */

#include "pbproxy.h"
#import "x-selection.h"

#include <pthread.h>
#include <X11/extensions/applewm.h>

Display *xpbproxy_dpy;
int xpbproxy_apple_wm_event_base, xpbproxy_apple_wm_error_base;
int xpbproxy_xfixes_event_base, xpbproxy_xfixes_error_base;
BOOL xpbproxy_have_xfixes;

#ifdef STANDALONE_XPBPROXY
BOOL xpbproxy_is_standalone = NO;
#endif

x_selection *_selection_object;

static int x_io_error_handler (Display *dpy) {
    /* We lost our connection to the server. */
    
    TRACE ();

    /* trigger the thread to restart?
     *   NO - this would be to a "deeper" problem, and restarts would just
     *        make things worse...
     */
#ifdef STANDALONE_XPBPROXY
    if(xpbproxy_is_standalone)
        exit(EXIT_FAILURE);
#endif

    return 0;
}

static int x_error_handler (Display *dpy, XErrorEvent *errevent) {
    return 0;
}

BOOL xpbproxy_init (void) {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    xpbproxy_dpy = XOpenDisplay (NULL);
    if (xpbproxy_dpy == NULL) {
        fprintf (stderr, "can't open default display\n");
        [pool release];
        return FALSE;
    }
    
    XSetIOErrorHandler (x_io_error_handler);
    XSetErrorHandler (x_error_handler);
    
    if (!XAppleWMQueryExtension (xpbproxy_dpy, &xpbproxy_apple_wm_event_base,
                                 &xpbproxy_apple_wm_error_base)) {
        fprintf (stderr, "can't open AppleWM server extension\n");
        [pool release];
        return FALSE;
    }
    
    xpbproxy_have_xfixes = XFixesQueryExtension(xpbproxy_dpy, &xpbproxy_xfixes_event_base, &xpbproxy_xfixes_error_base);
    
    XAppleWMSelectInput (xpbproxy_dpy, AppleWMActivationNotifyMask |
                         AppleWMPasteboardNotifyMask);
    
    _selection_object = [[x_selection alloc] init];
    
    if(!xpbproxy_input_register()) {
        [pool release];
        return FALSE;
    }

    xpbproxy_input_run();

    [pool release];
    
    return TRUE;
}

id xpbproxy_selection_object (void) {
    return _selection_object;
}

Time xpbproxy_current_timestamp (void) {
    /* FIXME: may want to fetch a timestamp from the server.. */
    return CurrentTime;
}

void debug_printf (const char *fmt, ...) {
    static int spew = -1;
    
    if (spew == -1) {
        char *x = getenv ("DEBUG");
        spew = (x != NULL && atoi (x) != 0);
    }
    
    if (spew) {
        va_list args;
        va_start(args, fmt);
        vfprintf (stderr, fmt, args);
        va_end(args);
    }
}
