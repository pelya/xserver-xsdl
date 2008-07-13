/*
 * Copyright © 2006 Keith Packard
 * Copyright © 2008 Peter Hutterer
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WAXIANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WAXIANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

/* This code is a modified version of randr/rrproperty.c */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "dix.h"
#include "inputstr.h"
#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>
#include "exglobals.h"
#include "exevents.h"
#include "swaprep.h"

#include "xiproperty.h"

static long XIPropHandlerID = 1;

/* Registers a new property handler on the given device and returns a unique
 * identifier for this handler. This identifier is required to unregister the
 * property handler again.
 * @return The handler's identifier or 0 if an error occured.
 */
long
XIRegisterPropertyHandler(DeviceIntPtr         dev,
                          Bool (*SetProperty) (DeviceIntPtr dev,
                                               Atom property,
                                               XIPropertyValuePtr prop),
                          Bool (*GetProperty) (DeviceIntPtr dev,
                                               Atom property))
{
    XIPropertyHandlerPtr new_handler;

    new_handler = xcalloc(1, sizeof(XIPropertyHandler));
    if (!new_handler)
        return 0;

    new_handler->id = XIPropHandlerID++;
    new_handler->SetProperty = SetProperty;
    new_handler->GetProperty = GetProperty;
    new_handler->next = dev->properties.handlers;
    dev->properties.handlers = new_handler;

    return new_handler->id;
}

void
XIUnRegisterPropertyHandler(DeviceIntPtr dev, long id)
{
    XIPropertyHandlerPtr curr, prev = NULL;

    curr = dev->properties.handlers;
    while(curr && curr->id != id)
    {
        prev = curr;
        curr = curr->next;
    }

    if (!curr)
        return;

    if (!prev) /* first one */
        dev->properties.handlers = curr->next;
    else
        prev->next = curr->next;

    xfree(curr);
}

static void
XIInitDevicePropertyValue (XIPropertyValuePtr property_value)
{
    property_value->type   = None;
    property_value->format = 0;
    property_value->size   = 0;
    property_value->data   = NULL;
}

static XIPropertyPtr
XICreateDeviceProperty (Atom property)
{
    XIPropertyPtr   prop;

    prop = (XIPropertyPtr)xalloc(sizeof(XIPropertyRec));
    if (!prop)
        return NULL;

    prop->next         = NULL;
    prop->propertyName = property;
    prop->is_pending   = FALSE;
    prop->range        = FALSE;
    prop->fromClient   = FALSE;
    prop->immutable    = FALSE;
    prop->num_valid    = 0;
    prop->valid_values = NULL;

    XIInitDevicePropertyValue (&prop->current);
    XIInitDevicePropertyValue (&prop->pending);
    return prop;
}

static void
XIDestroyDeviceProperty (XIPropertyPtr prop)
{
    if (prop->valid_values)
        xfree (prop->valid_values);
    if (prop->current.data)
        xfree(prop->current.data);
    if (prop->pending.data)
        xfree(prop->pending.data);
    xfree(prop);
}

/* This function destroys all of the device's property-related stuff,
 * including removing all device handlers.
 * DO NOT CALL FROM THE DRIVER.
 */
void
XIDeleteAllDeviceProperties (DeviceIntPtr device)
{
    XIPropertyPtr               prop, next;
    XIPropertyHandlerPtr        curr_handler, next_handler;
    devicePropertyNotifyEvent   event;

    for (prop = device->properties.properties; prop; prop = next)
    {
        next = prop->next;

        event.type      = GenericEvent;
        event.extension = IReqCode;
        event.evtype    = XI_DevicePropertyNotify;
        event.length    = 0;
        event.deviceid  = device->id;
        event.state     = PropertyDelete;
        event.atom      = prop->propertyName;
        event.time      = currentTime.milliseconds;
        SendEventToAllWindows(device, XI_DevicePropertyNotifyMask,
                (xEvent*)&event, 1);

        XIDestroyDeviceProperty(prop);
    }

    /* Now free all handlers */
    curr_handler = device->properties.handlers;
    while(curr_handler)
    {
        next_handler = curr_handler->next;
        xfree(curr_handler);
        curr_handler = next_handler;
    }
}


