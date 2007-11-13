/*

Copyright 2007 Peter Hutterer <peter@cs.unisa.edu.au>

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the author shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from the author.

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
#include <X11/extensions/XInput.h>
#include <X11/extensions/XIproto.h>
#include <X11/extensions/geproto.h>
#include "extnsionst.h"
#include "extinit.h"	/* LookupDeviceIntRec */
#include "exevents.h"
#include "exglobals.h"
#include "geext.h"

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
    deviceHierarchyChangedEvent ev;

    REQUEST(xChangeDeviceHierarchyReq);
    REQUEST_AT_LEAST_SIZE(xChangeDeviceHierarchyReq);

    /* XXX: check if client is allowed to change hierarch */


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
                    int ret;

                    SWAPIF(swaps(&c->namelen, n));
                    name = xcalloc(c->namelen + 1, sizeof(char));
                    strncpy(name, (char*)&c[1], c->namelen);

                    ret = AllocMasterDevice(name, &ptr, &keybd);
                    if (ret != Success)
                    {
                        xfree(name);
                        return ret;
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
                }
                break;
            case CH_RemoveMasterDevice:
                {
                    xRemoveMasterInfo* r = (xRemoveMasterInfo*)any;

                    if (r->returnMode != AttachToMaster &&
                            r->returnMode != Floating)
                        return BadValue;

                    ptr = LookupDeviceIntRec(r->deviceid);
                    if (!ptr || !ptr->isMaster)
                        return BadDevice;

                    /* XXX: For now, don't allow removal of VCP, VCK */
                    if (ptr == inputInfo.pointer ||
                            ptr == inputInfo.keyboard)
                        return BadDevice;

                    /* disable keyboards first */
                    if (IsPointerDevice(ptr))
                        keybd = ptr->spriteInfo->paired;
                    else
                    {
                        keybd = ptr;
                        ptr = keybd->spriteInfo->paired;
                    }

                    /* Disabling sends the devices floating, reattach them if
                     * desired. */
                    if (r->returnMode == AttachToMaster)
                    {
                        DeviceIntPtr attached,
                                     newptr,
                                     newkeybd;

                        newptr = LookupDeviceIntRec(r->returnPointer);
                        newkeybd = LookupDeviceIntRec(r->returnKeyboard);
                        if (!newptr || !newptr->isMaster ||
                                !newkeybd || !newkeybd->isMaster)
                            return BadDevice;

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
                }
                break;
            case CH_ChangeAttachment:
                {
                    xChangeAttachmentInfo* c = (xChangeAttachmentInfo*)any;

                    ptr = LookupDeviceIntRec(c->deviceid);
                    if (!ptr || ptr->isMaster)
                        return BadDevice;

                    if (c->changeMode == Floating)
                        AttachDevice(client, ptr, NULL);
                    else
                    {
                        DeviceIntPtr newmaster = LookupDeviceIntRec(c->newMaster);
                        if (!newmaster || !newmaster->isMaster)
                            return BadDevice;

                        if ((IsPointerDevice(newmaster) &&
                                    !IsPointerDevice(ptr)) ||
                                (IsKeyboardDevice(newmaster) &&
                                 !IsKeyboardDevice(ptr)))
                                return BadDevice;
                        AttachDevice(client, ptr, newmaster);
                    }

                }
                break;
        }

        any = (xAnyHierarchyChangeInfo*)((char*)any + any->length);
    }

    ev.type = GenericEvent;
    ev.extension = IReqCode;
    ev.length = 0;
    ev.evtype = XI_DeviceHierarchyChangedNotify;
    ev.time = GetTimeInMillis();

    SendEventToAllWindows(&dummyDev, XI_DeviceHierarchyChangedMask,
            (xEvent*)&ev, 1);
    return Success;
}

