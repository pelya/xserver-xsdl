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


#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif
#include "windowstr.h"
#include <X11/extensions/ge.h>

#include "geint.h"
#include "geext.h"

int GEEventBase;
int GEErrorBase;
int GEClientPrivateIndex;
int GEEventType; /* The opcode for all GenericEvents will have. */


GEExtension GEExtensions[MAXEXTENSIONS];

/* Major available requests */
static const int version_requests[] = {
    X_GEQueryVersion,	/* before client sends QueryVersion */
    X_GEQueryVersion,	/* must be set to last request in version 1 */
};

/* Forward declarations */
static void SGEGenericEvent(xEvent* from, xEvent* to);

#define NUM_VERSION_REQUESTS	(sizeof (version_requests) / sizeof (version_requests[0]))

/************************************************************/
/*                request handlers                          */
/************************************************************/

static int ProcGEQueryVersion(ClientPtr client)
{
    int n;
    GEClientInfoPtr pGEClient = GEGetClient(client);
    xGEQueryVersionReply rep;
    REQUEST(xGEQueryVersionReq);

    REQUEST_SIZE_MATCH(xGEQueryVersionReq);

    rep.repType = X_Reply;
    rep.RepType = X_GEQueryVersion;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;

    if (stuff->majorVersion < GE_MAJOR) {
        rep.majorVersion = stuff->majorVersion;
        rep.minorVersion = stuff->minorVersion;
    } else {
        rep.majorVersion = GE_MAJOR;
        if (stuff->majorVersion == GE_MAJOR && 
                stuff->minorVersion < GE_MINOR)
            rep.minorVersion = stuff->minorVersion;
        else
            rep.minorVersion = GE_MINOR;
    }

    pGEClient->major_version = rep.majorVersion;
    pGEClient->minor_version = rep.minorVersion;

    if (client->swapped)
    {
    	swaps(&rep.sequenceNumber, n);
    	swapl(&rep.length, n);
        swaps(&rep.majorVersion, n);
        swaps(&rep.minorVersion, n);
    }


    WriteToClient(client, sizeof(xGEQueryVersionReply), (char*)&rep);
    return(client->noClientException);
}

int (*ProcGEVector[GENumberRequests])(ClientPtr) = {
    /* Version 1.0 */
    ProcGEQueryVersion
};

/************************************************************/
/*                swapped request handlers                  */
/************************************************************/
static int
SProcGEQueryVersion(ClientPtr client)
{
    int n;
    REQUEST(xGEQueryVersionReq);

    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xGEQueryVersionReq);
    swaps(&stuff->majorVersion, n);
    swaps(&stuff->minorVersion, n);
    return(*ProcGEVector[stuff->ReqType])(client);
}

int (*SProcGEVector[GENumberRequests])(ClientPtr) = {
    /* Version 1.0 */
    SProcGEQueryVersion
};


/************************************************************/
/*                callbacks                                 */
/************************************************************/

/* dispatch requests */
static int 
ProcGEDispatch(ClientPtr client)
{
    GEClientInfoPtr pGEClient = GEGetClient(client);
    REQUEST(xGEReq);

    if (pGEClient->major_version >= NUM_VERSION_REQUESTS)
        return BadRequest;
    if (stuff->ReqType > version_requests[pGEClient->major_version])
        return BadRequest;

    return (ProcGEVector[stuff->ReqType])(client);
}

/* dispatch swapped requests */
static int
SProcGEDispatch(ClientPtr client)
{
    REQUEST(xGEReq);
    if (stuff->ReqType >= GENumberRequests)
        return BadRequest;
    return (*SProcGEVector[stuff->ReqType])(client);
}

/* new client callback */
static void GEClientCallback(CallbackListPtr *list,
        pointer closure,
        pointer data)
{
    NewClientInfoRec	*clientinfo = (NewClientInfoRec *) data;
    ClientPtr		pClient = clientinfo->client;
    GEClientInfoPtr     pGEClient = GEGetClient(pClient);

    pGEClient->major_version = 0;
    pGEClient->minor_version = 0;
}

/* reset extension */
static void
GEResetProc(ExtensionEntry *extEntry)
{
    DeleteCallback(&ClientStateCallback, GEClientCallback, 0);
    EventSwapVector[GenericEvent] = NotImplemented;

    GEEventBase = 0;
    GEErrorBase = 0;
    GEEventType = 0;
}

/*  Calls the registered event swap function for the extension. */
static void 
SGEGenericEvent(xEvent* from, xEvent* to)
{
    xGenericEvent* gefrom = (xGenericEvent*)from;
    xGenericEvent* geto = (xGenericEvent*)to;

    if (gefrom->extension > MAXEXTENSIONS)
    {
        ErrorF("GE: Invalid extension offset for event.\n");
        return;
    }

    if (GEExtensions[gefrom->extension & 0x7F].evswap)
        GEExtensions[gefrom->extension & 0x7F].evswap(gefrom, geto);
}

