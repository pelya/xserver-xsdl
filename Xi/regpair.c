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
 * Request to authenticate as pairing client
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
#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>
#include "extnsionst.h"
#include "exevents.h"
#include "exglobals.h"

#include "regpair.h"

/***********************************************************************
 *
 * This procedure allows a client to register the pairing of a pointer 
 * with a keyboard.
 *
 */

int 
SProcXRegisterPairingClient(ClientPtr client)
{
    char n;
    REQUEST(xRegisterPairingClientReq);
    swaps(&stuff->length, n);
    return ProcXRegisterPairingClient(client);
}

int 
ProcXRegisterPairingClient(ClientPtr client)
{
    xRegisterPairingClientReply rep;

    REQUEST(xRegisterPairingClientReq);
    REQUEST_SIZE_MATCH(xRegisterPairingClientReq);

    if (stuff->disable)
        UnregisterPairingClient(client);

    rep.repType = X_Reply;
    rep.RepType = X_RegisterPairingClient;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.success = stuff->disable || RegisterPairingClient(client);

    WriteReplyToClient(client, sizeof(xRegisterPairingClientReply), &rep);
    return Success;
}

/***********************************************************************
 *
 * This procedure writes the reply for the XRegisterPairingClient function,
 * if the client and server have a different byte ordering.
 *
 */

void
SRepXRegisterPairingClient(ClientPtr client, int size, 
        xRegisterPairingClientReply* rep)
{
    register char n;

    swaps(&rep->sequenceNumber, n);
    swapl(&rep->length, n);
    WriteToClient(client, size, (char *)rep);
}