int
XIDeleteDeviceProperty (DeviceIntPtr device, Atom property, Bool fromClient)
{
    XIPropertyPtr               prop, *prev;
    devicePropertyNotifyEvent   event;

    for (prev = &device->properties.properties; (prop = *prev); prev = &(prop->next))
        if (prop->propertyName == property)
            break;

    if (!prop->fromClient && fromClient)
        return BadAtom;

    if (prop)
    {
        *prev = prop->next;
        event.type      = GenericEvent;
        event.extension = IReqCode;
        event.length    = 0;
        event.evtype    = XI_DevicePropertyNotify;
        event.deviceid  = device->id;
        event.state     = PropertyDelete;
        event.atom      = prop->propertyName;
        event.time      = currentTime.milliseconds;
        SendEventToAllWindows(device, XI_DevicePropertyNotifyMask,
                              (xEvent*)&event, 1);
        XIDestroyDeviceProperty (prop);
    }

    return Success;
}

int
XIChangeDeviceProperty (DeviceIntPtr dev, Atom property, Atom type,
                        int format, int mode, unsigned long len,
                        pointer value, Bool sendevent, Bool pending,
                        Bool fromClient)
{
    XIPropertyPtr               prop;
    devicePropertyNotifyEvent   event;
    int                         size_in_bytes;
    int                         total_size;
    unsigned long               total_len;
    XIPropertyValuePtr          prop_value;
    XIPropertyValueRec          new_value;
    Bool                        add = FALSE;

    size_in_bytes = format >> 3;

    /* first see if property already exists */
    prop = XIQueryDeviceProperty (dev, property);
    if (!prop)   /* just add to list */
    {
        prop = XICreateDeviceProperty (property);
        if (!prop)
            return(BadAlloc);
        prop->fromClient = fromClient;
        add = TRUE;
        mode = PropModeReplace;
    }
    if (pending && prop->is_pending)
        prop_value = &prop->pending;
    else
        prop_value = &prop->current;

    /* To append or prepend to a property the request format and type
     must match those of the already defined property.  The
     existing format and type are irrelevant when using the mode
     "PropModeReplace" since they will be written over. */

    if ((format != prop_value->format) && (mode != PropModeReplace))
        return(BadMatch);
    if ((prop_value->type != type) && (mode != PropModeReplace))
        return(BadMatch);
    new_value = *prop_value;
    if (mode == PropModeReplace)
        total_len = len;
    else
        total_len = prop_value->size + len;

    if (mode == PropModeReplace || len > 0)
    {
        pointer            new_data = NULL, old_data = NULL;

        total_size = total_len * size_in_bytes;
        new_value.data = (pointer)xalloc (total_size);
        if (!new_value.data && total_size)
        {
            if (add)
                XIDestroyDeviceProperty (prop);
            return BadAlloc;
        }
        new_value.size = len;
        new_value.type = type;
        new_value.format = format;

        switch (mode) {
        case PropModeReplace:
            new_data = new_value.data;
            old_data = NULL;
            break;
        case PropModeAppend:
            new_data = (pointer) (((char *) new_value.data) +
                                  (prop_value->size * size_in_bytes));
            old_data = new_value.data;
            break;
        case PropModePrepend:
            new_data = new_value.data;
            old_data = (pointer) (((char *) new_value.data) +
                                  (prop_value->size * size_in_bytes));
            break;
        }
        if (new_data)
            memcpy ((char *) new_data, (char *) value, len * size_in_bytes);
        if (old_data)
            memcpy ((char *) old_data, (char *) prop_value->data,
                    prop_value->size * size_in_bytes);

        /* We must set pendingProperties TRUE before we commit to the driver,
           we're in a single thread after all
         */
        if (pending && prop->is_pending)
            dev->properties.pendingProperties = TRUE;
        if (pending && dev->properties.handlers)
        {
            XIPropertyHandlerPtr handler = dev->properties.handlers;
            while(handler)
            {
                if (handler->SetProperty &&
                    !handler->SetProperty(dev, prop->propertyName, &new_value))
                {
                    if (new_value.data)
                        xfree (new_value.data);
                    return (BadValue);
                }
                handler = handler->next;
            }
        }
        if (prop_value->data)
            xfree (prop_value->data);
        *prop_value = new_value;
    }

    else if (len == 0)
    {
        /* do nothing */
    }

    if (add)
    {
        prop->next = dev->properties.properties;
        dev->properties.properties = prop;
    }


    if (sendevent)
    {
        event.type      = GenericEvent;
        event.extension = IReqCode;
        event.length    = 0;
        event.evtype    = XI_DevicePropertyNotify;
        event.deviceid  = dev->id;
        event.state     = PropertyNewValue;
        event.atom      = prop->propertyName;
        event.time      = currentTime.milliseconds;
        SendEventToAllWindows(dev, XI_DevicePropertyNotifyMask,
                              (xEvent*)&event, 1);
    }
    return(Success);
}

