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
#include <X11/Xutil.h>
#import <AppKit/NSBitmapImageRep.h>


/*
 * The basic design of the pbproxy code is as follows.
 *
 * When a client selects text, say from an xterm - we only copy it when the
 * X11 Edit->Copy menu item is pressed or the shortcut activated.  In this
 * case we take the PRIMARY selection, and set it as the NSPasteboard data.
 *
 * When an X11 client copies something to the CLIPBOARD, pbproxy greedily grabs
 * the data, sets it as the NSPasteboard data, and finally sets itself as 
 * owner of the CLIPBOARD.
 * 
 * When an X11 window is activated we check to see if the NSPasteboard has
 * changed.  If the NSPasteboard has changed, then we set pbproxy as owner
 * of the PRIMARY and CLIPBOARD and respond to requests for text and images.
 *
 */

/*
 * TODO:
 * 1. finish handling these pbproxy control knobs.
 * 2. handle  MULTIPLE - I need to study the ICCCM further.
 * 3. Handle PICT images properly.
 */

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

    TRACE ();
    
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

/* Implementation methods */

/* This finds the preferred type from a TARGETS list.*/
- (Atom) find_preferred:(struct propdata *)pdata
{
    Atom a = None;
    size_t i;
    Bool png = False, utf8 = False, string = False;

    TRACE ();

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

/* Return True if this is an INCR-style transfer. */
- (Bool) is_incr_type:(XSelectionEvent *)e
{
    Atom seltype;
    int format;
    unsigned long numitems = 0UL, bytesleft = 0UL;
    unsigned char *chunk;
       
    TRACE ();

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

/* 
 * This should be called after a selection has been copied, 
 * or when the selection is unfinished before a transfer completes. 
 */
- (void) release_pending
{
    TRACE ();

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
    
    TRACE ();
    
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
    static NSInteger changeCount;
    NSInteger countNow;

    TRACE ();

    countNow = [_pasteboard changeCount];

    if (countNow != changeCount)
    {
	DB ("changed pasteboard!\n");
	changeCount = countNow;

	XSetSelectionOwner (x_dpy, atoms->primary, _selection_window, CurrentTime);
	[self own_clipboard];
    }

#if 0
    if ([_pasteboard changeCount] != _my_last_change)
    {
	/*gstaplin: we should perhaps investigate something like this branch above...*/
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
    TRACE ();
}

/* This requests the TARGETS list from the PRIMARY selection owner. */
- (void) x_copy_request_targets
{
    TRACE ();

    request_atom = atoms->targets;
    XConvertSelection (x_dpy, atoms->primary, atoms->targets,
		       atoms->primary, _selection_window, CurrentTime);
}

/* Called when the Edit/Copy item on the main X11 menubar is selected
   and no appkit window claims it. */
- (void) x_copy:(Time)timestamp
{
    Window w;

    TRACE ();

    w = XGetSelectionOwner (x_dpy, atoms->primary);

    if (None != w)
    {
	++pending_copy;
	
	if (1 == pending_copy) {
	    /*
	     * There are no other copy operations in progress, so we
	     * can proceed safely.
	     */	    
	    [self x_copy_request_targets];
	}
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
    TRACE ();

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
    
    DB ("e->selection %s\n", XGetAtomName (x_dpy, e->selection));
 
    if (atoms->clipboard == e->selection)
    {
	/* 
	 * We lost ownership of the CLIPBOARD.
	 */
	++pending_clipboard;

	if (1 == pending_clipboard) 
	{
	    /* Claim the clipboard contents from the new owner. */
	    [self claim_clipboard];
	}
    } 
    else if (atoms->clipboard_manager == e->selection)
    {
	/* Another CLIPBOARD_MANAGER has set itself as owner.
         * a) we can call [self set_clipboard_manager] here and risk a war.
	 * b) we can print a message and exit.  Ideally we would popup a message box.
	 */
	fprintf (stderr, "error: another clipboard manager was started!\n"); 
	//exit (EXIT_FAILURE);
    }
}

/* 
 * We greedily acquire the clipboard after it changes, and on startup.
 */
- (void) claim_clipboard
{
    Window owner;
    
    TRACE ();

    if (NO == pbproxy_clipboard_to_pasteboard)
	return;
    
    owner = XGetSelectionOwner (x_dpy, atoms->clipboard);
    if (None == owner)
    {
	/*
	 * The owner probably died or we are just starting up pbproxy.
	 * Set pbproxy's _selection_window as the owner, and continue.
	 */
	DB ("No clipboard owner.\n");
	[self copy_completed:atoms->clipboard];
	return;
    }
    
    DB ("requesting targets\n");

    request_atom = atoms->targets;
    XConvertSelection (x_dpy, atoms->clipboard, atoms->targets,
		       atoms->clipboard, _selection_window, CurrentTime);
    XFlush (x_dpy);
    /* Now we will get a SelectionNotify event in the future. */
}

/* Greedily acquire the clipboard. */
- (void) own_clipboard
{

    TRACE ();

    /* We should perhaps have a boundary limit on the number of iterations... */
    do 
    {
	XSetSelectionOwner (x_dpy, atoms->clipboard, _selection_window,
			    CurrentTime);
    } while (_selection_window != XGetSelectionOwner (x_dpy,
						      atoms->clipboard));
}

- (void) init_reply:(XEvent *)reply request:(XSelectionRequestEvent *)e
{
    reply->xselection.type = SelectionNotify;
    reply->xselection.selection = e->selection;
    reply->xselection.target = e->target;
    reply->xselection.requestor = e->requestor;
    reply->xselection.time = e->time;
    reply->xselection.property = None; 
}

- (void) send_reply:(XEvent *)reply
{
    /*
     * We are supposed to use an empty event mask, and not propagate
     * the event, according to the ICCCM.
     */
    DB ("reply->xselection.requestor 0x%lx\n", reply->xselection.requestor);
  
    XSendEvent (x_dpy, reply->xselection.requestor, False, 0, reply);
    XFlush (x_dpy);
}

/* 
 * This responds to a TARGETS request.
 * The result is a list of a ATOMs that correspond to the types available
 * for a selection.  
 * For instance an application might provide a UTF8_STRING and a STRING
 * (in Latin-1 encoding).  The requestor can then make the choice based on
 * the list.
 */
- (void) send_targets:(XSelectionRequestEvent *)e
{
    XEvent reply;
    NSArray *pbtypes;

    [self init_reply:&reply request:e];

    pbtypes = [_pasteboard types];
    if (pbtypes)
    {
	long list[6];
        long count = 0;
	
	if ([pbtypes containsObject:NSStringPboardType])
	{
	    /* We have a string type that we can convert to UTF8, or Latin-1... */
	    DB ("NSStringPboardType\n");
	    list[count] = atoms->utf8_string;
	    ++count;
	    list[count] = atoms->string;
	    ++count;
	    list[count] = atoms->compound_text;
	    ++count;
	}

	/* TODO add the NSPICTPboardType back again, once we have conversion
	 * functionality in send_image.
	 */

	if ([pbtypes containsObject:NSTIFFPboardType]) 
	{
	    /* We can convert a TIFF to a PNG or JPEG. */
	    DB ("NSTIFFPboardType\n");
	    list[count] = atoms->image_png;
	    ++count;
	    list[count] = atoms->image_jpeg;
	    ++count;
	} 

	if (count)
	{
	    /* We have a list of ATOMs to send. */
	    XChangeProperty (x_dpy, e->requestor, e->property, atoms->atom, 32,
			 PropModeReplace, (unsigned char *) list, count);
	    
	    reply.xselection.property = e->property;
	}
    }

    [self send_reply:&reply];
}


- (void) send_string:(XSelectionRequestEvent *)e utf8:(BOOL)utf8
{
    XEvent reply;
    NSArray *pbtypes;
    
    TRACE ();

    [self init_reply:&reply request:e];

    pbtypes = [_pasteboard types];

    if ([pbtypes containsObject: NSStringPboardType])
    {
	NSString *data = [_pasteboard stringForType:NSStringPboardType];
	if (nil != data)
	{
	    const char *bytes;
	    NSUInteger length;

	    if (utf8) 
	    {
		bytes = [data UTF8String];
		/*
		 * We don't want the UTF-8 string length here.  
		 * We want the length in bytes.
		 */
		length = strlen (bytes);

		if (length < 50) {
		    DB ("UTF-8: %s\n", bytes);
		    DB ("UTF-8 length: %u\n", length); 
		}
	    } 
	    else 
	    {
		DB ("Latin-1\n");
		bytes = [data cStringUsingEncoding:NSISOLatin1StringEncoding];
		length = strlen (bytes);
	    }

	    DB ("e->target %s\n", XGetAtomName (x_dpy, e->target));

	    XChangeProperty (x_dpy, e->requestor, e->property, e->target,
			     8, PropModeReplace, (unsigned char *) bytes, length);
	    
	    reply.xselection.property = e->property;
 	}
    }

    [self send_reply:&reply];
}

- (void) send_compound_text:(XSelectionRequestEvent *)e
{
    XEvent reply;
    NSArray *pbtypes;
    
    TRACE ();
    
    [self init_reply:&reply request:e];
     
    pbtypes = [_pasteboard types];

    if ([pbtypes containsObject: NSStringPboardType])
    {
	NSString *data = [_pasteboard stringForType:NSStringPboardType];
	if (nil != data)
	{
	    /*
	     * Cast to (void *) to avoid a const warning. 
	     * AFAIK Xutf8TextListToTextProperty does not modify the input memory.
	     */
	    void *utf8 = (void *)[data UTF8String];
	    char *list[] = { utf8, NULL };
	    XTextProperty textprop;
	    
	    textprop.value = NULL;

	    if (Success == Xutf8TextListToTextProperty (x_dpy, list, 1,
							XCompoundTextStyle,
							&textprop))
	    {
		
		if (8 != textprop.format)
		    DB ("textprop.format is unexpectedly not 8 - it's %d instead\n",
			textprop.format);

		XChangeProperty (x_dpy, e->requestor, e->property, 
				 atoms->compound_text, textprop.format, 
				 PropModeReplace, textprop.value,
				 textprop.nitems);
		
		reply.xselection.property = e->property;
	    }

	    if (textprop.value)
 		XFree (textprop.value);
	}
    }
    
    [self send_reply:&reply];
}

/* Finding a test application that uses MULTIPLE has proven to be difficult. */
- (void) send_multiple:(XSelectionRequestEvent *)e
{
    XEvent reply;

    TRACE ();

    [self init_reply:&reply request:e];

    if (None != e->property) 
    {
	
    }
    
    [self send_reply:&reply];
}


- (void) send_image:(XSelectionRequestEvent *)e
{
    XEvent reply;
    NSArray *pbtypes;
    NSString *type = nil;
    NSBitmapImageFileType imagetype = /*quiet warning*/ NSPNGFileType; 
    NSData *data;

    TRACE ();

    [self init_reply:&reply request:e];

    pbtypes = [_pasteboard types];

    if (pbtypes) 
    {
	if ([pbtypes containsObject:NSTIFFPboardType])
	    type = NSTIFFPboardType;

	/* PICT is not yet supported by pbproxy. 
	 * The NSBitmapImageRep doesn't support it. 
	else if ([pbtypes containsObject:NSPICTPboardType])
	    type  = NSPICTPboardType;
	*/
    }

    if (e->target == atoms->image_png)
	imagetype = NSPNGFileType;
    else if (e->target == atoms->image_jpeg)
	imagetype = NSJPEGFileType;
        

    if (nil == type) 
    {
	[self send_reply:&reply];
	return;
    }

    data = [_pasteboard dataForType:type];

    if (nil == data)
    {
	[self send_reply:&reply];
	return;
    }
	 
    if (NSTIFFPboardType == type)
    {
	NSBitmapImageRep *bmimage = [[NSBitmapImageRep alloc] initWithData:data];
	NSDictionary *dict;
	NSData *encdata;

	if (nil == bmimage) 
	{
	    [self send_reply:&reply];
	    return;
	}

	DB ("have valid bmimage\n");
	
	dict = [[NSDictionary alloc] init];
	encdata = [bmimage representationUsingType:imagetype properties:dict];
	if (encdata)
	{
	    NSUInteger length;
	    const void *bytes;
	    
	    length = [encdata length];
	    bytes = [encdata bytes];
		
	    XChangeProperty (x_dpy, e->requestor, e->property, e->target,
				 8, PropModeReplace, bytes, length);
	    
	    reply.xselection.property = e->property;
	    
	    DB ("changed property for %s\n", XGetAtomName (x_dpy, e->target));
	}
	[dict release];
	[bmimage release];
    } 

    [self send_reply:&reply];
}

- (void)send_none:(XSelectionRequestEvent *)e
{
    XEvent reply;

    TRACE ();

    [self init_reply:&reply request:e];
    [self send_reply:&reply];
}


/* Another client requested the data or targets of data available from the clipboard. */
- (void)request_event:(XSelectionRequestEvent *)e
{
    TRACE ();

    /* TODO We should also keep track of the time of the selection, and 
     * according to the ICCCM "refuse the request" if the event timestamp
     * is before we owned it.
     * What should we base the time on?  How can we get the current time just
     * before an XSetSelectionOwner?  Is it the server's time, or the clients?
     * According to the XSelectionRequestEvent manual page, the Time value
     * may be set to CurrentTime or a time, so that makes it a bit different.
     * Perhaps we should just punt and ignore races.
     */

    /*TODO we need a COMPOUND_TEXT test app*/
    /*TODO we need a MULTIPLE test app*/

    DB ("e->target %s\n", XGetAtomName (x_dpy, e->target));

    if (e->target == atoms->targets) 
    {
	/* The paste requestor wants to know what TARGETS we support. */
	[self send_targets:e];
    }
    else if (e->target == atoms->multiple)
    {
	[self send_multiple:e];
    } 
    else if (e->target == atoms->utf8_string)
    {
	[self send_string:e utf8:YES];
    } 
    else if (e->target == atoms->string)
    {
	[self send_string:e utf8:NO];
    }
    else if (e->target == atoms->compound_text)
    {
	[self send_compound_text:e];
    }
    else if (e->target == atoms->multiple)
    {
	[self send_multiple:e];
    }
    else if (e->target == atoms->image_png || e->target == atoms->image_jpeg)
    {
	[self send_image:e];
    }
    else 
    {
	[self send_none:e];
    }
}

/* This handles the events resulting from an XConvertSelection request. */
- (void) notify_event:(XSelectionEvent *)e
{
    Atom type;
    struct propdata pdata;
	
    TRACE ();

    [self release_pending];
 
    if (None == e->property) {
	DB ("e->property is None.\n");
	[self copy_completed:e->selection];
	/* Nothing is selected. */
	return;
    }

    DB ("e->selection %s\n", XGetAtomName (x_dpy, e->selection));
    DB ("e->property %s\n", XGetAtomName (x_dpy, e->property));

    if ([self is_incr_type:e]) 
    {
	/*
	 * This is an INCR-style transfer, which means that we 
	 * will get the data after a series of PropertyNotify events.
	 */
	DB ("is INCR\n");

	if (get_property (e->requestor, e->property, &pdata, /*Delete*/ True, &type)) 
	{
	    /* 
	     * An error occured, so we should invoke the copy_completed:, but
	     * not handle_selection:type:propdata:
	     */
	    [self copy_completed:e->selection];
	    return;
	}

	free_propdata (&pdata);

	pending.requestor = e->requestor;
	pending.selection = e->selection;

	DB ("set pending.requestor to 0x%lx\n", pending.requestor);
    }
    else
    {
	if (get_property (e->requestor, e->property, &pdata, /*Delete*/ True, &type))
	{
	    [self copy_completed:e->selection];
	    return;
	}

	/* We have the complete selection data.*/
	[self handle_selection:e->selection type:type propdata:&pdata];
	
	DB ("handled selection with the first notify_event\n");
    }
}

/* This is used for INCR transfers.  See the ICCCM for the details. */
/* This is used to retrieve PRIMARY and CLIPBOARD selections. */
- (void) property_event:(XPropertyEvent *)e
{
    struct propdata pdata;
    Atom type;

    TRACE ();
    
    if (None != e->atom)
	DB ("e->atom %s\n", XGetAtomName (x_dpy, e->atom));


    if (None != pending.requestor && PropertyNewValue == e->state) 
    {
	DB ("pending.requestor 0x%lx\n", pending.requestor);

	if (get_property (e->window, e->atom, &pdata, /*Delete*/ True, &type))
        {
	    [self copy_completed:pending.selection];
	    [self release_pending];
	    return;
	}

	if (0 == pdata.length) 
	{
	    /* We completed the transfer. */
	    [self handle_selection:pending.selection type:type propdata:&pending.propdata];
	    pending.propdata = null_propdata;
	    pending.requestor = None;
	    pending.selection = None;
	} 
	else 
	{
	    [self append_to_pending:&pdata requestor:e->window];
	}       
    }
}

- (void) handle_targets: (Atom)selection propdata:(struct propdata *)pdata
{
    /* Find a type we can handle and prefer from the list of ATOMs. */
    Atom preferred;

    TRACE ();

    preferred = [self find_preferred:pdata];
    
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
/* We convert to a TIFF, so that other applications can paste more easily. */
- (void) handle_image: (struct propdata *)pdata
{
    NSArray *pbtypes;
    NSUInteger length;
    NSData *data, *tiff;
    NSBitmapImageRep *bmimage;

    TRACE ();

    length = pdata->length;
    data = [[NSData alloc] initWithBytes:pdata->data length:length];

    if (nil == data)
    {
	DB ("unable to create NSData object!\n");
	return;
    }

    bmimage = [[NSBitmapImageRep alloc] initWithData:data];

    if (nil == bmimage)
    {
	DB ("unable to create NSBitmapImageRep!\n");
	return;
    }

    @try 
    {
	tiff = [bmimage TIFFRepresentation];
    }

    @catch (NSException *e) 
    {
	DB ("NSTIFFException!\n");
	[data release];
	[bmimage release];
	return;
    }
    
    pbtypes = [NSArray arrayWithObjects:NSTIFFPboardType, nil];

    if (nil == pbtypes)
    {
	[tiff release];
	[data release];
	[bmimage release];
    }

    [_pasteboard declareTypes:pbtypes owner:self];
    if (YES != [_pasteboard setData:data forType:NSTIFFPboardType])
    {
	DB ("writing pasteboard data failed!\n");
    }

    [pbtypes release];
    [data release];
    [tiff release];
    [bmimage release];
}

/* This handles the UTF8_STRING type of selection. */
- (void) handle_utf8_string: (struct propdata *)pdata
{
    NSString *string;
    NSArray *pbtypes;

    TRACE ();

    string = [[NSString alloc] initWithBytes:pdata->data length:pdata->length encoding:NSUTF8StringEncoding];
 
    if (nil == string)
	return;

    pbtypes = [NSArray arrayWithObjects:NSStringPboardType, nil];

    if (nil != pbtypes)
    {
	[_pasteboard declareTypes:pbtypes owner:self];

	if (YES != [_pasteboard setString:string forType:NSStringPboardType]) {
	    DB ("_pasteboard setString:forType: failed!\n");
	}
	[pbtypes release];
    }
    [string release];

    DB ("done handling utf8 string\n");
}

/* This handles the STRING type, which should be in Latin-1. */
- (void) handle_string: (struct propdata *)pdata
{
    NSString *string; 
    NSArray *pbtypes;

    TRACE ();

    string = [[NSString alloc] initWithBytes:pdata->data length:pdata->length encoding:NSISOLatin1StringEncoding];
    
    if (nil == string)
	return;

    pbtypes = [NSArray arrayWithObjects:NSStringPboardType, nil];

    if (nil != pbtypes)
    {
	[_pasteboard declareTypes:pbtypes owner:self];
	[_pasteboard setString:string forType:NSStringPboardType];
	[pbtypes release];
    }

    [string release];
}

/* This is called when the selection is completely retrieved from another client. */
/* Warning: this frees the propdata. */
- (void) handle_selection:(Atom)selection type:(Atom)type propdata:(struct propdata *)pdata
{
    TRACE ();

    if (request_atom == atoms->targets && type == atoms->atom)
    {
	[self handle_targets:selection propdata:pdata];
    } 
    else if (type == atoms->image_png)
    {
	[self handle_image:pdata];
    } 
    else if (type == atoms->image_jpeg)
    {
	[self handle_image:pdata];
    }
    else if (type == atoms->utf8_string) 
    {
	[self handle_utf8_string:pdata];
    } 
    else if (type == atoms->string)
    {
	[self handle_string:pdata];
    } 
 
    free_propdata(pdata);
    
    [self copy_completed:selection];
}

- (void) copy_completed:(Atom)selection
{
    TRACE ();
    
    DB ("copy_completed: %s\n", XGetAtomName (x_dpy, selection));

    if (selection == atoms->primary && pending_copy > 0)
    {
	--pending_copy;
	if (pending_copy > 0)
	{
	    /* Copy PRIMARY again. */
	    [self x_copy_request_targets];
	    return;
	}
    }
    else if (selection == atoms->clipboard && pending_clipboard > 0) 
    {
	--pending_clipboard;
	if (pending_clipboard > 0) 
	{
	    /* Copy CLIPBOARD. */
	    [self claim_clipboard];
	    return;
	} 
	else 
	{
	    /* We got the final data.  Now set pbproxy as the owner. */
	    [self own_clipboard];
	    return;
	}
    }
    
    /* 
     * We had 1 or more primary in progress, and the clipboard arrived
     * while we were busy. 
     */
    if (pending_clipboard > 0)
    {
	[self claim_clipboard];
    }
}


/* NSPasteboard-required methods */

- (void) paste:(id)sender
{
    TRACE ();
}

- (void) pasteboard:(NSPasteboard *)pb provideDataForType:(NSString *)type
{
    TRACE ();
}

- (void) pasteboardChangedOwner:(NSPasteboard *)pb
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

    atoms->primary = XInternAtom (x_dpy, "PRIMARY", False);
    atoms->clipboard = XInternAtom (x_dpy, "CLIPBOARD", False);
    atoms->text = XInternAtom (x_dpy, "TEXT", False);
    atoms->utf8_string = XInternAtom (x_dpy, "UTF8_STRING", False);
    atoms->targets = XInternAtom (x_dpy, "TARGETS", False);
    atoms->multiple = XInternAtom (x_dpy, "MULTIPLE", False);
    atoms->cstring = XInternAtom (x_dpy, "CSTRING", False);
    atoms->image_png = XInternAtom (x_dpy, "image/png", False);
    atoms->image_jpeg = XInternAtom (x_dpy, "image/jpeg", False);
    atoms->incr = XInternAtom (x_dpy, "INCR", False);
    atoms->atom = XInternAtom (x_dpy, "ATOM", False);
    atoms->clipboard_manager = XInternAtom (x_dpy, "CLIPBOARD_MANAGER", False);
    atoms->compound_text = XInternAtom (x_dpy, "COMPOUND_TEXT", False);
    atoms->atom_pair = XInternAtom (x_dpy, "ATOM_PAIR", False);

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

    pending_copy = 0;
    pending_clipboard = 0;

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

    [super dealloc];
}

@end
