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

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xatom.h>

#include <unistd.h> /*GPS may not be needed now */

// These will be set by X11Controller.m once this is integrated into a server thread
BOOL pbproxy_active = YES;
BOOL pbproxy_primary_on_grab = NO; // This is provided as an option for people who want it and has issues that won't ever be addressed to make it *always* work
BOOL pbproxy_clipboard_to_pasteboard = YES;
BOOL pbproxy_pasteboard_to_primary = YES;
BOOL pbproxy_pasteboard_to_clipboard = YES;

@implementation x_selection

static struct propdata null_propdata = {NULL, 0};

static void
init_propdata (struct propdata *pdata)
{
    *pdata = null_propdata;
}

static void
free_propdata (struct propdata *pdata)
{
    free (pdata->data);
    *pdata = null_propdata;
}

/* Return True if this is an INCR-style transfer. */
static Bool
is_incr_type (XSelectionEvent *e)
{
    Atom seltype;
    int format;
    unsigned long numitems = 0UL, bytesleft = 0UL;
    unsigned char *chunk;
       
    if (Success != XGetWindowProperty (x_dpy, e->requestor, e->property,
				       /*offset*/ 0L, /*length*/ 4UL,
				       /*Delete*/ False,
				       AnyPropertyType, &seltype, &format,
				       &numitems, &bytesleft, &chunk))
    {
	return False;
    }

    if(chunk)
	XFree(chunk);

    return (seltype == atoms->incr) ? True : False;
}

/* This finds the preferred type from a TARGETS list.*/
static Atom 
find_preferred (struct propdata *pdata)
{
    Atom a = None;
    size_t i;
    Bool png = False, utf8 = False, string = False;

    if (pdata->length % sizeof (a))
    {
	fprintf(stderr, "Atom list is not a multiple of the size of an atom!\n");
	return None;
    }

    for (i = 0; i < pdata->length; i += sizeof (a))
    {
	memcpy (&a, pdata->data + i, sizeof (a));
	
	if (a == atoms->image_png)
	{
	    png = True;
	} 
	else if (a == atoms->utf8_string)
	{
	    utf8 = True;
        } 
	else if (a == atoms->string)
	{
	    string = True;
	}
    }

    /*We prefer PNG over strings, and UTF8 over a Latin-1 string.*/
    if (png)
	return atoms->image_png;

    if (utf8)
	return atoms->utf8_string;

    if (string)
	return atoms->string;

    /* This is evidently something we don't know how to handle.*/
    return None;
}

/*
 * Return True if an error occurs.  Return False if pdata has data 
 * and we finished. 
 * The property is only deleted when bytesleft is 0 if delete is True.
 */
static Bool
get_property(Window win, Atom property, struct propdata *pdata, Bool delete, Atom *type) 
{
    long offset = 0;
    unsigned long numitems, bytesleft = 0;
#ifdef TEST
    /* This is used to test the growth handling. */
    unsigned long length = 4UL;
#else
    unsigned long length = (100000UL + 3) / 4; 
#endif
    unsigned char *buf = NULL, *chunk = NULL;
    size_t buflen = 0, chunkbytesize = 0;
    int format;
    
    if(None == property)
	return True;
    
    do {
	unsigned long newbuflen;
	unsigned char *newbuf;
	
	if (Success != XGetWindowProperty (x_dpy, win, property,
					   offset, length, delete, 
					   AnyPropertyType,
					   type, &format, &numitems, 
					   &bytesleft, &chunk)) {
	    free (buf);
	    return True;
	}
	
#ifdef TEST
	printf("format %d numitems %lu bytesleft %lu\n",
	       format, numitems, bytesleft);
	
	printf("type %s\n", XGetAtomName(dis, *type));
#endif
	
	/* Format is the number of bits. */
	chunkbytesize = numitems * (format / 8);

#ifdef TEST
	printf("chunkbytesize %zu\n", chunkbytesize);
#endif
	
	newbuflen = buflen + chunkbytesize;
	newbuf = realloc (buf, newbuflen);
	if (NULL == newbuf) {
	    XFree (chunk);
	    free (buf);
	    return True;
	}
	
	memcpy (newbuf + buflen, chunk, chunkbytesize);
	XFree (chunk);
	buf = newbuf;
	buflen = newbuflen;
	/* offset is a multiple of 32 bits*/
	offset += chunkbytesize / 4;
    } while (bytesleft > 0);
    
    pdata->data = buf;
    pdata->length = buflen;

    return /*success*/ False;
}


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

