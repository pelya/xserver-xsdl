/*
 * Copyright © 2006 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#include "randrstr.h"
#include "propertyst.h"
#include "swaprep.h"

static void
RRDeliverEvent (ScreenPtr pScreen, xEvent *event, CARD32 mask)
{
    
}

void
RRDeleteAllOutputProperties (RROutputPtr output)
{
    PropertyPtr prop, next;
    xRROutputPropertyNotifyEvent    event;

    for (prop = output->properties; prop; prop = next)
    {
	next = prop->next;
	event.type = RREventBase + RRNotify;
	event.subCode = RRNotify_OutputProperty;
	event.output = output->id;
	event.state = PropertyDelete;
	event.atom = prop->propertyName;
	event.timestamp = currentTime.milliseconds;
	RRDeliverEvent (output->pScreen, (xEvent *) &event, RROutputPropertyNotifyMask);
        xfree(prop->data);
        xfree(prop);
    }
}

void
RRDeleteOutputProperty (RROutputPtr output, Atom property)
{
    PropertyPtr	prop, *prev;
    xRROutputPropertyNotifyEvent    event;

    for (prev = &output->properties; (prop = *prev); prev = &(prop->next))
	if (prop->propertyName == property)
	    break;
    if (prop)
    {
	*prev = prop->next;
	event.type = RREventBase + RRNotify;
	event.subCode = RRNotify_OutputProperty;
	event.output = output->id;
	event.state = PropertyDelete;
	event.atom = prop->propertyName;
	event.timestamp = currentTime.milliseconds;
	RRDeliverEvent (output->pScreen, (xEvent *) &event, RROutputPropertyNotifyMask);
        xfree(prop->data);
        xfree(prop);
    }
}

int
RRChangeOutputProperty (RROutputPtr output, Atom property, Atom type,
			int format, int mode, unsigned long len,
			pointer value, Bool sendevent)
{
    PropertyPtr			    prop;
    xRROutputPropertyNotifyEvent    event;
    int				    sizeInBytes;
    int				    totalSize;
    pointer			    data;

    sizeInBytes = format >> 3;
    totalSize = len * sizeInBytes;

    /* first see if property already exists */

    for (prop = output->properties; prop; prop = prop->next)
	if (prop->propertyName == property)
	    break;
    
    if (!prop)   /* just add to list */
    {
        prop = (PropertyPtr)xalloc(sizeof(PropertyRec));
	if (!prop)
	    return(BadAlloc);
        data = (pointer)xalloc(totalSize);
	if (!data && len)
	{
	    xfree(prop);
	    return(BadAlloc);
	}
        prop->propertyName = property;
        prop->type = type;
        prop->format = format;
        prop->data = data;
	if (len)
	    memmove((char *)data, (char *)value, totalSize);
	prop->size = len;
        prop->next = output->properties;
        output->properties = prop;
    }
    else
    {
	/* To append or prepend to a property the request format and type
		must match those of the already defined property.  The
		existing format and type are irrelevant when using the mode
		"PropModeReplace" since they will be written over. */

        if ((format != prop->format) && (mode != PropModeReplace))
	    return(BadMatch);
        if ((prop->type != type) && (mode != PropModeReplace))
            return(BadMatch);
        if (mode == PropModeReplace)
        {
	    if (totalSize != prop->size * (prop->format >> 3))
	    {
	    	data = (pointer)xrealloc(prop->data, totalSize);
	    	if (!data && len)
		    return(BadAlloc);
            	prop->data = data;
	    }
	    if (len)
		memmove((char *)prop->data, (char *)value, totalSize);
	    prop->size = len;
    	    prop->type = type;
	    prop->format = format;
	}
	else if (len == 0)
	{
	    /* do nothing */
	}
        else if (mode == PropModeAppend)
        {
	    data = (pointer)xrealloc(prop->data,
				     sizeInBytes * (len + prop->size));
	    if (!data)
		return(BadAlloc);
            prop->data = data;
	    memmove(&((char *)data)[prop->size * sizeInBytes], 
		    (char *)value,
		  totalSize);
            prop->size += len;
	}
        else if (mode == PropModePrepend)
        {
            data = (pointer)xalloc(sizeInBytes * (len + prop->size));
	    if (!data)
		return(BadAlloc);
	    memmove(&((char *)data)[totalSize], (char *)prop->data, 
		  (int)(prop->size * sizeInBytes));
            memmove((char *)data, (char *)value, totalSize);
	    xfree(prop->data);
            prop->data = data;
            prop->size += len;
	}
    }
    if (sendevent)
    {
	event.type = RREventBase + RRNotify;
	event.subCode = RRNotify_OutputProperty;
	event.output = output->id;
	event.state = PropertyNewValue;
	event.atom = prop->propertyName;
	event.timestamp = currentTime.milliseconds;
	RRDeliverEvent (output->pScreen, (xEvent *) &event, RROutputPropertyNotifyMask);
    }
    return(Success);
}

