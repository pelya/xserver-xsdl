/* x11-exec.c -- Find X11.app by bundle-id and exec it.  This is so launchd
   can correctly find X11.app, even if the user moved it.

 Copyright (c) 2007 Apple, Inc.
 
 Permission is hereby granted, free of charge, to any person
 obtaining a copy of this software and associated documentation files
 (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge,
 publish, distribute, sublicense, and/or sell copies of the Software,
 and to permit persons to whom the Software is furnished to do so,
 subject to the following conditions:
 
 The above copyright notice and this permission notice shall be
 included in all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT.  IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT
 HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 DEALINGS IN THE SOFTWARE.
 
 Except as contained in this notice, the name(s) of the above
 copyright holders shall not be used in advertising or otherwise to
 promote the sale, use or other dealings in this Software without
 prior written authorization. */

#include <CoreServices/CoreServices.h>
#include <stdio.h>

#define kX11AppBundleId "org.x.X11"
#define kX11AppBundlePath "/Contents/MacOS/X11"

int main(int argc, char **argv) {
  char x11_path[PATH_MAX];
  char** args = NULL;
  CFURLRef appURL = NULL;
  OSStatus osstatus = LSFindApplicationForInfo(kLSUnknownCreator, CFSTR(kX11AppBundleId), 
					       nil, nil, &appURL);
  
  switch (osstatus) {
  case noErr:
    if (appURL == NULL) {
      fprintf(stderr, "%s: Invalid response from LSFindApplicationForInfo(%s)\n", 
	      argv[0], kX11AppBundleId);
      exit(1);
    }
    if (!CFURLGetFileSystemRepresentation(appURL, true, (unsigned char *)x11_path, sizeof(x11_path))) {
      fprintf(stderr, "%s: Error resolving URL for %s\n", argv[0], kX11AppBundleId);
      exit(2);
    }
    
    args = (char**)malloc(sizeof (char*) * (argc + 1));
    strlcat(x11_path, kX11AppBundlePath, sizeof(x11_path));
    if (args) {
      int i;
      args[0] = x11_path;
      for (i = 1; i < argc; ++i) {
        args[i] = argv[i];
      }
      args[i] = NULL;
    }
    
    fprintf(stderr, "X11.app = %s\n", x11_path);
    execv(x11_path, args);
    fprintf(stderr, "Error executing X11.app (%s):", x11_path);
    perror(NULL);
    exit(3);
    break;
  case kLSApplicationNotFoundErr:
    fprintf(stderr, "%s: Unable to find application for %s\n", argv[0], kX11AppBundleId);
    exit(4);
  default:
    fprintf(stderr, "%s: Unable to find application for %s, error code = %d\n", 
	    argv[0], kX11AppBundleId, osstatus);
    exit(5);
  }
  /* not reached */
}

    
