/* x-selection.m -- proxies between NSPasteboard and X11 selections
   $Id: x-selection.m,v 1.9 2006-07-07 18:24:28 jharper Exp $

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

#import "x-selection.h"

#include <X11/Xatom.h>

#include <unistd.h>

@implementation x_selection

static unsigned long *
read_prop_32 (Window id, Atom prop, int *nitems_ret)
{
    int r, format;
    Atom type;
    unsigned long nitems, bytes_after;
    unsigned char *data;

    r = XGetWindowProperty (x_dpy, id, prop, 0, 0,
			    False, AnyPropertyType, &type, &format,
			    &nitems, &bytes_after, &data);

    if (r == Success && bytes_after != 0)
    {
	XFree (data);
	r = XGetWindowProperty (x_dpy, id, prop, 0,
				(bytes_after / 4) + 1, False,
				AnyPropertyType, &type, &format,
				&nitems, &bytes_after, &data);
    }

    if (r != Success)
	return NULL;

    if (format != 32)
    {
	XFree (data);
	return NULL;
    }

    *nitems_ret = nitems;
    return (unsigned long *) data;
}

float
get_time (void)
{
  extern void Microseconds ();
  UnsignedWide usec;
  long long ll;

  Microseconds (&usec);
  ll = ((long long) usec.hi << 32) | usec.lo;

  return ll / 1e6;
}

static Bool
IfEventWithTimeout (Display *dpy, XEvent *e, int timeout,
		    Bool (*pred) (Display *, XEvent *, XPointer),
		    XPointer arg)
{
    float start = get_time ();
    fd_set fds;
    struct timeval tv;

    do {
	if (XCheckIfEvent (x_dpy, e, pred, arg))
	    return True;

	FD_ZERO (&fds);
	FD_SET (ConnectionNumber (x_dpy), &fds);
	tv.tv_usec = 0;
	tv.tv_sec = timeout;

	if (select (FD_SETSIZE, &fds, NULL, NULL, &tv) != 1)
	    break;

    } while (start + timeout > get_time ());

    return False;
}

/* Called when X11 becomes active (i.e. has key focus) */
- (void) x_active:(Time)timestamp
{
    TRACE ();

    if ([_pasteboard changeCount] != _my_last_change)
    {
	if ([_pasteboard availableTypeFromArray: _known_types] != nil)
	{
	    /* Pasteboard has data we should proxy; I think it makes
	       sense to put it on both CLIPBOARD and PRIMARY */

	    XSetSelectionOwner (x_dpy, x_atom_clipboard,
				_selection_window, timestamp);
	    XSetSelectionOwner (x_dpy, XA_PRIMARY,
				_selection_window, timestamp);
	}
    }
}

/* Called when X11 loses key focus */
- (void) x_inactive:(Time)timestamp
{
    Window w;

    TRACE ();

    if (_proxied_selection == XA_PRIMARY)
      return;

    w = XGetSelectionOwner (x_dpy, x_atom_clipboard);

    if (w != None && w != _selection_window)
    {
	/* An X client has the selection, proxy it to the pasteboard */

	_my_last_change = [_pasteboard declareTypes:_known_types owner:self];
	_proxied_selection = x_atom_clipboard;
    }
}

/* Called when the Edit/Copy item on the main X11 menubar is selected
   and no appkit window claims it. */
- (void) x_copy:(Time)timestamp
{
    Window w;

    /* Lazily copies the PRIMARY selection to the pasteboard. */

    w = XGetSelectionOwner (x_dpy, XA_PRIMARY);

    if (w != None && w != _selection_window)
    {
	XSetSelectionOwner (x_dpy, x_atom_clipboard,
			    _selection_window, timestamp);
	_my_last_change = [_pasteboard declareTypes:_known_types owner:self];
	_proxied_selection = XA_PRIMARY;
    }
    else
    {
	XBell (x_dpy, 0);
    }
}


/* X events */

