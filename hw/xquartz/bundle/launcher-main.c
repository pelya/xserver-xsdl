/* main.c -- X application launcher
 
 Copyright (c) 2007 Apple Inc.
 
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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <CoreFoundation/CoreFoundation.h>

#define DEFAULT_APP "/usr/X11/bin/xterm"

int launcher_main (int argc, char **argv) {
  char *command = DEFAULT_APP;
  const char *newargv[7];
  int child;
  

	CFPropertyListRef PlistRef = CFPreferencesCopyAppValue(CFSTR("app_to_run"),
									kCFPreferencesCurrentApplication);
	
	if ((PlistRef == NULL) || (CFGetTypeID(PlistRef) != CFStringGetTypeID())) {
		CFPreferencesSetAppValue(CFSTR("app_to_run"), CFSTR(DEFAULT_APP), 
								 kCFPreferencesCurrentApplication);
		CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication);
	} else {
		int len = CFStringGetLength((CFStringRef)PlistRef)+1;
		command = (char *) malloc(len);
		CFStringGetCString((CFStringRef)PlistRef, command, len,  kCFStringEncodingASCII);
		fprintf(stderr, "command=%s\n", command);
	}
	
	if (PlistRef) CFRelease(PlistRef);
	
	newargv[0] = "/usr/bin/login";
	newargv[1] = "-fp";
	newargv[2] = getlogin();
	newargv[3] = "/bin/sh";
	newargv[4] = "-c";
	newargv[5] = command;
	newargv[6] = NULL;

    child = fork();
	
    switch (child) {
    case -1:				/* error */
      perror ("fork");
      return EXIT_FAILURE;		
    case 0:				    /* child */
      execvp (newargv[0], (char **const) newargv);
      perror ("Couldn't exec");
      _exit (1);
   }
	
    return 0;
}
