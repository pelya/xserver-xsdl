/* x-selection.h -- proxies between NSPasteboard and X11 selections
   $Id: x-selection.h,v 1.2 2002-12-13 00:21:00 jharper Exp $

   Copyright (c) 2002 Apple Computer, Inc. All rights reserved.

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

#ifndef X_SELECTION_H
#define X_SELECTION_H 1

#include "pbproxy.h"
#include <AppKit/NSPasteboard.h>

/* This stores image data or text. */
struct propdata {
	unsigned char *data;
	size_t length;
};

@interface x_selection : NSObject
{
@private

    /* The unmapped window we use for fetching selections. */
    Window _selection_window;

    /* Cached general pasteboard and array of types we can handle. */
    NSPasteboard *_pasteboard;
    NSArray *_known_types;

    /* Last time we declared anything on the pasteboard. */
    int _my_last_change;

    /* Name of the selection we're proxying onto the pasteboard. */
    Atom _proxied_selection;

    /* When true, we're expecting a SelectionNotify event. */
    unsigned int _pending_notify :1;
 
    Atom request_atom;
    
    struct {
        struct propdata propdata;
        Window requestor;
        Atom selection;
    } pending;
}

- (void) x_active:(Time)timestamp;
- (void) x_inactive:(Time)timestamp;

- (void) x_copy:(Time)timestamp;

- (void) clear_event:(XSelectionClearEvent *)e;
- (void) request_event:(XSelectionRequestEvent *)e;
- (void) notify_event:(XSelectionEvent *)e;
- (void) property_event:(XPropertyEvent *)e;
- (void) handle_selection:(Atom)selection type:(Atom)type propdata:(struct propdata *)pdata;
- (void) claim_clipboard;
- (void) set_clipboard_manager;
- (void) own_clipboard;

@end

#endif /* X_SELECTION_H */