/* 
 * This should be called after a selection has been copied, 
 * or when the selection is unfinished before a transfer completes. 
 */
- (void) release_pending
{
    free_propdata (&pending.propdata);
    pending.requestor = None;
    pending.selection = None;
}

/* Return True if an error occurs during an append.*/
/* Return False if the append succeeds. */
- (Bool) append_to_pending:(struct propdata *)pdata requestor:(Window)requestor
{
    unsigned char *newdata;
    size_t newlength;
    
    if (requestor != pending.requestor)
    {
	[self release_pending];
	pending.requestor = requestor;
    }
	
    newlength = pending.propdata.length + pdata->length;
    newdata = realloc(pending.propdata.data, newlength);

    if(NULL == newdata) 
    {
	perror("realloc propdata");
	[self release_pending];
        return True;
    }

    memcpy(newdata + pending.propdata.length, pdata->data, pdata->length);
    pending.propdata.data = newdata;
    pending.propdata.length = newlength;
	
    return False;
}



/* Called when X11 becomes active (i.e. has key focus) */
- (void) x_active:(Time)timestamp
{
    TRACE ();

#if 0
    if ([_pasteboard changeCount] != _my_last_change)
    {
	if ([_pasteboard availableTypeFromArray: _known_types] != nil)
	{
	    /* Pasteboard has data we should proxy; I think it makes
	       sense to put it on both CLIPBOARD and PRIMARY */

	    XSetSelectionOwner (x_dpy, atoms->clipboard,
				_selection_window, timestamp);
	    XSetSelectionOwner (x_dpy, XA_PRIMARY,
				_selection_window, timestamp);
	}
    }
#endif
}

/* Called when X11 loses key focus */
- (void) x_inactive:(Time)timestamp
{
    Window w;

    TRACE ();
#if 0
    if (_proxied_selection == XA_PRIMARY)
      return;

    w = XGetSelectionOwner (x_dpy, atoms->clipboard);

    if (w != None && w != _selection_window)
    {
	/* An X client has the selection, proxy it to the pasteboard */

	_my_last_change = [_pasteboard declareTypes:_known_types owner:self];
	_proxied_selection = atoms->clipboard;
    }
#endif
}

/* Called when the Edit/Copy item on the main X11 menubar is selected
   and no appkit window claims it. */
- (void) x_copy:(Time)timestamp
{
    Window w;

    w = XGetSelectionOwner (x_dpy, atoms->primary);

    if (None != w)
    {
	DB ("requesting targets\n");
	request_atom = atoms->targets;
	XConvertSelection (x_dpy, atoms->primary, atoms->targets,
			   atoms->primary, _selection_window, CurrentTime);
    }
    else
    {
	XBell (x_dpy, 0);
    }
}

/*
 *
 */
- (void) set_clipboard_manager
{
    if (None != XGetSelectionOwner (x_dpy, atoms->clipboard_manager))
    {
	fprintf (stderr, "A clipboard manager is already running!\n"
		 "pbproxy can not continue!\n");
	exit (EXIT_FAILURE);
    }

    XSetSelectionOwner (x_dpy, atoms->clipboard_manager, _selection_window,
			CurrentTime);
}

/*
 * This occurs when we previously owned a selection, 
 * and then lost it from another client.
 */
- (void) clear_event:(XSelectionClearEvent *)e
{
    TRACE ();
 
    if (atoms->clipboard == e->selection)
    {
	/* 
	 * We lost ownership of the CLIPBOARD.
	 */
	[self claim_clipboard];
    } 
    else if (atoms->clipboard_manager == e->selection)
    {
	/*TODO/HMM  What should we do here? */
	/* Another CLIPBOARD_MANAGER has set itself as owner.
         * a) we can call [self set_clipboard_manager] here and risk a war.
	 * b) we can print a message and exit.
	 */
	fprintf (stderr, "error: another clipboard manager was started!\n"); 
	exit (EXIT_FAILURE);
    }
}

/* 
 * We greedily acquire the clipboard after it changes, and on startup.
 */
