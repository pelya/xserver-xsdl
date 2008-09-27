/* main.m
 $Id: main.m,v 1.29 2007-04-07 20:39:03 jharper Exp $
 
 Copyright (c) 2002 Apple Computer, Inc. All rights reserved. */

#include "pbproxy.h"
#import "x-selection.h"

#include <pthread.h>
#include <unistd.h> /*for getpid*/

#include <X11/extensions/applewm.h>

/* X11 code */
static void x_shutdown (void) {
    /*gstaplin: signal_handler() calls this, and I don't think these are async-signal safe. */
    /*TODO use a socketpair() to trigger a cleanup.  This is totally unsafe according to Jordan.  It's a segfault waiting to happen on a signal*/

    [_selection_object release];
    _selection_object = nil;

    XCloseDisplay (x_dpy);
    x_dpy = NULL;
    exit(0); 
}

/* Startup */
static void signal_handler (int sig) {
    x_shutdown ();
}

int main (int argc, const char *argv[]) {
    printf("pid: %u\n", getpid());
    
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
