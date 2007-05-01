/*

Copyright 2006 Peter Hutterer <peter@cs.unisa.edu.au>

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
 * Request change pairing between pointer and keyboard device.
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


#include "chpkpair.h"

/***********************************************************************
 *
 * This procedure allows a client to change the pairing of a pointer with a
 * a keyboard.
 *
 */

int SProcXChangePointerKeyboardPairing(ClientPtr client)
{
    char n;

    REQUEST(xChangePointerKeyboardPairingReq);
    swaps(&stuff->length, n);
    return (ProcXChangePointerKeyboardPairing(client));
}

int
ProcXChangePointerKeyboardPairing(ClientPtr client)
{
    DeviceIntPtr pPointer, pKeyboard;
    int ret;
    pairingChangedNotify ev; 

    REQUEST(xChangePointerKeyboardPairingReq);
    REQUEST_SIZE_MATCH(xChangePointerKeyboardPairingReq);

    /* check if client is registered */

    pPointer = LookupDeviceIntRec(stuff->pointer);
    if (pPointer == NULL)
    {
        SendErrorToClient(client, IReqCode, X_ChangePointerKeyboardPairing, 
                stuff->pointer, BadDevice); 
        return Success;
    }

    pKeyboard = LookupDeviceIntRec(stuff->keyboard);
    if (pKeyboard == NULL)
    {
        SendErrorToClient(client, IReqCode, X_ChangePointerKeyboardPairing, 
                stuff->keyboard, BadDevice); 
        return Success;
    }

    ret = PairDevices(client, pPointer, pKeyboard);
    if (ret != Success)
    {
        SendErrorToClient(client, IReqCode, X_ChangePointerKeyboardPairing,
                0, ret);
        return Success;
    }


    memset(&ev, 0, sizeof(pairingChangedNotify));
    GEInitEvent(GEV(&ev), IReqCode);
    ev.evtype = XI_PointerKeyboardPairingChangedNotify;
    ev.pointer = pPointer->id;
    ev.keyboard = pKeyboard->id;
    ev.length = 0;
    ev.time = currentTime.milliseconds;
    SendEventToAllWindows(inputInfo.pointer, 
            XI_PointerKeyboardPairingChangedMask,
            (xEvent*)&ev, 1);
    return Success;
}

/* Event swap proc */
void 
SPointerKeyboardPairingChangedNotifyEvent (pairingChangedNotify *from, 
                                                pairingChangedNotify *to)
{
    char n;

    *to = *from;
    swaps(&to->sequenceNumber, n);
    swapl(&to->time, n);
}
