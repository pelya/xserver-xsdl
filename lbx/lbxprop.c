/* $Xorg: lbxprop.c,v 1.4 2001/02/09 02:05:17 xorgcvs Exp $ */
/*

Copyright 1986, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

*/
/*
 * Copyright 1993 Network Computing Devices, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name Network Computing Devices, Inc. not be
 * used in advertising or publicity pertaining to distribution of this
 * software without specific, written prior permission.
 *
 * THIS SOFTWARE IS PROVIDED `AS-IS'.  NETWORK COMPUTING DEVICES, INC.,
 * DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING WITHOUT
 * LIMITATION ALL IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NONINFRINGEMENT.  IN NO EVENT SHALL NETWORK
 * COMPUTING DEVICES, INC., BE LIABLE FOR ANY DAMAGES WHATSOEVER, INCLUDING
 * SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES, INCLUDING LOSS OF USE, DATA,
 * OR PROFITS, EVEN IF ADVISED OF THE POSSIBILITY THEREOF, AND REGARDLESS OF
 * WHETHER IN AN ACTION IN CONTRACT, TORT OR NEGLIGENCE, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $XFree86: xc/programs/Xserver/lbx/lbxprop.c,v 1.5 2001/12/14 20:00:00 dawes Exp $ */

/* various bits of DIX-level mangling */

#include <sys/types.h>
#include <stdio.h>
#define NEED_REPLIES
#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "os.h"
#include "dixstruct.h"
#include "resource.h"
#include "servermd.h"
#include "propertyst.h"
#include "colormapst.h"
#include "windowstr.h"
#define _XLBX_SERVER_
#include "lbxstr.h"
#include "lbxserve.h"
#include "lbxtags.h"
#include "Xfuncproto.h"
#ifdef XCSECURITY
#define _SECURITY_SERVER
#include "extensions/security.h"
#endif
#include "swaprep.h"

void
LbxStallPropRequest(ClientPtr   client,
		    PropertyPtr pProp)
{
    xReq *req = (xReq *) client->requestBuffer;
    register char n;

    LbxQueryTagData(client, pProp->owner_pid,
		    pProp->tag_id, LbxTagTypeProperty);

    /*
     * Before we reset the request, we must make sure
     * it is in the client's byte order.
     */

    if (client->swapped) {
	if (req->reqType == X_ChangeProperty) {
	    xChangePropertyReq *stuff = (xChangePropertyReq *) req;
	    swaps(&stuff->length, n);
	    swapl(&stuff->window, n);
	    swapl(&stuff->property, n);
	    swapl(&stuff->type, n);
	    swapl(&stuff->nUnits, n);
	    switch ( stuff->format ) {
	    case 16:
		SwapRestS(stuff);
		break;
	    case 32:
		SwapRestL(stuff);
		break;
	    }
	} else if (req->reqType == X_GetProperty) {
	    xGetPropertyReq *stuff = (xGetPropertyReq *) req;
	    swaps(&stuff->length, n);
	    swapl(&stuff->window, n);
	    swapl(&stuff->property, n);
	    swapl(&stuff->type, n);
	    swapl(&stuff->longOffset, n);
	    swapl(&stuff->longLength, n);
	} else if (req->data == X_LbxChangeProperty) {
	    xLbxChangePropertyReq *stuff = (xLbxChangePropertyReq *) req;
	    swaps(&stuff->length, n);
	    swapl(&stuff->window, n);
	    swapl(&stuff->property, n);
	    swapl(&stuff->type, n);
	    swapl(&stuff->nUnits, n);
	} else if (req->data == X_LbxGetProperty) {
	    xLbxGetPropertyReq *stuff = (xLbxGetPropertyReq *) req;
	    swaps(&stuff->length, n);
	    swapl(&stuff->window, n);
	    swapl(&stuff->property, n);
	    swapl(&stuff->type, n);
	    swapl(&stuff->longOffset, n);
	    swapl(&stuff->longLength, n);
	}
    }
    ResetCurrentRequest(client);
    client->sequence--;
    IgnoreClient(client);
}