XIPropertyPtr
XIQueryDeviceProperty (DeviceIntPtr dev, Atom property)
{
    XIPropertyPtr   prop;

    for (prop = dev->properties.properties; prop; prop = prop->next)
        if (prop->propertyName == property)
            return prop;
    return NULL;
}

XIPropertyValuePtr
XIGetDeviceProperty (DeviceIntPtr dev, Atom property, Bool pending)
{
    XIPropertyPtr   prop = XIQueryDeviceProperty (dev, property);

    if (!prop)
        return NULL;
    if (pending && prop->is_pending)
        return &prop->pending;
    else {
        /* If we can, try to update the property value first */
        if (dev->properties.handlers)
        {
            XIPropertyHandlerPtr handler = dev->properties.handlers;
            while(handler)
            {
                if (handler->GetProperty)
                    handler->GetProperty(dev, prop->propertyName);
                handler = handler->next;
            }
        }
        return &prop->current;
    }
}

int
XIConfigureDeviceProperty (DeviceIntPtr dev, Atom property,
                           Bool pending, Bool range, Bool immutable,
                           int num_values, INT32 *values)
{
    XIPropertyPtr   prop = XIQueryDeviceProperty (dev, property);
    Bool            add = FALSE;
    INT32           *new_values;

    if (!prop)
    {
        prop = XICreateDeviceProperty (property);
        if (!prop)
            return(BadAlloc);
        add = TRUE;
    } else if (prop->immutable && !immutable)
        return(BadAccess);

    /*
     * ranges must have even number of values
     */
    if (range && (num_values & 1))
        return BadMatch;

    new_values = xalloc (num_values * sizeof (INT32));
    if (!new_values && num_values)
        return BadAlloc;
    if (num_values)
        memcpy (new_values, values, num_values * sizeof (INT32));

    /*
     * Property moving from pending to non-pending
     * loses any pending values
     */
    if (prop->is_pending && !pending)
    {
        if (prop->pending.data)
            xfree (prop->pending.data);
        XIInitDevicePropertyValue (&prop->pending);
    }

    prop->is_pending = pending;
    prop->range = range;
    prop->immutable = immutable;
    prop->num_valid = num_values;
    if (prop->valid_values)
        xfree (prop->valid_values);
    prop->valid_values = new_values;

    if (add) {
        prop->next = dev->properties.properties;
        dev->properties.properties = prop;
    }

    return Success;
}

int
ProcXListDeviceProperties (ClientPtr client)
{
    Atom                        *pAtoms = NULL, *temppAtoms;
    xListDevicePropertiesReply  rep;
    int                         numProps = 0;
    DeviceIntPtr                dev;
    XIPropertyPtr               prop;
    int                         rc = Success;

    REQUEST(xListDevicePropertiesReq);
    REQUEST_SIZE_MATCH(xListDevicePropertiesReq);

    rc = dixLookupDevice (&dev, stuff->deviceid, client, DixReadAccess);
    if (rc != Success)
        return rc;

    for (prop = dev->properties.properties; prop; prop = prop->next)
        numProps++;
    if (numProps)
        if(!(pAtoms = (Atom *)xalloc(numProps * sizeof(Atom))))
            return(BadAlloc);

    rep.repType = X_Reply;
    rep.RepType = X_ListDeviceProperties;
    rep.length = (numProps * sizeof(Atom)) >> 2;
    rep.sequenceNumber = client->sequence;
    rep.nAtoms = numProps;
    if (client->swapped)
    {
        int n;
        swaps (&rep.sequenceNumber, n);
        swapl (&rep.length, n);
    }
    temppAtoms = pAtoms;
    for (prop = dev->properties.properties; prop; prop = prop->next)
        *temppAtoms++ = prop->propertyName;

    WriteReplyToClient(client, sizeof(xListDevicePropertiesReply), &rep);
    if (numProps)
    {
        client->pSwapReplyFunc = (ReplySwapPtr)Swap32Write;
        WriteSwappedDataToClient(client, numProps * sizeof(Atom), pAtoms);
        xfree(pAtoms);
    }
    return rc;
}