/* init extension, register at server */
void 
GEExtensionInit(void)
{
    ExtensionEntry *extEntry;

    GEClientPrivateIndex = AllocateClientPrivateIndex(); 
    if (!AllocateClientPrivate(GEClientPrivateIndex, 
                               sizeof(GEClientRec)))
    {
        FatalError("GEExtensionInit: Alloc client private failed.\n");
    }

    if(!AddCallback(&ClientStateCallback, GEClientCallback, 0))
    {
        FatalError("GEExtensionInit: register client callback failed.\n");
    }

    if((extEntry = AddExtension(GE_NAME, 
                        GENumberEvents, GENumberErrors, 
                        ProcGEDispatch, SProcGEDispatch, 
                        GEResetProc, StandardMinorOpcode)) != 0)
    {
        GEEventBase = extEntry->eventBase;
        GEErrorBase = extEntry->errorBase;
        GEEventType = GEEventBase;

        memset(GEExtensions, 0, sizeof(GEExtensions));

        EventSwapVector[X_GenericEvent] = (EventSwapPtr) SGEGenericEvent;
    } else {
        FatalError("GEInit: AddExtensions failed.\n");
    }

}

/************************************************************/
/*                interface for extensions                  */
/************************************************************/

/* Register an extension with GE. The given swap function will be called each
 * time an event is sent to a client with different byte order.
 * @param extension The extensions major opcode 
 * @param ev_swap The event swap function.  
 * @param ev_fill Called for an event before delivery. The extension now has
 * the chance to fill in necessary fields for the event.
 */
void GERegisterExtension(
        int extension, 
        void (*ev_swap)(xGenericEvent* from, xGenericEvent* to),
        void (*ev_fill)(xGenericEvent* ev, DeviceIntPtr pDev, 
                        WindowPtr pWin, GrabPtr pGrab)
        )
{
    if ((extension & 0x7F) >=  MAXEXTENSIONS)
        FatalError("GE: extension > MAXEXTENSIONS. This should not happen.\n");

    /* extension opcodes are > 128, might as well save some space here */
    GEExtensions[extension & 0x7f].evswap = ev_swap;
    GEExtensions[extension & 0x7f].evfill = ev_fill;
}


/* Sets type and extension field for a generic event. This is just an
 * auxiliary function, extensions could do it manually too. 
 */ 
void GEInitEvent(xGenericEvent* ev, int extension)
{
    ev->type = GenericEvent;
    ev->extension = extension;
    ev->length = 0;
}

/* Recalculates the summary mask for the window. */
static void 
GERecalculateWinMask(WindowPtr pWin)
{
    int i;
    GEClientPtr it;
    GEEventMasksPtr evmasks;

    if (!pWin->optional)
        return;

    evmasks = pWin->optional->geMasks;

    for (i = 0; i < MAXEXTENSIONS; i++)
    {
        evmasks->eventMasks[i] = 0;
    }

    it = pWin->optional->geMasks->geClients;
    while(it)
    {
        for (i = 0; i < MAXEXTENSIONS; i++)
        {
            evmasks->eventMasks[i] |= it->eventMask[i];
        }
        it = it->next;
    }
}

/* Set generic event mask for given window. */
void GEWindowSetMask(ClientPtr pClient, WindowPtr pWin, int extension, Mask mask)
{
    GEClientPtr cli;

    extension = (extension & 0x7F);

    if (extension > MAXEXTENSIONS)
    {
        ErrorF("Invalid extension number.\n");
        return;
    }

    if (!pWin->optional && !MakeWindowOptional(pWin))
    {
        ErrorF("GE: Could not make window optional.\n");
        return;
    }

    if (mask)
    {
        GEEventMasksPtr evmasks = pWin->optional->geMasks;

        /* check for existing client */
        cli = evmasks->geClients;
        while(cli)
        {
            if (cli->client == pClient)
                break;
            cli = cli->next;
        }
        if (!cli)
        {
            /* new client */
            cli  = (GEClientPtr)xcalloc(1, sizeof(GEClientRec));
            if (!cli)
            {
                ErrorF("GE: Insufficient memory to alloc client.\n");
                return;
            }
            cli->next = evmasks->geClients;
            cli->client = pClient;
            evmasks->geClients = cli;
        }
        cli->eventMask[extension] = mask;
    } else
    {
        /* remove client. */
        cli = pWin->optional->geMasks->geClients;
        if (cli->client == pClient)
        {
            pWin->optional->geMasks->geClients = cli->next;
            xfree(cli);
        } else 
        { 
            GEClientPtr prev = cli;
            cli = cli->next;

            while(cli)
            {
                if (cli->client == pClient)
                {
                    prev->next = cli->next;
                    xfree(cli);
                    break;
                }
                prev = cli;
                cli = cli->next;
            }
        }
        if (!cli)
            return;
    }

    GERecalculateWinMask(pWin);
}


