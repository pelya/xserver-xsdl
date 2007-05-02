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

#define	 NEED_EVENTS
#define	 NEED_REPLIES

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/extensions/XIproto.h>

#include "dixstruct.h"
#include "windowstr.h"

#include "exglobals.h"
#include "xiselev.h"
#include "geext.h"

int
SProcXiSelectEvent(ClientPtr client)
{
    char n;

    REQUEST(xXiSelectEventReq);
    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xXiSelectEventReq);
    swapl(&stuff->window, n);
    swapl(&stuff->mask, n);
    return (ProcXiSelectEvent(client));
}


int
ProcXiSelectEvent(ClientPtr client)
{
    int ret;
    WindowPtr pWin;
    REQUEST(xXiSelectEventReq);
    REQUEST_SIZE_MATCH(xXiSelectEventReq);

    ret = dixLookupWindow(&pWin, stuff->window, client, DixReadWriteAccess);
    if (ret != Success)
    {
        SendErrorToClient(client, IReqCode, X_XiSelectEvent, 0, ret);
        return Success;
    }

    GEWindowSetMask(client, pWin, IReqCode, stuff->mask);

    return Success;
}

