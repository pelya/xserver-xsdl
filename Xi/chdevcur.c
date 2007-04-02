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
 * Request to change a given device pointer's cursor.
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
#include "extnsionst.h"
#include "extinit.h"	/* LookupDeviceIntRec */
#include "exevents.h"
#include "exglobals.h"

#include "chdevcur.h"

/***********************************************************************
 *
 * This procedure allows a client to set one pointer's cursor.
 *
 */

int
SProcXChangeDeviceCursor(ClientPtr client)
{
    char n;

    REQUEST(xChangeDeviceCursorReq);
    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xChangeDeviceCursorReq);
    return (ProcXChangeDeviceCursor(client));
}

int ProcXChangeDeviceCursor(ClientPtr client)
{
    int err;
    WindowPtr pWin    = NULL;
    DeviceIntPtr pDev = NULL;
    CursorPtr pCursor = NULL;

    REQUEST(xChangeDeviceCursorReq);
    REQUEST_SIZE_MATCH(xChangeDeviceCursorReq);

    pDev = LookupDeviceIntRec(stuff->deviceid);
    if (pDev == NULL) {
        SendErrorToClient(client, IReqCode, X_ChangeDeviceCursor, 0,
                BadDevice); 
        return Success;
    }

    if (stuff->win != None)
    {
        err = dixLookupWindow(&pWin, stuff->win, client, DixReadWriteAccess);
        if (err != Success)
        {
            SendErrorToClient(client, IReqCode, X_ChangeDeviceCursor, 
                    stuff->win, err);
            return Success;
        }
    }

    if (stuff->cursor == None)
    {
        if (pWin == WindowTable[pWin->drawable.pScreen->myNum])
            pCursor = rootCursor;
        else
            pCursor = (CursorPtr)None;
    } 
    else 
    {
        pCursor = (CursorPtr)SecurityLookupIDByType(client, stuff->cursor,
                                RT_CURSOR, DixReadAccess); 
        if (!pCursor)
        {
            SendErrorToClient(client, IReqCode, X_ChangeDeviceCursor, 
                    stuff->cursor, BadCursor);
            return Success;
        }
    } 

    ChangeWindowDeviceCursor(pWin, pDev, pCursor);

    return Success;
}

