/**************************************************************
 *
 * Startup program for Darwin X servers
 *
 * This program selects the appropriate X server to launch:
 *  XDarwin         IOKit X server (default)
 *  XDarwinQuartz   A soft link to the Quartz X server
 *                  executable (-quartz etc. option)
 *
 * If told to idle, the program will simply pause and not
 * launch any X server. This is to support startx being
 * run by XDarwin.app.
 *
 **************************************************************/
/*
 * Copyright (c) 2001-2002 Torrey T. Lyons. All Rights Reserved.
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
 * TORREY T. LYONS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of Torrey T. Lyons shall not
 * be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from
 * Torrey T. Lyons.
 */
/* $XFree86: xc/programs/Xserver/hw/darwin/quartz/XDarwinStartup.c,v 1.1 2002/03/28 02:21:18 torrey Exp $ */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syslimits.h>
#include <ApplicationServices/ApplicationServices.h>

extern int errno;

// Macros to build the path name
#ifndef XBINDIR
#define XBINDIR /usr/X11R6/bin
#endif
#define STR(s) #s
#define XSTRPATH(s) STR(s) "/"
#define XPATH(file) XSTRPATH(XBINDIR) STR(file)

int main(
    int         argc,
    char        *argv[] )
{
    int         i, j, quartzMode = -1;
    char        **newargv;

    // Check if we are going to run in Quartz mode or idle
    // to support startx from the Quartz server. The last
    // parameter in the list is the one used.
    for (i = argc-1; i; i--) {
        if (!strcmp(argv[i], "-idle")) {
            pause();
            return 0;

        } else if (!strcmp(argv[i], "-quartz") ||
                   !strcmp(argv[i], "-rootless") ||
                   !strcmp(argv[i], "-fullscreen"))
        {
            quartzMode = 1;
            break;

        } else if (!strcmp(argv[i], "-iokit")) {
            quartzMode = 0;
            break;
        }
    }

    if (quartzMode == -1) {
#ifdef HAS_CG_MACH_PORT
        // Check if the CoreGraphics window server is running.
        // Mike Paquette says this is the fastest way to determine if it is running.
        CFMachPortRef cgMachPortRef = CGWindowServerCFMachPort();
        if (cgMachPortRef == NULL)
            quartzMode = 0;
        else
            quartzMode = 1;
#else
        // On older systems we assume IOKit mode.
        quartzMode = 0;
#endif
    }

    if (quartzMode) {
        // Launch the X server for the quartz modes

        char quartzPath[PATH_MAX+1];
        int pathLength;
        OSStatus theStatus;
        CFURLRef appURL;
        CFStringRef appPath;
        Boolean success;

        // Build the new argument list
        newargv = (char **) malloc((argc+2) * sizeof(char *));
        for (j = argc; j; j--)
            newargv[j] = argv[j];
        newargv[argc] = "-nostartx";
        newargv[argc+1] = NULL;

        // Use the XDarwinQuartz soft link if it is valid
        pathLength = readlink(XPATH(XDarwinQuartz), quartzPath, PATH_MAX);
        if (pathLength != -1) {
            quartzPath[pathLength] = '\0';
            newargv[0] = quartzPath;
            execv(newargv[0], newargv);
        }

        // Otherwise query LaunchServices for the location of the XDarwin application
        theStatus = LSFindApplicationForInfo(kLSUnknownCreator,
                                             CFSTR("org.xfree86.XDarwin"),
                                             NULL, NULL, &appURL);
        if (theStatus) {
            fprintf(stderr, "Could not find the XDarwin application. (Error = 0x%lx)\n", theStatus);
            fprintf(stderr, "Launch XDarwin once from the Finder.\n");
            return theStatus;
        }

        appPath = CFURLCopyFileSystemPath (appURL, kCFURLPOSIXPathStyle);
        success = CFStringGetCString(appPath, quartzPath, PATH_MAX, CFStringGetSystemEncoding());
        if (! success) {
            fprintf(stderr, "Could not find path to XDarwin application.\n");
            return success;
        }

        // Launch the XDarwin application
        strncat(quartzPath, "/Contents/MacOS/XDarwin", PATH_MAX);
        newargv[0] = quartzPath;
        execv(newargv[0], newargv);
        fprintf(stderr, "Could not start XDarwin application at %s.\n", newargv[0]);
        return errno;

    } else {

        // Build the new argument list
        newargv = (char **) malloc((argc+1) * sizeof(char *));
        for (j = argc; j; j--)
            newargv[j] = argv[j];
        newargv[0] = "XDarwin";
        newargv[argc] = NULL;
    
        // Launch the IOKit X server
        execvp(newargv[0], newargv);
        fprintf(stderr, "Could not start XDarwin IOKit X server.\n");
        return errno;
    }
}
