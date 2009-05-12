/*
 * Copyright 2007-2008 Peter Hutterer
 * Copyright 2009 Red Hat, Inc.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author: Peter Hutterer, University of South Australia, NICTA
 */

/***********************************************************************
 *
 * Request change in the device hierarchy.
 *
 */


#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>	/* for inputstr.h    */
#include <X11/Xproto.h>	/* Request macro     */
#include "inputstr.h"	/* DeviceIntPtr      */
#include "windowstr.h"	/* window structure  */
#include "scrnintstr.h"	/* screen structure  */
#include <X11/extensions/XI.h>
#include <X11/extensions/XI2proto.h>
#include <X11/extensions/geproto.h>
#include "extnsionst.h"
#include "exevents.h"
#include "exglobals.h"
#include "geext.h"
#include "xace.h"
#include "querydev.h" /* for GetDeviceUse */

#include "xkbsrv.h"

#include "chdevhier.h"

extern DevPrivateKey XTstDevicePrivateKey;

/**
 * Send the current state of the device hierarchy to all clients.
 */
void XISendDeviceHierarchyEvent(int flags[MAXDEVICES])
{
    xXIDeviceHierarchyEvent *ev;
    xXIHierarchyInfo *info;
    DeviceIntRec dummyDev;
    DeviceIntPtr dev;
    int i;

    if (!flags)
        return;

    ev = xcalloc(1, sizeof(xXIDeviceHierarchyEvent) +
                 MAXDEVICES * sizeof(xXIHierarchyInfo));
    ev->type = GenericEvent;
    ev->extension = IReqCode;
    ev->evtype = XI_HierarchyChanged;
    ev->time = GetTimeInMillis();
    ev->flags = 0;
    ev->num_devices = inputInfo.numDevices;

    info = (xXIHierarchyInfo*)&ev[1];
    for (dev = inputInfo.devices; dev; dev = dev->next)
    {
        info->deviceid = dev->id;
        info->enabled = dev->enabled;
        info->use = GetDeviceUse(dev, &info->attachment);
        info->flags = flags[dev->id];
        ev->flags |= info->flags;
        info++;
    }
    for (dev = inputInfo.off_devices; dev; dev = dev->next)
    {
        info->deviceid = dev->id;
        info->enabled = dev->enabled;
        info->use = GetDeviceUse(dev, &info->attachment);
        info->flags = flags[dev->id];
        ev->flags |= info->flags;
        info++;
    }


    for (i = 0; i < MAXDEVICES; i++)
    {
        if (flags[i] & (XIMasterRemoved | XISlaveRemoved))
        {
            info->deviceid = i;
            info->enabled = FALSE;
            info->flags = flags[i];
            info->use = 0;
            ev->flags |= info->flags;
            ev->num_devices++;
            info++;
        }
    }

    ev->length = (ev->num_devices * sizeof(xXIHierarchyInfo))/4;

    dummyDev.id = XIAllDevices;
    SendEventToAllWindows(&dummyDev, (XI_HierarchyChangedMask >> 8), (xEvent*)ev, 1);
}


/***********************************************************************
 *
 * This procedure allows a client to change the device hierarchy through
 * adding new master devices, removing them, etc.
 *
 */

int SProcXIChangeDeviceHierarchy(ClientPtr client)
{
    char n;

    REQUEST(xXIChangeDeviceHierarchyReq);
    swaps(&stuff->length, n);
    return (ProcXIChangeDeviceHierarchy(client));
}

#define SWAPIF(cmd) if (client->swapped) { cmd; }

