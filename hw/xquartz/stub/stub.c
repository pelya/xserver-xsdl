/* Copyright (c) 2008 Apple Inc.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT
 * HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above
 * copyright holders shall not be used in advertising or otherwise to
 * promote the sale, use or other dealings in this Software without
 * prior written authorization.
 */

#include <CoreServices/CoreServices.h>

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define kX11AppBundleId "org.x.X11"
#define kX11AppBundlePath "/Contents/MacOS/X11"

static char x11_path[PATH_MAX + 1];

static void set_x11_path() {
    CFURLRef appURL = NULL;
    OSStatus osstatus = LSFindApplicationForInfo(kLSUnknownCreator, CFSTR(kX11AppBundleId), nil, nil, &appURL);
    
    switch (osstatus) {
        case noErr:
            if (appURL == NULL) {
                fprintf(stderr, "xinit: Invalid response from LSFindApplicationForInfo(%s)\n", 
                        kX11AppBundleId);
                exit(1);
            }
            
            if (!CFURLGetFileSystemRepresentation(appURL, true, (unsigned char *)x11_path, sizeof(x11_path))) {
                fprintf(stderr, "xinit: Error resolving URL for %s\n", kX11AppBundleId);
                exit(2);
            }
            
            strlcat(x11_path, kX11AppBundlePath, sizeof(x11_path));
#ifdef DEBUG
            fprintf(stderr, "XQuartz: X11.app = %s\n", x11_path);
#endif
            break;
        case kLSApplicationNotFoundErr:
            fprintf(stderr, "XQuartz: Unable to find application for %s\n", kX11AppBundleId);
            exit(4);
        default:
            fprintf(stderr, "XQuartz: Unable to find application for %s, error code = %d\n", 
                    kX11AppBundleId, (int)osstatus);
            exit(5);
    }
}

#ifndef BUILD_DATE
#define BUILD_DATE "?"
#endif
#ifndef XSERVER_VERSION
#define XSERVER_VERSION "?"
#endif

int main(int argc, char **argv) {
    
    if(argc == 2 && !strcmp(argv[1], "-version")) {
        fprintf(stderr, "X.org Release 7.3\n");
        fprintf(stderr, "X.Org X Server %s\n", XSERVER_VERSION);
        fprintf(stderr, "Build Date: %s\n", BUILD_DATE);
        return 0;
    }
    
    set_x11_path();
    
    argv[0] = x11_path;
    return execvp(x11_path, argv);
}