int
ProcXQueryDeviceProperty (ClientPtr client)
{
    REQUEST(xQueryDevicePropertyReq);
    xQueryDevicePropertyReply   rep;
    DeviceIntPtr                dev;
    XIPropertyPtr               prop;
    int                         rc;

    REQUEST_SIZE_MATCH(xQueryDevicePropertyReq);

    rc = dixLookupDevice (&dev, stuff->deviceid, client, DixReadAccess);

    if (rc != Success)
        return rc;

    prop = XIQueryDeviceProperty (dev, stuff->property);
    if (!prop)
        return BadName;

    rep.repType = X_Reply;
    rep.length = prop->num_valid;
    rep.sequenceNumber = client->sequence;
    rep.pending = prop->is_pending;
    rep.range = prop->range;
    rep.immutable = prop->immutable;
    rep.fromClient = prop->fromClient;
    if (client->swapped)
    {
        int n;
        swaps (&rep.sequenceNumber, n);
        swapl (&rep.length, n);
    }
    WriteReplyToClient (client, sizeof (xQueryDevicePropertyReply), &rep);
    if (prop->num_valid)
    {
        client->pSwapReplyFunc = (ReplySwapPtr)Swap32Write;
        WriteSwappedDataToClient(client, prop->num_valid * sizeof(INT32),
                                 prop->valid_values);
    }
    return(client->noClientException);
}

int
ProcXConfigureDeviceProperty (ClientPtr client)
{
    REQUEST(xConfigureDevicePropertyReq);
    DeviceIntPtr        dev;
    int                 num_valid;
    int                 rc;

    REQUEST_AT_LEAST_SIZE(xConfigureDevicePropertyReq);

    rc = dixLookupDevice (&dev, stuff->deviceid, client, DixReadAccess);

    if (rc != Success)
        return rc;

    num_valid = stuff->length - (sizeof (xConfigureDevicePropertyReq) >> 2);
    return XIConfigureDeviceProperty (dev, stuff->property,
                                      stuff->pending, stuff->range,
                                      FALSE, num_valid,
                                      (INT32 *) (stuff + 1));
}

int
ProcXChangeDeviceProperty (ClientPtr client)
{
    REQUEST(xChangeDevicePropertyReq);
    DeviceIntPtr        dev;
    char                format, mode;
    unsigned long       len;
    int                 sizeInBytes;
    int                 totalSize;
    int                 rc;

    REQUEST_AT_LEAST_SIZE(xChangeDevicePropertyReq);
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
    REQUEST_FIXED_SIZE(xChangeDevicePropertyReq, totalSize);

    rc = dixLookupDevice (&dev, stuff->deviceid, client, DixWriteAccess);
    if (rc != Success)
        return rc;

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

    rc = XIChangeDeviceProperty(dev, stuff->property,
                                 stuff->type, (int)format,
                                 (int)mode, len, (pointer)&stuff[1], TRUE,
                                 TRUE, TRUE);

    return rc;
}

int
ProcXDeleteDeviceProperty (ClientPtr client)
{
    REQUEST(xDeleteDevicePropertyReq);
    DeviceIntPtr        dev;
    int                 rc;

    REQUEST_SIZE_MATCH(xDeleteDevicePropertyReq);
    UpdateCurrentTime();
    rc =  dixLookupDevice (&dev, stuff->deviceid, client, DixWriteAccess);
    if (rc != Success)
        return rc;

    if (!ValidAtom(stuff->property))
    {
        client->errorValue = stuff->property;
        return (BadAtom);
    }


    rc = XIDeleteDeviceProperty(dev, stuff->property, TRUE);
    return rc;
}