int
ProcXIChangeDeviceHierarchy(ClientPtr client)
{
    DeviceIntPtr ptr, keybd, xtstptr, xtstkeybd;
    xXIAnyHierarchyChangeInfo *any;
    int required_len = sizeof(xXIChangeDeviceHierarchyReq);
    char n;
    int rc = Success;
    int flags[MAXDEVICES] = {0};

    REQUEST(xXIChangeDeviceHierarchyReq);
    REQUEST_AT_LEAST_SIZE(xXIChangeDeviceHierarchyReq);

    if (!stuff->num_changes)
        return rc;

    any = (xXIAnyHierarchyChangeInfo*)&stuff[1];
    while(stuff->num_changes--)
    {
        SWAPIF(swapl(&any->type, n));
        SWAPIF(swaps(&any->length, n));

        required_len += any->length;
        if ((stuff->length * 4) < required_len)
            return BadLength;

        switch(any->type)
        {
            case XICreateMasterDevice:
                {
                    xXICreateMasterInfo* c = (xXICreateMasterInfo*)any;
                    char* name;

                    SWAPIF(swaps(&c->name_len, n));
                    name = xcalloc(c->name_len + 1, sizeof(char));
                    strncpy(name, (char*)&c[1], c->name_len);


                    rc = AllocDevicePair(client, name, &ptr, &keybd, TRUE);
                    if (rc != Success)
                    {
                        xfree(name);
                        goto unwind;
                    }

                    if (!c->send_core)
                        ptr->coreEvents = keybd->coreEvents =  FALSE;

		    /* Allocate virtual slave devices for xtest events */
                    rc = AllocXtstDevice(client, name, &xtstptr, &xtstkeybd);
                    if (rc != Success)
                    {

                        xfree(name);
                        goto unwind;
                    }

                    ActivateDevice(ptr);
                    ActivateDevice(keybd);
                    flags[ptr->id] |= XIMasterAdded;
                    flags[keybd->id] |= XIMasterAdded;

                    ActivateDevice(xtstptr);
                    ActivateDevice(xtstkeybd);
                    flags[xtstptr->id] |= XISlaveAdded;
                    flags[xtstkeybd->id] |= XISlaveAdded;

                    if (c->enable)
                    {
                        EnableDevice(ptr);
                        EnableDevice(keybd);
                        flags[ptr->id] |= XIDeviceEnabled;
                        flags[keybd->id] |= XIDeviceEnabled;

                        EnableDevice(xtstptr);
                        EnableDevice(xtstkeybd);
                        flags[xtstptr->id] |= XIDeviceEnabled;
                        flags[xtstkeybd->id] |= XIDeviceEnabled;
                    }

                    /* Attach the XTest virtual devices to the newly
                       created master device */
                    AttachDevice(NULL, xtstptr, ptr);
                    AttachDevice(NULL, xtstkeybd, keybd);
                    flags[xtstptr->id] |= XISlaveAttached;
                    flags[xtstkeybd->id] |= XISlaveAttached;

                    xfree(name);
                }
                break;
            case XIRemoveMasterDevice:
                {
                    xXIRemoveMasterInfo* r = (xXIRemoveMasterInfo*)any;
                    DeviceIntPtr xtstdevice;

                    if (r->return_mode != XIAttachToMaster &&
                            r->return_mode != XIFloating)
                        return BadValue;

                    rc = dixLookupDevice(&ptr, r->deviceid, client,
                                         DixDestroyAccess);
                    if (rc != Success)
                        goto unwind;

                    if (!ptr->isMaster)
                    {
                        client->errorValue = r->deviceid;
                        rc = BadDevice;
                        goto unwind;
                    }

                    /* XXX: For now, don't allow removal of VCP, VCK */
                    if (ptr == inputInfo.pointer ||
                            ptr == inputInfo.keyboard)
                    {
                        rc = BadDevice;
                        goto unwind;
                    }

                    for(xtstdevice = inputInfo.devices; xtstdevice ; xtstdevice = xtstdevice->next )
                        if( !xtstdevice->isMaster && xtstdevice->u.master == ptr &&
                            dixLookupPrivate(&xtstdevice->devPrivates, XTstDevicePrivateKey ))
                            break;

                    rc = dixLookupDevice(&xtstptr, xtstdevice->id, client,
                                         DixDestroyAccess);
                    if (rc != Success)
                        goto unwind;

                    /* find keyboards to destroy */
                    if (IsPointerDevice(ptr))
                    {
                        rc = dixLookupDevice(&keybd,
                                             ptr->spriteInfo->paired->id,
                                             client,
                                             DixDestroyAccess);
                        if (rc != Success)
                            goto unwind;

                    }
                    else
                    {
                        keybd = ptr;
                        rc = dixLookupDevice(&ptr,
                                             keybd->spriteInfo->paired->id,
                                             client,
                                             DixDestroyAccess);
                        if (rc != Success)
                            goto unwind;

                    }

                    /* handle xtst pointer / keyboard slave devices */
                    if ( IsPointerDevice(xtstptr))
                    {
                        /* Search the matching keyboard */
                        for(xtstdevice = inputInfo.devices; xtstdevice ; xtstdevice = xtstdevice->next )
                            if( !xtstdevice->isMaster &&
                                xtstdevice->u.master == keybd &&
                                IsKeyboardDevice(xtstdevice) &&
                                dixLookupPrivate(&xtstdevice->devPrivates, XTstDevicePrivateKey ))
                                break;

                        rc = dixLookupDevice(&xtstkeybd,
                                             xtstdevice->id,
                                             client,
                                             DixDestroyAccess);

                        if (rc != Success)
                            goto unwind;
                    }
                    else
                    {
                        xtstkeybd = xtstptr;
                        /* Search the matching pointer */
                        for(xtstdevice = inputInfo.devices; xtstdevice ; xtstdevice = xtstdevice->next )
                            if( !xtstdevice->isMaster &&
                                xtstdevice->u.master == ptr &&
                                IsPointerDevice(xtstdevice) &&
                                dixLookupPrivate(&xtstdevice->devPrivates, XTstDevicePrivateKey )
                              )
                                break;
                        rc = dixLookupDevice(&xtstptr,
                                             xtstdevice->id,
                                             client,
                                             DixDestroyAccess);

                        if (rc != Success)
                            goto unwind;
                    }

                    /* Disabling sends the devices floating, reattach them if
                     * desired. */
                    if (r->return_mode == XIAttachToMaster)
                    {
                        DeviceIntPtr attached,
                                     newptr,
                                     newkeybd;

                        rc = dixLookupDevice(&newptr, r->return_pointer,
                                             client, DixWriteAccess);
                        if (rc != Success)
                            goto unwind;

                        if (!newptr->isMaster)
                        {
                            client->errorValue = r->return_pointer;
                            rc = BadDevice;
                            goto unwind;
                        }

                        rc = dixLookupDevice(&newkeybd, r->return_keyboard,
                                             client, DixWriteAccess);
                        if (rc != Success)
                            goto unwind;

                        if (!newkeybd->isMaster)
                        {
                            client->errorValue = r->return_keyboard;
                            rc = BadDevice;
                            goto unwind;
                        }

                        for (attached = inputInfo.devices;
                                attached;
                                attached = attached->next)
                        {
                            if (!attached->isMaster) {
                                if (attached->u.master == ptr)
                                {
                                    AttachDevice(client, attached, newptr);
                                    flags[attached->id] |= XISlaveAttached;
                                }
                                if (attached->u.master == keybd)
                                {
                                    AttachDevice(client, attached, newkeybd);
                                    flags[attached->id] |= XISlaveAttached;
                                }
                            }
                        }
                    }

                    /* can't disable until we removed pairing */
                    keybd->spriteInfo->paired = NULL;
                    ptr->spriteInfo->paired = NULL;
                    xtstptr->spriteInfo->paired = NULL;
                    xtstkeybd->spriteInfo->paired = NULL;

                    /* disable the remove the devices, xtst devices must be done first
                       else the sprites they rely on will be destroyed  */
                    DisableDevice(xtstptr);
                    DisableDevice(xtstkeybd);
                    DisableDevice(keybd);
                    DisableDevice(ptr);
                    flags[xtstptr->id] |= XIDeviceDisabled | XISlaveDetached;
                    flags[xtstkeybd->id] |= XIDeviceDisabled | XISlaveDetached;
                    flags[keybd->id] |= XIDeviceDisabled;
                    flags[ptr->id] |= XIDeviceDisabled;

                    RemoveDevice(xtstptr);
                    RemoveDevice(xtstkeybd);
                    RemoveDevice(keybd);
                    RemoveDevice(ptr);
                    flags[xtstptr->id] |= XISlaveRemoved;
                    flags[xtstkeybd->id] |= XISlaveRemoved;
                    flags[keybd->id] |= XIMasterRemoved;
                    flags[ptr->id] |= XIMasterRemoved;
                }
                break;
            case XIDetachSlave:
                {
                    xXIDetachSlaveInfo* c = (xXIDetachSlaveInfo*)any;
                    DeviceIntPtr *xtstdevice;

                    rc = dixLookupDevice(&ptr, c->deviceid, client,
                                          DixWriteAccess);
                    if (rc != Success)
                       goto unwind;

                    if (ptr->isMaster)
                    {
                        client->errorValue = c->deviceid;
                        rc = BadDevice;
                        goto unwind;
                    }

                    xtstdevice = dixLookupPrivate( &ptr->devPrivates,
                                                   XTstDevicePrivateKey );

                    /* Don't allow changes to Xtst Devices, these are fixed */
                    if( xtstdevice )
                    {
                        client->errorValue = c->deviceid;
                        rc = BadDevice;
                        goto unwind;
                    }

                    AttachDevice(client, ptr, NULL);
                    flags[ptr->id] |= XISlaveDetached;
                }
                break;
            case XIAttachSlave:
                {
                    xXIAttachSlaveInfo* c = (xXIAttachSlaveInfo*)any;
                    DeviceIntPtr newmaster;
                    DeviceIntPtr *xtstdevice;

                    rc = dixLookupDevice(&ptr, c->deviceid, client,
                                          DixWriteAccess);
                    if (rc != Success)
                       goto unwind;

                    if (ptr->isMaster)
                    {
                        client->errorValue = c->deviceid;
                        rc = BadDevice;
                        goto unwind;
                    }

                    xtstdevice = dixLookupPrivate( &ptr->devPrivates,
                                                   XTstDevicePrivateKey );

                    /* Don't allow changes to Xtst Devices, these are fixed */
                    if( xtstdevice )
                    {
                        client->errorValue = c->deviceid;
                        rc = BadDevice;
                        goto unwind;
                    }

                    rc = dixLookupDevice(&newmaster, c->new_master,
                            client, DixWriteAccess);
                    if (rc != Success)
                        goto unwind;
                    if (!newmaster->isMaster)
                    {
                        client->errorValue = c->new_master;
                        rc = BadDevice;
                        goto unwind;
                    }

                    if (!((IsPointerDevice(newmaster) &&
                                    IsPointerDevice(ptr)) ||
                                (IsKeyboardDevice(newmaster) &&
                                 IsKeyboardDevice(ptr))))
                    {
                        rc = BadDevice;
                        goto unwind;
                    }
                    AttachDevice(client, ptr, newmaster);
                    flags[ptr->id] |= XISlaveAttached;
                }
                break;
        }

        any = (xXIAnyHierarchyChangeInfo*)((char*)any + any->length * 4);
    }

unwind:

    XISendDeviceHierarchyEvent(flags);
    return rc;
}

