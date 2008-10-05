/* main.m
 $Id: main.m,v 1.29 2007-04-07 20:39:03 jharper Exp $
 
 Copyright (c) 2002, 2008 Apple Computer, Inc. All rights reserved. */

#include "pbproxy.h"
#import "x-selection.h"

#include <pthread.h>
#include <X11/extensions/applewm.h>

Display *x_dpy;
int x_apple_wm_event_base, x_apple_wm_error_base;

x_selection *_selection_object;

static int x_io_error_handler (Display *dpy) {
    /* We lost our connection to the server. */
    
    TRACE ();

    /* TODO: trigger the thread to restart? */
#ifndef INTEGRATED_XPBPROXY
    exit(1);
#endif
    
    return 0;
}

static int x_error_handler (Display *dpy, XErrorEvent *errevent) {
    return 0;
}

void x_init (void) {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    x_dpy = XOpenDisplay (NULL);
    if (x_dpy == NULL) {
        fprintf (stderr, "can't open default display\n");
        exit (1);
    }
    
    XSetIOErrorHandler (x_io_error_handler);
    XSetErrorHandler (x_error_handler);
    
    if (!XAppleWMQueryExtension (x_dpy, &x_apple_wm_event_base,
                                 &x_apple_wm_error_base)) {
        fprintf (stderr, "can't open AppleWM server extension\n");
        exit (1);
    }
    
    XAppleWMSelectInput (x_dpy, AppleWMActivationNotifyMask |
                         AppleWMPasteboardNotifyMask);
    
    _selection_object = [[x_selection alloc] init];
    
    x_input_register ();
    x_input_run ();

    [pool release];
}

id x_selection_object (void) {
    return _selection_object;
}

Time x_current_timestamp (void) {
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