int
LbxChangeWindowProperty(ClientPtr   	client,
			WindowPtr   	pWin,
			Atom        	property,
			Atom	    	type,
			int         	format,
			int	    	mode,
			unsigned long	len,
			Bool        	have_data,
			pointer     	value,
			Bool        	sendevent,
			XID            *tag)
{
    PropertyPtr pProp;
    xEvent      event;
    int         sizeInBytes;
    int         totalSize;
    pointer     data;

    sizeInBytes = format >> 3;
    totalSize = len * sizeInBytes;

    /* first see if property already exists */

    pProp = wUserProps(pWin);
    while (pProp) {
	if (pProp->propertyName == property)
	    break;
	pProp = pProp->next;
    }
    if (!pProp) {		/* just add to list */
	if (!pWin->optional && !MakeWindowOptional(pWin))
	    return (BadAlloc);
	pProp = (PropertyPtr) xalloc(sizeof(PropertyRec));
	if (!pProp)
	    return (BadAlloc);
	data = (pointer) xalloc(totalSize);
	if (!data && len) {
	    xfree(pProp);
	    return (BadAlloc);
	}
	pProp->propertyName = property;
	pProp->type = type;
	pProp->format = format;
	pProp->data = data;
	if (have_data) {
	    if (len)
		memmove((char *) data, (char *) value, totalSize);
	    pProp->tag_id = 0;
	    pProp->owner_pid = 0;
	} else {
	    if (!TagSaveTag(LbxTagTypeProperty, totalSize,
			    (pointer)pProp, &pProp->tag_id)) {
		xfree(pProp);
		xfree(pProp->data);
		return BadAlloc;
	    }
	    pProp->owner_pid = LbxProxyID(client);
	    TagMarkProxy(pProp->tag_id, pProp->owner_pid);
	}
	pProp->size = len;
	pProp->next = pWin->optional->userProps;
	pWin->optional->userProps = pProp;
    } else {
	/*
	 * To append or prepend to a property the request format and type must
	 * match those of the already defined property.  The existing format
	 * and type are irrelevant when using the mode "PropModeReplace" since
	 * they will be written over.
	 */

	if ((format != pProp->format) && (mode != PropModeReplace))
	    return (BadMatch);
	if ((pProp->type != type) && (mode != PropModeReplace))
	    return (BadMatch);

	/*
	 * if its a modify instead of replace, make sure we have the current
	 * value
	 */
	if ((mode != PropModeReplace) && pProp->tag_id && pProp->owner_pid) {
	    LbxStallPropRequest(client, pProp);
	    return (client->noClientException);
	}
	/* make sure any old tag is flushed first */
	if (pProp->tag_id)
	    TagDeleteTag(pProp->tag_id);
	if (mode == PropModeReplace) {
	    if (totalSize != pProp->size * (pProp->format >> 3)) {
		data = (pointer) xrealloc(pProp->data, totalSize);
		if (!data && len)
		    return (BadAlloc);
		pProp->data = data;
	    }
	    if (have_data) {
		if (len)
		    memmove((char *) pProp->data, (char *) value, totalSize);
	    } else {
		if (!TagSaveTag(LbxTagTypeProperty, totalSize,
				(pointer)pProp, &pProp->tag_id)) {
		    xfree(pProp);
		    xfree(pProp->data);
		    return BadAlloc;
		}
		pProp->owner_pid = LbxProxyID(client);
		TagMarkProxy(pProp->tag_id, pProp->owner_pid);
	    }
	    pProp->size = len;
	    pProp->type = type;
	    pProp->format = format;
	} else if (len == 0) {
	    /* do nothing */
	} else if (mode == PropModeAppend) {
	    data = (pointer) xrealloc(pProp->data,
				      sizeInBytes * (len + pProp->size));
	    if (!data)
		return (BadAlloc);
	    pProp->data = data;
	    memmove(&((char *) data)[pProp->size * sizeInBytes],
		    (char *) value,
		    totalSize);
	    pProp->size += len;
	} else if (mode == PropModePrepend) {
	    data = (pointer) xalloc(sizeInBytes * (len + pProp->size));
	    if (!data)
		return (BadAlloc);
	    memmove(&((char *) data)[totalSize], (char *) pProp->data,
		    (int) (pProp->size * sizeInBytes));
	    memmove((char *) data, (char *) value, totalSize);
	    xfree(pProp->data);
	    pProp->data = data;
	    pProp->size += len;
	}
    }
    if (sendevent) {
	event.u.u.type = PropertyNotify;
	event.u.property.window = pWin->drawable.id;
	event.u.property.state = PropertyNewValue;
	event.u.property.atom = pProp->propertyName;
	event.u.property.time = currentTime.milliseconds;
	DeliverEvents(pWin, &event, 1, (WindowPtr) NULL);
    }
    if (pProp->tag_id)
	*tag = pProp->tag_id;
    return (Success);
}