int
ProcXGetDeviceProperty (ClientPtr client)
{
    REQUEST(xGetDevicePropertyReq);
    XIPropertyPtr               prop, *prev;
    XIPropertyValuePtr          prop_value;
    unsigned long               n, len, ind;
    DeviceIntPtr                dev;
    xGetDevicePropertyReply     reply;
    int                         rc;

    REQUEST_SIZE_MATCH(xGetDevicePropertyReq);
    if (stuff->delete)
        UpdateCurrentTime();
    rc = dixLookupDevice (&dev, stuff->deviceid, client,
                           stuff->delete ? DixWriteAccess :
                           DixReadAccess);
    if (rc != Success)
        return rc;

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

    for (prev = &dev->properties.properties; (prop = *prev); prev = &prop->next)
        if (prop->propertyName == stuff->property)
            break;

    reply.repType = X_Reply;
    reply.RepType = X_GetDeviceProperty;
    reply.sequenceNumber = client->sequence;
    reply.deviceid = dev->id;
    if (!prop)
    {
        reply.nItems = 0;
        reply.length = 0;
        reply.bytesAfter = 0;
        reply.propertyType = None;
        reply.format = 0;
        WriteReplyToClient(client, sizeof(xGetDevicePropertyReply), &reply);
        return(client->noClientException);
    }

    if (prop->immutable && stuff->delete)
        return BadAccess;

    prop_value = XIGetDeviceProperty(dev, stuff->property, stuff->pending);
    if (!prop_value)
        return BadAtom;

    /* If the request type and actual type don't match. Return the
    property information, but not the data. */

    if (((stuff->type != prop_value->type) &&
         (stuff->type != AnyPropertyType))
       )
    {
        reply.bytesAfter = prop_value->size;
        reply.format = prop_value->format;
        reply.length = 0;
        reply.nItems = 0;
        reply.propertyType = prop_value->type;
        WriteReplyToClient(client, sizeof(xGetDevicePropertyReply), &reply);
        return(client->noClientException);
    }

/*
 *  Return type, format, value to client
 */
    n = (prop_value->format/8) * prop_value->size; /* size (bytes) of prop */
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
    reply.format = prop_value->format;
    reply.length = (len + 3) >> 2;
    if (prop_value->format)
        reply.nItems = len / (prop_value->format / 8);
    else
        reply.nItems = 0;
    reply.propertyType = prop_value->type;

    if (stuff->delete && (reply.bytesAfter == 0))
    {
        devicePropertyNotifyEvent    event;

        event.type      = GenericEvent;
        event.extension = IReqCode;
        event.length    = 0;
        event.evtype    = XI_DevicePropertyNotify;
        event.deviceid  = dev->id;
        event.state     = PropertyDelete;
        event.atom      = prop->propertyName;
        event.time      = currentTime.milliseconds;
        SendEventToAllWindows(dev, XI_DevicePropertyNotifyMask,
                              (xEvent*)&event, 1);
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
                                 (char *)prop_value->data + ind);
    }

    if (stuff->delete && (reply.bytesAfter == 0))
    { /* delete the Property */
        *prev = prop->next;
        XIDestroyDeviceProperty (prop);
    }
    return(client->noClientException);
}


int
SProcXListDeviceProperties (ClientPtr client)
{
    REQUEST(xListDevicePropertiesReq);

    REQUEST_SIZE_MATCH(xListDevicePropertiesReq);
    (void) stuff;
    return BadImplementation;
}

int
SProcXQueryDeviceProperty (ClientPtr client)
{
    REQUEST(xQueryDevicePropertyReq);

    REQUEST_SIZE_MATCH(xQueryDevicePropertyReq);
    (void) stuff;
    return BadImplementation;
}

int
SProcXConfigureDeviceProperty (ClientPtr client)
{
    REQUEST(xConfigureDevicePropertyReq);

    REQUEST_SIZE_MATCH(xConfigureDevicePropertyReq);
    (void) stuff;
    return BadImplementation;
}

int
SProcXChangeDeviceProperty (ClientPtr client)
{
    REQUEST(xChangeDevicePropertyReq);

    REQUEST_SIZE_MATCH(xChangeDevicePropertyReq);
    (void) stuff;
    return BadImplementation;
}

int
SProcXDeleteDeviceProperty (ClientPtr client)
{
    REQUEST(xDeleteDevicePropertyReq);

    REQUEST_SIZE_MATCH(xDeleteDevicePropertyReq);
    (void) stuff;
    return BadImplementation;
}

int
SProcXGetDeviceProperty (ClientPtr client)
{
    REQUEST(xGetDevicePropertyReq);

    REQUEST_SIZE_MATCH(xGetDevicePropertyReq);
    (void) stuff;
    return BadImplementation;
}

