/* app-main.m
 $Id: main.m,v 1.29 2007-04-07 20:39:03 jharper Exp $
 
 Copyright (c) 2002, 2008 Apple Computer, Inc. All rights reserved. */

#include "pbproxy.h"
#import "x-selection.h"

#include <pthread.h>
#include <unistd.h> /*for getpid*/
#include <Cocoa/Cocoa.h>

BOOL prefs_reload = NO;

static void signal_handler (int sig) {
    switch(sig) {
        case SIGHUP:
            prefs_reload = YES;
            break;
        default:
            _exit(EXIT_SUCCESS);
    }
}

int main (int argc, const char *argv[]) {
#ifdef TEST
    printf("pid: %u\n", getpid());
#endif

    if(!x_init())
        return EXIT_FAILURE;
    
    signal (SIGINT, signal_handler);
    signal (SIGTERM, signal_handler);
    signal (SIGHUP, signal_handler);
    signal (SIGPIPE, SIG_IGN);

    [NSApplication sharedApplication];
    [NSApp run];
    
    return EXIT_SUCCESS;
}