int
LbxChangeProperty(ClientPtr client)
{
    WindowPtr   pWin;
    char        format,
                mode;
    unsigned long len;
    int         err;
    int         n;
    XID         newtag;
    xLbxChangePropertyReply rep;
    REQUEST(xLbxChangePropertyReq);

    REQUEST_SIZE_MATCH(xLbxChangePropertyReq);
    UpdateCurrentTime();
    format = stuff->format;
    mode = stuff->mode;
    if ((mode != PropModeReplace) && (mode != PropModeAppend) &&
	    (mode != PropModePrepend)) {
	client->errorValue = mode;
	return BadValue;
    }
    if ((format != 8) && (format != 16) && (format != 32)) {
	client->errorValue = format;
	return BadValue;
    }
    len = stuff->nUnits;
    if (len > ((0xffffffff - sizeof(xChangePropertyReq)) >> 2))
	return BadLength;

    pWin = (WindowPtr) SecurityLookupWindow(stuff->window, client,
					    SecurityWriteAccess);
    if (!pWin)
	return (BadWindow);
    if (!ValidAtom(stuff->property)) {
	client->errorValue = stuff->property;
	return (BadAtom);
    }
    if (!ValidAtom(stuff->type)) {
	client->errorValue = stuff->type;
	return (BadAtom);
    }

    rep.type = X_Reply;
    rep.sequenceNumber = client->sequence;
    rep.length = 0;
    rep.pad = rep.pad0 = rep.pad1 = rep.pad2 = rep.pad3 = rep.pad4 = 0;
    if (client->swapped) {
	swaps(&rep.sequenceNumber, n);
    }

#ifdef XCSECURITY
    switch (SecurityCheckPropertyAccess(client, pWin, stuff->property,
					SecurityWriteAccess))
    {
	case SecurityErrorOperation:
	    client->errorValue = stuff->property;
	    return BadAtom;
	case SecurityIgnoreOperation:
	    rep.tag = 0;
	    WriteToClient(client, sizeof(xLbxChangePropertyReply), (char *)&rep);
	    return client->noClientException;
    }
#endif

    err = LbxChangeWindowProperty(client, pWin, stuff->property, stuff->type,
		   (int) format, (int) mode, len, FALSE, (pointer) &stuff[1],
				  TRUE, &newtag);
    if (err)
	return err;

    rep.tag = newtag;

    if (client->swapped) {
	swapl(&rep.tag, n);
    }
    WriteToClient(client, sizeof(xLbxChangePropertyReply), (char *)&rep);

    return client->noClientException;
}

static void
LbxWriteGetpropReply(ClientPtr   	   client,
		     xLbxGetPropertyReply *rep)
{
    int         n;

    if (client->swapped) {
	swaps(&rep->sequenceNumber, n);
	swapl(&rep->length, n);
	swapl(&rep->propertyType, n);
	swapl(&rep->bytesAfter, n);
	swapl(&rep->nItems, n);
	swapl(&rep->tag, n);
    }
    WriteToClient(client, sizeof(xLbxGetPropertyReply), (char *)rep);
}

