/*
 * Copyright 2007-2008 Peter Hutterer
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


#define	 NEED_EVENTS
#define	 NEED_REPLIES
#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>	/* for inputstr.h    */
#include <X11/Xproto.h>	/* Request macro     */
#include "inputstr.h"	/* DeviceIntPtr      */
#include "windowstr.h"	/* window structure  */
#include "scrnintstr.h"	/* screen structure  */
#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>
#include <X11/extensions/geproto.h>
#include "extnsionst.h"
#include "exevents.h"
#include "exglobals.h"
#include "geext.h"
#include "xace.h"

#include "chdevhier.h"


/***********************************************************************
 *
 * This procedure allows a client to change the device hierarchy through
 * adding new master devices, removing them, etc.
 *
 */

int SProcXChangeDeviceHierarchy(ClientPtr client)
{
    char n;

    REQUEST(xChangeDeviceHierarchyReq);
    swaps(&stuff->length, n);
    return (ProcXChangeDeviceHierarchy(client));
}

#define SWAPIF(cmd) if (client->swapped) { cmd; }

int
ProcXChangeDeviceHierarchy(ClientPtr client)
{
    DeviceIntPtr ptr, keybd;
    DeviceIntRec dummyDev;
    xAnyHierarchyChangeInfo *any;
    int required_len = sizeof(xChangeDeviceHierarchyReq);
    char n;
    int rc = Success;
    int nchanges = 0;
    deviceHierarchyChangedEvent ev;

    REQUEST(xChangeDeviceHierarchyReq);
    REQUEST_AT_LEAST_SIZE(xChangeDeviceHierarchyReq);

    any = (xAnyHierarchyChangeInfo*)&stuff[1];
    while(stuff->num_changes--)
    {
        SWAPIF(swapl(&any->type, n));
        SWAPIF(swaps(&any->length, n));

        required_len += any->length;
        if ((stuff->length * 4) < required_len)
            return BadLength;

        switch(any->type)
        {
            case CH_CreateMasterDevice:
                {
                    xCreateMasterInfo* c = (xCreateMasterInfo*)any;
                    char* name;

                    SWAPIF(swaps(&c->namelen, n));
                    name = xcalloc(c->namelen + 1, sizeof(char));
                    strncpy(name, (char*)&c[1], c->namelen);


                    rc = AllocMasterDevice(client, name, &ptr, &keybd);
                    if (rc != Success)
                    {
                        xfree(name);
                        goto unwind;
                    }

                    if (!c->sendCore)
                        ptr->coreEvents = keybd->coreEvents =  FALSE;

                    ActivateDevice(ptr);
                    ActivateDevice(keybd);

                    if (c->enable)
                    {
                        EnableDevice(ptr);
                        EnableDevice(keybd);
                    }
                    xfree(name);
                    nchanges++;
                }
                break;
            case CH_RemoveMasterDevice:
                {
                    xRemoveMasterInfo* r = (xRemoveMasterInfo*)any;

                    if (r->returnMode != AttachToMaster &&
                            r->returnMode != Floating)
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

                    /* disable keyboards first */
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


                    /* Disabling sends the devices floating, reattach them if
                     * desired. */
                    if (r->returnMode == AttachToMaster)
                    {
                        DeviceIntPtr attached,
                                     newptr,
                                     newkeybd;

                        rc = dixLookupDevice(&newptr, r->returnPointer,
                                             client, DixWriteAccess);
                        if (rc != Success)
                            goto unwind;

                        if (!newptr->isMaster)
                        {
                            client->errorValue = r->returnPointer;
                            rc = BadDevice;
                            goto unwind;
                        }

                        rc = dixLookupDevice(&newkeybd, r->returnKeyboard,
                                             client, DixWriteAccess);
                        if (rc != Success)
                            goto unwind;

                        if (!newkeybd->isMaster)
                        {
                            client->errorValue = r->returnKeyboard;
                            rc = BadDevice;
                            goto unwind;
                        }

                        for (attached = inputInfo.devices;
                                attached;
                                attached = attached->next)
                        {
                            if (!attached->isMaster) {
                                if (attached->u.master == ptr)
                                    AttachDevice(client, attached, newptr);
                                if (attached->u.master == keybd)
                                    AttachDevice(client, attached, newkeybd);
                            }
                        }
                    }

                    /* can't disable until we removed pairing */
                    keybd->spriteInfo->paired = NULL;
                    ptr->spriteInfo->paired = NULL;
                    DisableDevice(keybd);
                    DisableDevice(ptr);

                    RemoveDevice(keybd);
                    RemoveDevice(ptr);
                    nchanges++;
                }
                break;
            case CH_ChangeAttachment:
                {
                    xChangeAttachmentInfo* c = (xChangeAttachmentInfo*)any;

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

                    if (c->changeMode == Floating)
                        AttachDevice(client, ptr, NULL);
                    else
                    {
                        DeviceIntPtr newmaster;
                        rc = dixLookupDevice(&newmaster, c->newMaster,
                                             client, DixWriteAccess);
                        if (rc != Success)
                            goto unwind;
                        if (!newmaster->isMaster)
                        {
                            client->errorValue = c->newMaster;
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
                    }
                    nchanges++;
                }
                break;
        }

        any = (xAnyHierarchyChangeInfo*)((char*)any + any->length);
    }

unwind:

    if (nchanges > 0) /* even if an error occured, we need to send an event if
                       we changed anything in the hierarchy. */
    {
        ev.type = GenericEvent;
        ev.extension = IReqCode;
        ev.length = 0;
        ev.evtype = XI_DeviceHierarchyChangedNotify;
        ev.time = GetTimeInMillis();

        SendEventToAllWindows(&dummyDev, XI_DeviceHierarchyChangedMask,
                (xEvent*)&ev, 1);
    }
    return rc;
}

