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
    CFBundleRef bundle = NULL;
    OSStatus osstatus = LSFindApplicationForInfo(kLSUnknownCreator, CFSTR(kX11AppBundleId), nil, nil, &appURL);
    UInt32 ver;

    switch (osstatus) {
        case noErr:
            if (appURL == NULL) {
                fprintf(stderr, "Xquartz: Invalid response from LSFindApplicationForInfo(%s)\n", 
                        kX11AppBundleId);
                exit(1);
            }

            bundle = CFBundleCreate(NULL, appURL);
            if(!bundle) {
                fprintf(stderr, "Xquartz: Null value returned from CFBundleCreate().\n");
                exit(2);                
            }

            if (!CFURLGetFileSystemRepresentation(appURL, true, (unsigned char *)x11_path, sizeof(x11_path))) {
                fprintf(stderr, "Xquartz: Error resolving URL for %s\n", kX11AppBundleId);
                exit(3);
            }

            ver = CFBundleGetVersionNumber(bundle);
            if(ver < 0x02308000) {
                CFStringRef versionStr = CFBundleGetValueForInfoDictionaryKey(bundle, kCFBundleVersionKey);
                const char * versionCStr = "Unknown";

                if(versionStr) 
                    versionCStr = CFStringGetCStringPtr(versionStr, kCFStringEncodingMacRoman);

                fprintf(stderr, "Xquartz: Could not find a new enough X11.app LSFindApplicationForInfo() returned\n");
                fprintf(stderr, "         X11.app = %s\n", x11_path);
                fprintf(stderr, "         Version = %s (%x), Expected Version > 2.3.0\n", versionCStr, (unsigned)ver);
                exit(9);
            }

            strlcat(x11_path, kX11AppBundlePath, sizeof(x11_path));
#ifdef DEBUG
            fprintf(stderr, "Xquartz: X11.app = %s\n", x11_path);
#endif
            break;
        case kLSApplicationNotFoundErr:
            fprintf(stderr, "Xquartz: Unable to find application for %s\n", kX11AppBundleId);
            exit(10);
        default:
            fprintf(stderr, "Xquartz: Unable to find application for %s, error code = %d\n", 
                    kX11AppBundleId, (int)osstatus);
            exit(11);
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
