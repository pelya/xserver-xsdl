/* main.m
 $Id: main.m,v 1.29 2007-04-07 20:39:03 jharper Exp $
 
 Copyright (c) 2002 Apple Computer, Inc. All rights reserved. */

#include "pbproxy.h"
#import "x-selection.h"

#include <pthread.h>
#include <unistd.h> /*for getpid*/

#include <X11/extensions/applewm.h>

Display *x_dpy;
int x_apple_wm_event_base, x_apple_wm_error_base;

static int x_grab_count;
static Bool x_grab_synced;

static BOOL _is_active = YES;		/* FIXME: should query server */ 
/*gstaplin: why? Is there a race?*/

x_selection *_selection_object;

/* X11 code */
void x_grab_server (Bool sync) {
    if (x_grab_count++ == 0) {
        XGrabServer (x_dpy);
    }
    
    if (sync && !x_grab_synced) {
        XSync (x_dpy, False);
        x_grab_synced = True;
    }
}

void x_ungrab_server (void) {
    if (--x_grab_count == 0) {
        XUngrabServer (x_dpy);
        XFlush (x_dpy);
        x_grab_synced = False;
    }
}

static int x_io_error_handler (Display *dpy) {
    /* We lost our connection to the server. */
    
    TRACE ();

    /* TODO: tirgger the thread to restart? */
#ifndef INTEGRATED_XPBPROXY
    exit(1);
#endif
    
    return 0;
}

static int x_error_handler (Display *dpy, XErrorEvent *errevent) {
    return 0;
}

void x_init (void) {
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

    [_selection_object set_clipboard_manager];
    [_selection_object claim_clipboard];
}

id x_selection_object (void) {
    return _selection_object;
}

Time x_current_timestamp (void) {
    /* FIXME: may want to fetch a timestamp from the server.. */
    return CurrentTime;
}


/* Finding things */
BOOL x_get_is_active (void) {
    return _is_active;
}

void x_set_is_active (BOOL state) {    
    if (_is_active == state)
        return;

    _is_active = state;
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
