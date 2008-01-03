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
 * Request to Warp the pointer location of an extension input device.
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
#include "exevents.h"
#include "exglobals.h"


#include "warpdevp.h"
/***********************************************************************
 *
 * This procedure allows a client to warp the pointer of a device.
 *
 */

int
SProcXWarpDevicePointer(ClientPtr client)
{
    char n;

    REQUEST(xWarpDevicePointerReq);
    swaps(&stuff->length, n);
    return (ProcXWarpDevicePointer(client));
}

int
ProcXWarpDevicePointer(ClientPtr client)
{
    int err;
    int x, y;
    WindowPtr dest = NULL;
    DeviceIntPtr pDev;
    SpritePtr pSprite;
    ScreenPtr newScreen;

    REQUEST(xWarpDevicePointerReq);
    REQUEST_SIZE_MATCH(xWarpDevicePointerReq);

    /* FIXME: panoramix stuff is missing, look at ProcWarpPointer */

    err = dixLookupDevice(&pDev, stuff->deviceid, client, DixWriteAccess);

    if (err != Success)
        return err;

    if (stuff->dst_win != None)
    {
        err = dixLookupWindow(&dest, stuff->dst_win, client, DixReadAccess);
        if (err != Success)
        {
            SendErrorToClient(client, IReqCode, X_WarpDevicePointer,
                    stuff->dst_win, err);
            return Success;
        }
    }

    pSprite = pDev->spriteInfo->sprite;
    x = pSprite->hotPhys.x;
    y = pSprite->hotPhys.y;

    if (stuff->src_win != None)
    {
        int winX, winY;
        WindowPtr src;

        err = dixLookupWindow(&src, stuff->src_win, client, DixReadAccess);
        if (err != Success)
        {
            SendErrorToClient(client, IReqCode, X_WarpDevicePointer,
                    stuff->src_win, err);
            return Success;
        }

        winX = src->drawable.x;
        winY = src->drawable.y;
        if (src->drawable.pScreen != pSprite->hotPhys.pScreen ||
                x < winX + stuff->src_x ||
                y < winY + stuff->src_y ||
                (stuff->src_width != 0 &&
                 winX + stuff->src_x + (int)stuff->src_width < 0) ||
                (stuff->src_height != 0 &&
                 winY + stuff->src_y + (int)stuff->src_height < y) ||
                !PointInWindowIsVisible(src, x, y))
            return Success;
    }

    if (dest)
    {
        x = dest->drawable.x;
        y = dest->drawable.y;
        newScreen = dest->drawable.pScreen;
    } else
        newScreen = pSprite->hotPhys.pScreen;

    x += stuff->dst_x;
    y += stuff->dst_y;

    if (x < 0)
        x = 0;
    else if (x > newScreen->width)
        x = newScreen->width - 1;

    if (y < 0)
        y = 0;
    else if (y > newScreen->height)
        y = newScreen->height - 1;

    if (newScreen == pSprite->hotPhys.pScreen)
    {
        if (x < pSprite->physLimits.x1)
            x = pSprite->physLimits.x1;
        else if (x >= pSprite->physLimits.x2)
            x = pSprite->physLimits.x2 - 1;

        if (y < pSprite->physLimits.y1)
            y = pSprite->physLimits.y1;
        else if (y >= pSprite->physLimits.y2)
            y = pSprite->physLimits.y2 - 1;

#if defined(SHAPE)
        if (pSprite->hotShape)
            ConfineToShape(pDev, pSprite->hotShape, &x, &y);
#endif
        (*newScreen->SetCursorPosition)(pDev, newScreen, x, y, TRUE);
    } else if (!PointerConfinedToScreen(pDev))
    {
        NewCurrentScreen(pDev, newScreen, x, y);
    }

    /* if we don't update the device, we get a jump next time it moves */
    pDev->lastx = x;
    pDev->lasty = x;
    miPointerUpdateSprite(pDev);

    /* FIXME: XWarpPointer is supposed to generate an event. It doesn't do it
       here though. */
    return Success;
}