int
ProcRRListOutputProperties (ClientPtr client)
{
    REQUEST(xRRListOutputPropertiesReq);
    Atom			    *pAtoms = NULL, *temppAtoms;
    xRRListOutputPropertiesReply    rep;
    int				    numProps = 0;
    RROutputPtr			    output;
    PropertyPtr			    prop;
    
    REQUEST_SIZE_MATCH(xRRListOutputPropertiesReq);

    output = LookupOutput (client, stuff->output, SecurityReadAccess);
    
    if (!output)
        return RRErrorBase + BadRROutput;

    for (prop = output->properties; prop; prop = prop->next)
	numProps++;
    if (numProps)
        if(!(pAtoms = (Atom *)ALLOCATE_LOCAL(numProps * sizeof(Atom))))
            return(BadAlloc);

    rep.type = X_Reply;
    rep.nProperties = numProps;
    rep.length = (numProps * sizeof(Atom)) >> 2;
    rep.sequenceNumber = client->sequence;
    temppAtoms = pAtoms;
    for (prop = output->properties; prop; prop = prop->next)
	*temppAtoms++ = prop->propertyName;

    WriteReplyToClient(client, sizeof(xRRListOutputPropertiesReply), &rep);
    if (numProps)
    {
        client->pSwapReplyFunc = (ReplySwapPtr)Swap32Write;
        WriteSwappedDataToClient(client, numProps * sizeof(Atom), pAtoms);
        DEALLOCATE_LOCAL(pAtoms);
    }
    return(client->noClientException);
}

int
ProcRRChangeOutputProperty (ClientPtr client)
{
    REQUEST(xRRChangeOutputPropertyReq);
    RROutputPtr	    output;
    char	    format, mode;
    unsigned long   len;
    int		    sizeInBytes;
    int		    totalSize;
    int		    err;

    REQUEST_AT_LEAST_SIZE(xRRChangeOutputPropertyReq);
    UpdateCurrentTime();
    format = stuff->format;
    mode = stuff->mode;
    if ((mode != PropModeReplace) && (mode != PropModeAppend) &&
	(mode != PropModePrepend))
    {
	client->errorValue = mode;
	return BadValue;
    }
    if ((format != 8) && (format != 16) && (format != 32))
    {
	client->errorValue = format;
        return BadValue;
    }
    len = stuff->nUnits;
    if (len > ((0xffffffff - sizeof(xChangePropertyReq)) >> 2))
	return BadLength;
    sizeInBytes = format>>3;
    totalSize = len * sizeInBytes;
    REQUEST_FIXED_SIZE(xRRChangeOutputPropertyReq, totalSize);

    output = LookupOutput (client, stuff->output, SecurityWriteAccess);
    if (!output)
	return RRErrorBase + BadRROutput;
    
    if (!ValidAtom(stuff->property))
    {
	client->errorValue = stuff->property;
	return(BadAtom);
    }
    if (!ValidAtom(stuff->type))
    {
	client->errorValue = stuff->type;
	return(BadAtom);
    }

    err = RRChangeOutputProperty(output, stuff->property,
				 stuff->type, (int)format,
				 (int)mode, len, (pointer)&stuff[1], TRUE);
    if (err != Success)
	return err;
    else
	return client->noClientException;
}

