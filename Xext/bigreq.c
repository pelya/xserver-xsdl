/* $Xorg: bigreq.c,v 1.4 2001/02/09 02:04:32 xorgcvs Exp $ */
/*

Copyright 1992, 1998  The Open Group

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
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/
/* $XFree86: xc/programs/Xserver/Xext/bigreq.c,v 3.5 2001/12/14 19:58:48 dawes Exp $ */

#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "os.h"
#include "dixstruct.h"
#include "extnsionst.h"
#include "bigreqstr.h"

static unsigned char XBigReqCode;

static void BigReqResetProc(
#if NeedFunctionPrototypes
    ExtensionEntry * /* extEntry */
#endif
);

static DISPATCH_PROC(ProcBigReqDispatch);

void
BigReqExtensionInit()
{
    ExtensionEntry *extEntry;

    if ((extEntry = AddExtension(XBigReqExtensionName, 0, 0,
				 ProcBigReqDispatch, ProcBigReqDispatch,
				 BigReqResetProc, StandardMinorOpcode)) != 0)
	XBigReqCode = (unsigned char)extEntry->base;
    DeclareExtensionSecurity(XBigReqExtensionName, TRUE);
}

/*ARGSUSED*/
static void
BigReqResetProc (extEntry)
    ExtensionEntry	*extEntry;
{
}

static int
ProcBigReqDispatch (client)
    register ClientPtr	client;
{
    REQUEST(xBigReqEnableReq);
    xBigReqEnableReply rep;
    register int n;

    if (client->swapped) {
	swaps(&stuff->length, n);
    }
    if (stuff->brReqType != X_BigReqEnable)
	return BadRequest;
    REQUEST_SIZE_MATCH(xBigReqEnableReq);
    client->big_requests = TRUE;
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.max_request_size = MAX_BIG_REQUEST_SIZE;
    if (client->swapped) {
    	swaps(&rep.sequenceNumber, n);
	swapl(&rep.max_request_size, n);
    }
    WriteToClient(client, sizeof(xBigReqEnableReply), (char *)&rep);
    return(client->noClientException);
}
