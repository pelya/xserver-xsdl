/* main.m
 $Id: main.m,v 1.29 2007-04-07 20:39:03 jharper Exp $
 
 Copyright (c) 2002 Apple Computer, Inc. All rights reserved. */

#include "pbproxy.h"
#import "x-selection.h"

#include <pthread.h>

#include <X11/extensions/applewm.h>
#include <HIServices/CoreDockServices.h>

Display *x_dpy;
int x_apple_wm_event_base, x_apple_wm_error_base;

Atom x_atom_wm_state, x_atom_wm_protocols, x_atom_wm_delete_window;
Atom x_atom_clipboard, x_atom_text, x_atom_utf8_string;
Atom x_atom_targets, x_atom_multiple, x_atom_cstring;

static int x_grab_count;
static Bool x_grab_synced;

static BOOL _is_active = YES;		/* FIXME: should query server */

static x_selection *_selection_object;

/* X11 code */
static void x_error_shutdown(void);

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
    
	x_error_shutdown ();
    
    return 0;
}

static void x_init (void) {
    x_dpy = XOpenDisplay (NULL);
    if (x_dpy == NULL) {
        fprintf (stderr, "can't open default display\n");
        exit (1);
    }
    
    XSetIOErrorHandler (x_io_error_handler);
    x_atom_clipboard = XInternAtom (x_dpy, "CLIPBOARD", False);
    x_atom_text = XInternAtom (x_dpy, "TEXT", False);
    x_atom_utf8_string = XInternAtom (x_dpy, "UTF8_STRING", False);
    x_atom_targets = XInternAtom (x_dpy, "TARGETS", False);
    x_atom_multiple = XInternAtom (x_dpy, "MULTIPLE", False);
    x_atom_cstring = XInternAtom (x_dpy, "CSTRING", False);
        
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
}

static void x_shutdown (void) {
    [_selection_object release];
    _selection_object = nil;

    XCloseDisplay (x_dpy);
    x_dpy = NULL;
    exit(0);
}

static void x_error_shutdown (void) {
    exit(1);
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

/* Startup */
static void signal_handler (int sig) {
    x_shutdown ();
}

int main (int argc, const char *argv[]) {
    NSAutoreleasePool *pool;

    pool = [[NSAutoreleasePool alloc] init];
    
    x_init ();
    
    signal (SIGINT, signal_handler);
    signal (SIGTERM, signal_handler);
    signal (SIGPIPE, SIG_IGN);
    
    while (1) {
        NS_DURING
        CFRunLoopRun ();
        NS_HANDLER
        NSString *s = [NSString stringWithFormat:@"%@ - %@",
                       [localException name], [localException reason]];
        fprintf(stderr, "quartz-wm: caught exception: %s\n", [s UTF8String]);
        NS_ENDHANDLER
    }		
    
    return 0;
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