- (void) clear_event:(XSelectionClearEvent *)e
{
    TRACE ();

    /* Right now we don't care about this. */
}

static Atom
convert_1 (XSelectionRequestEvent *e, NSString *data, Atom target, Atom prop)
{
    Atom ret = None;

    if (data == nil)
	return ret;

    if (target == x_atom_text)
	target = x_atom_utf8_string;

    if (target == XA_STRING
	|| target == x_atom_cstring
	|| target == x_atom_utf8_string)
    {
	const char *bytes;

	if (target == XA_STRING)
	    bytes = [data lossyCString];
	else
	    bytes = [data UTF8String];

	if (bytes != NULL)
	{
	    XChangeProperty (x_dpy, e->requestor, prop, target,
			     8, PropModeReplace, (unsigned char *) bytes,
			     strlen (bytes));
	    ret = prop;
	}
    }
    /* FIXME: handle COMPOUND_TEXT target */

    return ret;
}

- (void) request_event:(XSelectionRequestEvent *)e
{
    /* Someone's asking us for the data on the pasteboard */

    XEvent reply;
    NSString *data;
    Atom target;

    TRACE ();

    reply.xselection.type = SelectionNotify;
    reply.xselection.selection = e->selection;
    reply.xselection.target = e->target;
    reply.xselection.requestor = e->requestor;
    reply.xselection.time = e->time;
    reply.xselection.property = None;

    target = e->target;

    if (target == x_atom_targets)
    {
	long data[2];

	data[0] = x_atom_utf8_string;
	data[1] = XA_STRING;

	XChangeProperty (x_dpy, e->requestor, e->property, target,
			 8, PropModeReplace, (unsigned char *) &data,
			 sizeof (data));
	reply.xselection.property = e->property;
    }
    else if (target == x_atom_multiple)
    {
	if (e->property != None)
	{
	    int i, nitems;
	    unsigned long *atoms;

	    atoms = read_prop_32 (e->requestor, e->property, &nitems);

	    if (atoms != NULL)
	    {
		data = [_pasteboard stringForType:NSStringPboardType];

		for (i = 0; i < nitems; i += 2)
		{
		    Atom target = atoms[i], prop = atoms[i+1];

		    atoms[i+1] = convert_1 (e, data, target, prop);
		}

		XChangeProperty (x_dpy, e->requestor, e->property, target,
				 32, PropModeReplace, (unsigned char *) atoms,
				 nitems);
		XFree (atoms);
	    }
	}
    }

    data = [_pasteboard stringForType:NSStringPboardType];
    if (data != nil)
    {
	reply.xselection.property = convert_1 (e, data, target, e->property);
    }

    XSendEvent (x_dpy, e->requestor, False, 0, &reply);
}