- (void) claim_clipboard
{
    Window owner;
    
    owner = XGetSelectionOwner (x_dpy, atoms->clipboard);
    if (None == owner)
    {
	/*
	 * The owner probably died or we are just starting up pbproxy.
	 * Set pbproxy's _selection_window as the owner, and continue.
	 */
	DB ("No clipboard owner.\n");

	do 
	{
	    XSetSelectionOwner (x_dpy, atoms->clipboard, _selection_window,
				CurrentTime);
	} while (_selection_window != XGetSelectionOwner (x_dpy,
							  atoms->clipboard));

	return;
    }
    
    DB ("requesting targets\n");

    request_atom = atoms->targets;
    XConvertSelection (x_dpy, atoms->clipboard, atoms->targets,
		       atoms->clipboard, _selection_window, CurrentTime);
    /* Now we will get a SelectionNotify event in the future. */
}

/* The preference should be for UTF8_STRING before the XA_STRING*/
static Atom
convert_1 (XSelectionRequestEvent *e, NSString *data, Atom target, Atom prop)
{
    Atom ret = None;

    if (data == nil)
	return ret;

    if (target == atoms->text)
	target = atoms->utf8_string;

    if (target == XA_STRING
	|| target == atoms->cstring
	|| target == atoms->utf8_string)
    {
	const char *bytes;

	if (target == XA_STRING)
	    bytes = [data cStringUsingEncoding:NSISOLatin1StringEncoding];
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
    /*gstaplin: should we [data release]? */
    [data release];

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
    
    if (target == atoms->targets) 
    {
	/* This is where we respond to the TARGETS request. */
	long data[2];

	data[0] = atoms->utf8_string;
	data[1] = XA_STRING;

	/*TODO add handling for when the data can be represented as an image. */

	XChangeProperty (x_dpy, e->requestor, e->property, target,
			 8, PropModeReplace, (unsigned char *) &data,
			 sizeof (data));
	reply.xselection.property = e->property;
    }
    else if (target == atoms->multiple)
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

/* This handles the events resulting from an XConvertSelection request. */
- (void) notify_event:(XSelectionEvent *)e
{
    Atom type;
    struct propdata pdata;
	
    [self release_pending];
    
    DB ("notify_event\n");
    if (None == e->property) {
	DB ("e->property is None.\n");
	/* Nothing is selected. */
	return;
    }

    if (is_incr_type (e)) 
    {
	/*
	 * This is an INCR-style transfer, which means that we 
	 * will get the data after a series of PropertyNotify events.
	 */

	DB ("is INCR\n");

	if (get_property (e->requestor, e->property, &pdata, /*Delete*/ True, &type)) 
	{
	    return;
	}

	free_propdata (&pdata);

	pending.requestor = e->requestor;
	pending.selection = e->selection;
    }
    else
    {
	if (get_property (e->requestor, e->property, &pdata, /*Delete*/ True, &type))
	{
	    return;
	}

	/* We have the complete selection data.*/
	[self handle_selection: e->selection type:type propdata:&pdata];
    }
}

/* This is used for INCR transfers.  See the ICCCM for the details. */
- (void) property_event:(XPropertyEvent *)e
{
    struct propdata pdata;
    Atom type;

    if (None != pending.requestor && PropertyNewValue == e->state) 
    {
	if (get_property (e->window, e->atom, &pdata, /*Delete*/ True, &type))
	{
	    return;
	}

	if (0 == pdata.length) 
	{
	    /* We completed the transfer. */
	    [self handle_selection: pending.selection type: type propdata: &pending.propdata];
	    pending.propdata = null_propdata;
	    pending.requestor = None;
	    pending.selection = None;
	} 
	else 
	{
	    [self append_to_pending: &pdata requestor: e->window];
	}       
    }
}

- (void) handle_targets: (Atom)selection propdata:(struct propdata *)pdata
{
    /* Find a type we can handle and prefer from the list of ATOMs. */
    Atom preferred = find_preferred (pdata);

    if (None == preferred) 
    {
	/* 
	 * This isn't required by the ICCCM, but some apps apparently 
	 * don't respond to TARGETS properly.
	 */
	preferred = XA_STRING;
    }

    DB ("requesting %s\n", XGetAtomName (x_dpy, preferred));
    request_atom = preferred;
    XConvertSelection (x_dpy, selection, preferred, selection,
		       _selection_window, CurrentTime);    
}

/* This handles the image type of selection (typically in CLIPBOARD). */
- (void) handle_image: (struct propdata *)pdata extension:(NSString *)fileext
{
    NSString *pbtype;
    NSArray *pbtypes;
    NSUInteger length;
    NSData *data;

    pbtype = NSCreateFileContentsPboardType (fileext);
    if (nil == pbtype) 
    {
	fprintf (stderr, "unknown extension or unable to create PboardType\n");
	return;
    }
    
    DB ("%s\n", [pbtype cStringUsingEncoding:NSISOLatin1StringEncoding]); 
    
    pbtypes = [NSArray arrayWithObject: pbtype];
    if (nil == pbtypes) 
    {
       DB ("error creating NSArray\n");
       [pbtype release];
       return;
    }
    
    length = pdata->length;
    data = [[NSData alloc] initWithBytes:pdata->data length:length];
    if (nil == data)
    {
	[pbtype release];
	[pbtypes release];
	return;
    }

    [_pasteboard declareTypes:pbtypes owner:self];
    if (YES != [_pasteboard setData:data forType:pbtype])
    {
	DB ("writing pasteboard data failed!\n");
    }

    [pbtype release];
    [pbtypes release];
    [data release];
    
    DB ("handled image\n");
}

/* This handles the UTF8_STRING type of selection. */
- (void) handle_utf8_string: (struct propdata *)pdata
{
    NSString *string = [[NSString alloc] initWithBytes:pdata->data length:pdata->length encoding:NSUTF8StringEncoding];
    if (nil == string)
	return;

    [_pasteboard setString:string forType:NSStringPboardType];
    [string release];
}

/* This handles the XA_STRING type, which should be in Latin-1. */
- (void) handle_string: (struct propdata *)pdata
{
    NSString *string = [[NSString alloc] initWithBytes:pdata->data length:pdata->length encoding:NSISOLatin1StringEncoding];
    if (nil == string)
	return;

    [_pasteboard setString:string forType:NSStringPboardType];
    [string release];
}

/* This is called when the selection is completely retrieved from another client. */
/* Warning: this frees the propdata in most cases. */
- (void) handle_selection:(Atom)selection type:(Atom)type propdata:(struct propdata *)pdata
{
    if (request_atom == atoms->targets && type == atoms->atom)
    {
	[self handle_targets:selection propdata:pdata];
    } 
    else if (type == atoms->image_png)
    {
	[self handle_image:pdata extension:@".png"];
    } 
    else if (type == atoms->image_jpeg)
    {
	[self handle_image:pdata extension:@".jpeg"];
    }
    else if (type == atoms->utf8_string) 
    {
	[self handle_utf8_string:pdata];
    } 
    else if (type == atoms->string)
    {
	[self handle_string:pdata];
    } 
    else
    {
	free_propdata(pdata);
    }
    
    if (selection == atoms->clipboard && pdata->data)
    {
	free_propdata(&request_data.propdata);
	request_data.propdata = *pdata;
	request_data.type = type;
	
	/* We greedily take the CLIPBOARD selection whenever it changes. */
	XSetSelectionOwner (x_dpy, atoms->clipboard, _selection_window,
			    CurrentTime);
    }
    else
    {
	free_propdata(pdata);
    }
}


/* NSPasteboard-required methods */

- (void) pasteboardChangedOwner:(NSPasteboard *)sender
{
    TRACE ();

    DB ("PB changed owner");

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

    /* This is used to get PropertyNotify events when doing INCR transfers. */
    XSelectInput (x_dpy, _selection_window, PropertyChangeMask);

    request_atom = None;

    init_propdata (&pending.propdata);
    pending.requestor = None;
    pending.selection = None;

    init_propdata (&request_data.propdata);
    request_data.type = None;
    
    return self;
}

- (void) dealloc
{
    [_pasteboard releaseGlobally];
    [_pasteboard release];
    _pasteboard = nil;

    [_known_types release];
    _known_types = nil;

    if (None != _selection_window)
    {
	XDestroyWindow (x_dpy, _selection_window);
	_selection_window = None;
    }

    free_propdata (&pending.propdata);
    free_propdata (&request_data.propdata);

    [super dealloc];
}

@end