int
ProcRRDeleteOutputProperty (ClientPtr client)
{
    REQUEST(xRRDeleteOutputPropertyReq);
    RROutputPtr	output;
              
    REQUEST_SIZE_MATCH(xRRDeleteOutputPropertyReq);
    UpdateCurrentTime();
    output = LookupOutput (client, stuff->output, SecurityWriteAccess);
    if (!output)
        return RRErrorBase + BadRROutput;
    
    if (!ValidAtom(stuff->property))
    {
	client->errorValue = stuff->property;
	return (BadAtom);
    }


    RRDeleteOutputProperty(output, stuff->property);
    return client->noClientException;
}

int
ProcRRGetOutputProperty (ClientPtr client)
{
    REQUEST(xRRGetOutputPropertyReq);
    PropertyPtr			prop, *prev;
    unsigned long		n, len, ind;
    RROutputPtr			output;
    xRRGetOutputPropertyReply	reply;

    REQUEST_SIZE_MATCH(xRRGetOutputPropertyReq);
    if (stuff->delete)
	UpdateCurrentTime();
    output = LookupOutput (client, stuff->output, 
			   stuff->delete ? SecurityWriteAccess :
			   SecurityReadAccess);
    if (!output)
	return RRErrorBase + BadRROutput;

    if (!ValidAtom(stuff->property))
    {
	client->errorValue = stuff->property;
	return(BadAtom);
    }
    if ((stuff->delete != xTrue) && (stuff->delete != xFalse))
    {
	client->errorValue = stuff->delete;
	return(BadValue);
    }
    if ((stuff->type != AnyPropertyType) && !ValidAtom(stuff->type))
    {
	client->errorValue = stuff->type;
	return(BadAtom);
    }

    for (prev = &output->properties; (prop = *prev); prev = &prop->next)
	if (prop->propertyName == stuff->property) 
	    break;

    reply.type = X_Reply;
    reply.sequenceNumber = client->sequence;
    if (!prop) 
    {
	reply.nItems = 0;
	reply.length = 0;
	reply.bytesAfter = 0;
	reply.propertyType = None;
	reply.format = 0;
	WriteReplyToClient(client, sizeof(xRRGetOutputPropertyReply), &reply);
	return(client->noClientException);
    }

    /* If the request type and actual type don't match. Return the
    property information, but not the data. */

    if (((stuff->type != prop->type) &&
	 (stuff->type != AnyPropertyType))
       )
    {
	reply.bytesAfter = prop->size;
	reply.format = prop->format;
	reply.length = 0;
	reply.nItems = 0;
	reply.propertyType = prop->type;
	WriteReplyToClient(client, sizeof(xRRGetOutputPropertyReply), &reply);
	return(client->noClientException);
    }

/*
 *  Return type, format, value to client
 */
    n = (prop->format/8) * prop->size; /* size (bytes) of prop */
    ind = stuff->longOffset << 2;        

   /* If longOffset is invalid such that it causes "len" to
	    be negative, it's a value error. */

    if (n < ind)
    {
	client->errorValue = stuff->longOffset;
	return BadValue;
    }

    len = min(n - ind, 4 * stuff->longLength);

    reply.bytesAfter = n - (ind + len);
    reply.format = prop->format;
    reply.length = (len + 3) >> 2;
    reply.nItems = len / (prop->format / 8 );
    reply.propertyType = prop->type;

    if (stuff->delete && (reply.bytesAfter == 0))
    {
	xRROutputPropertyNotifyEvent    event;

	event.type = RREventBase + RRNotify;
	event.subCode = RRNotify_OutputProperty;
	event.output = output->id;
	event.state = PropertyDelete;
	event.atom = prop->propertyName;
	event.timestamp = currentTime.milliseconds;
	RRDeliverEvent (output->pScreen, (xEvent *) &event, RROutputPropertyNotifyMask);
    }

    WriteReplyToClient(client, sizeof(xGenericReply), &reply);
    if (len)
    {
	switch (reply.format) {
	case 32: client->pSwapReplyFunc = (ReplySwapPtr)CopySwap32Write; break;
	case 16: client->pSwapReplyFunc = (ReplySwapPtr)CopySwap16Write; break;
	default: client->pSwapReplyFunc = (ReplySwapPtr)WriteToClient; break;
	}
	WriteSwappedDataToClient(client, len,
				 (char *)prop->data + ind);
    }

    if (stuff->delete && (reply.bytesAfter == 0))
    { /* delete the Property */
	*prev = prop->next;
	xfree(prop->data);
	xfree(prop);
    }
    return(client->noClientException);
}