- (void) notify_event:(XSelectionEvent *)e
{
    /* Someone sent us data we're waiting for. */

    Atom type;
    int format, r, offset;
    unsigned long nitems, bytes_after;
    unsigned char *data, *buf;
    NSString *string;

    TRACE ();

    if (e->target == x_atom_targets)
    {
	/* Was trying to fetch the TARGETS property; it lists the
	   formats supported by the selection owner. */

	unsigned long *atoms;
	int natoms;
	int i, utf8_i = -1, string_i = -1;

	if (e->property != None
	    && (atoms = read_prop_32 (e->requestor,
				      e->property, &natoms)) != NULL)
	{
	    for (i = 0; i < natoms; i++)
	    {
		if (atoms[i] == XA_STRING)
		    string_i = i;
		else if (atoms[i] == x_atom_utf8_string)
		    utf8_i = i;
	    }
	    XFree (atoms);
	}

	/* May as well try as STRING if nothing else, it can only
	   fail, and it will help broken clients who don't support
	   the TARGETS selection.. */

	if (utf8_i >= 0)
	    type = x_atom_utf8_string;
	else
	    type = XA_STRING;

	XConvertSelection (x_dpy, e->selection, type,
			   e->selection, e->requestor, e->time);
	_pending_notify = YES;
	return;
    }

    if (e->property == None)
	return;				/* FIXME: notify pasteboard? */

    /* Should be the data. Find out how big it is and what format it's in. */

    r = XGetWindowProperty (x_dpy, e->requestor, e->property,
			    0, 0, False, AnyPropertyType, &type,
			    &format, &nitems, &bytes_after, &data);
    if (r != Success)
	return;

    XFree (data);
    if (type == None || format != 8)
	return;

    bytes_after += nitems;
    
    /* Read it into a buffer. */

    buf = malloc (bytes_after + 1);
    if (buf == NULL)
	return;

    for (offset = 0; bytes_after > 0; offset += nitems)
    {
	r = XGetWindowProperty (x_dpy, e->requestor, e->property,
				offset / 4, (bytes_after / 4) + 1,
				False, AnyPropertyType, &type,
				&format, &nitems, &bytes_after, &data);
	if (r != Success)
	{
	    free (buf);
	    return;
	}

	memcpy (buf + offset, data, nitems);
	XFree (data);
    }
    buf[offset] = 0;
    XDeleteProperty (x_dpy, e->requestor, e->property);

    /* Convert to an NSString and write to the pasteboard. */

    if (type == XA_STRING)
	string = [NSString stringWithCString:(char *) buf];
    else /* if (type == x_atom_utf8_string) */
	string = [NSString stringWithUTF8String:(char *) buf];

    free (buf);

    [_pasteboard setString:string forType:NSStringPboardType];
}


/* NSPasteboard-required methods */

static Bool
selnotify_pred (Display *dpy, XEvent *e, XPointer arg)
{
    return e->type == SelectionNotify;
}

- (void) pasteboard:(NSPasteboard *)sender provideDataForType:(NSString *)type
{
    XEvent e;
    Atom request;

    TRACE ();

    /* Don't ask for the data yet, first find out which formats
       the selection owner supports. */

    request = x_atom_targets;

again:
    XConvertSelection (x_dpy, _proxied_selection, request,
		       _proxied_selection, _selection_window, CurrentTime);

    _pending_notify = YES;

    /* Seems like we need to be synchronous here.. Actually, this really
       sucks, since it means we could get deadlocked if people don't
       respond to our request. So we need to implement our own timeout
       code.. */

    while (_pending_notify
	   && IfEventWithTimeout (x_dpy, &e, 1, selnotify_pred, NULL))
    {
	_pending_notify = NO;
	[self notify_event:&e.xselection];
    }

    if (_pending_notify && request == x_atom_targets)
    {
	/* App didn't respond to request for TARGETS selection. Let's
	   try the STRING selection as a last resort.. Helps broken
	   applications (e.g. nedit, see #3199867) */

	request = XA_STRING;
	goto again;
    }

    _pending_notify = NO;
}

- (void) pasteboardChangedOwner:(NSPasteboard *)sender
{
    TRACE ();

    /* Right now we don't care with this. */
}


/* Allocation */

- init
{
    unsigned long pixel;

    self = [super init];
    if (self == nil)
	return nil;

    _pasteboard = [[NSPasteboard generalPasteboard] retain];

    _known_types = [[NSArray arrayWithObject:NSStringPboardType] retain];

    pixel = BlackPixel (x_dpy, DefaultScreen (x_dpy));
    _selection_window = XCreateSimpleWindow (x_dpy, DefaultRootWindow (x_dpy),
					     0, 0, 1, 1, 0, pixel, pixel);

    return self;
}

- (void) dealloc
{
    [_pasteboard releaseGlobally];
    [_pasteboard release];
    _pasteboard = nil;

    [_known_types release];
    _known_types = nil;

    if (_selection_window != 0)
    {
	XDestroyWindow (x_dpy, _selection_window);
	_selection_window = 0;
    }

    [super dealloc];
}

@end