int
LbxGetProperty(ClientPtr client)
{
    PropertyPtr pProp,
                prevProp;
    unsigned long n,
                len,
                ind;
    WindowPtr   pWin;
    xLbxGetPropertyReply reply;
    Bool        send_data = FALSE;

    REQUEST(xLbxGetPropertyReq);

    REQUEST_SIZE_MATCH(xLbxGetPropertyReq);

    reply.pad1 = 0;
    reply.pad2 = 0;

    if (stuff->delete)
	UpdateCurrentTime();
    pWin = (WindowPtr) SecurityLookupWindow(stuff->window, client,
					    SecurityReadAccess);
    if (!pWin)
	return (BadWindow);

    if (!ValidAtom(stuff->property)) {
	client->errorValue = stuff->property;
	return (BadAtom);
    }
    if ((stuff->delete != xTrue) && (stuff->delete != xFalse)) {
	client->errorValue = stuff->delete;
	return (BadValue);
    }
    if ((stuff->type != AnyPropertyType) && !ValidAtom(stuff->type))
    {
	client->errorValue = stuff->type;
	return(BadAtom);
    }
    pProp = wUserProps(pWin);
    prevProp = (PropertyPtr) NULL;
    while (pProp) {
	if (pProp->propertyName == stuff->property)
	    break;
	prevProp = pProp;
	pProp = pProp->next;
    }
    reply.type = X_Reply;
    reply.sequenceNumber = client->sequence;
    if (!pProp) {
	reply.nItems = 0;
	reply.length = 0;
	reply.bytesAfter = 0;
	reply.propertyType = None;
	reply.format = 0;
	reply.tag = 0;
	LbxWriteGetpropReply(client, &reply);
	return client->noClientException;
    }
    /*
     * If the request type and actual type don't match. Return the
     * property information, but not the data.
     */
    if ((stuff->type != pProp->type) &&
	(stuff->type != AnyPropertyType)) {
	reply.bytesAfter = pProp->size;
	reply.format = pProp->format;
	reply.length = 0;
	reply.nItems = 0;
	reply.propertyType = pProp->type;
	reply.tag = 0;
	LbxWriteGetpropReply(client, &reply);
	return client->noClientException;
    }
    /*
     * Return type, format, value to client
     */
    n = (pProp->format >> 3) * pProp->size;	/* size (bytes) of prop */
    ind = stuff->longOffset << 2;

    /*
     * If longOffset is invalid such that it causes "len" to be
     * negative, it's a value error.
     */

    if (n < ind) {
	client->errorValue = stuff->longOffset;
	return BadValue;
    }

    /* make sure we have the current value */
    if (pProp->tag_id && pProp->owner_pid) {
	LbxStallPropRequest(client, pProp);
	return client->noClientException;
    }

    len = min(n - ind, stuff->longLength << 2);

    reply.bytesAfter = n - (ind + len);
    reply.format = pProp->format;
    reply.propertyType = pProp->type;

    if (!pProp->tag_id) {
	if (n && (!stuff->delete || reply.bytesAfter)) {
	    TagSaveTag(LbxTagTypeProperty, n, (pointer)pProp, &pProp->tag_id);
	    pProp->owner_pid = 0;
	}
	send_data = TRUE;
    } else
	send_data = !TagProxyMarked(pProp->tag_id, LbxProxyID(client));
    if (pProp->tag_id && send_data)
	TagMarkProxy(pProp->tag_id, LbxProxyID(client));
    reply.tag = pProp->tag_id;

    if (!send_data)
	len = 0;
    else if (reply.tag) {
	len = n;
	ind = 0;
    }
    reply.nItems = len / (pProp->format >> 3);
    reply.length = (len + 3) >> 2;

    if (stuff->delete && (reply.bytesAfter == 0)) {
	xEvent      event;

	event.u.u.type = PropertyNotify;
	event.u.property.window = pWin->drawable.id;
	event.u.property.state = PropertyDelete;
	event.u.property.atom = pProp->propertyName;
	event.u.property.time = currentTime.milliseconds;
	DeliverEvents(pWin, &event, 1, (WindowPtr) NULL);
    }
    LbxWriteGetpropReply(client, &reply);
    if (len) {
	switch (reply.format) {
	case 32:
	    client->pSwapReplyFunc = (ReplySwapPtr)CopySwap32Write;
	    break;
	case 16:
	    client->pSwapReplyFunc = (ReplySwapPtr)CopySwap16Write;
	    break;
	default:
	    client->pSwapReplyFunc = (ReplySwapPtr) WriteToClient;
	    break;
	}
	WriteSwappedDataToClient(client, len,
				 (char *) pProp->data + ind);
    }
    if (stuff->delete && (reply.bytesAfter == 0)) {
	if (pProp->tag_id)
	    TagDeleteTag(pProp->tag_id);
	if (prevProp == (PropertyPtr) NULL) {
	    if (!(pWin->optional->userProps = pProp->next))
		CheckWindowOptionalNeed(pWin);
	} else
	    prevProp->next = pProp->next;
	xfree(pProp->data);
	xfree(pProp);
    }
    return client->noClientException;
}
