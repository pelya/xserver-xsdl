/************************************************************

Copyright 2003 Sun Microsystems, Inc.

All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, and/or sell copies of the Software, and to permit persons
to whom the Software is furnished to do so, provided that the above
copyright notice(s) and this permission notice appear in all copies of
the Software and that both the above copyright notice(s) and this
permission notice appear in supporting documentation.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
HOLDERS INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL
INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING
FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

Except as contained in this notice, the name of a copyright holder
shall not be used in advertising or otherwise to promote the sale, use
or other dealings in this Software without prior written authorization
of the copyright holder.

************************************************************/

#define NEED_REPLIES
#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "dixstruct.h"
#include "extnsionst.h"
#include "colormapst.h"
#include "scrnintstr.h"
#include "servermd.h"
#define  _XEVIE_SERVER_
#include "Xeviestr.h"
#include "Xfuncproto.h"
#include "input.h"

#include "../os/osdep.h"

#define NoSuchEvent 0x80000000

extern Bool noXkbExtension;
extern int    xeviegrabState;

static int		ProcDispatch (), SProcDispatch ();
static void		ResetProc ();

static unsigned char	ReqCode = 0;
static int		ErrorBase;

int			xevieFlag = 0;
int	 		xevieClientIndex = 0;
DeviceIntPtr		xeviekb = NULL;
DeviceIntPtr		xeviemouse = NULL;
Mask			xevieMask = 0;
int       		xevieEventSent = 0;
int			xevieKBEventSent = 0;
Mask xevieFilters[128] = 
{
        NoSuchEvent,                   /* 0 */
        NoSuchEvent,                   /* 1 */
        KeyPressMask,                  /* KeyPress */
        KeyReleaseMask,                /* KeyRelease */
        ButtonPressMask,               /* ButtonPress */
        ButtonReleaseMask,             /* ButtonRelease */
        PointerMotionMask              /* MotionNotify (initial state) */
};

static void XevieEnd(int clientIndex);
static void XevieClientStateCallback(CallbackListPtr *pcbl, pointer nulldata,
                                   pointer calldata);
static void XevieServerGrabStateCallback(CallbackListPtr *pcbl,
                                   pointer nulldata,
                                   pointer calldata);

void
XevieExtensionInit ()
{
    ExtensionEntry* extEntry;

    if (!AddCallback(&ServerGrabCallback,XevieServerGrabStateCallback,NULL))
       return;

    if (extEntry = AddExtension (XEVIENAME,
				0,
				XevieNumberErrors,
				ProcDispatch,
				SProcDispatch,
				ResetProc,
				StandardMinorOpcode)) {
	ReqCode = (unsigned char)extEntry->base;
	ErrorBase = extEntry->errorBase;
    }

    /* PC servers initialize the desktop colors (citems) here! */
}

/*ARGSUSED*/
static 
void ResetProc (extEntry)
    ExtensionEntry* extEntry;
{
}

static 
int ProcQueryVersion (client)
    register ClientPtr client;
{
    REQUEST (xXevieQueryVersionReq);
    xXevieQueryVersionReply rep;
    register int n;

    REQUEST_SIZE_MATCH (xXevieQueryVersionReq);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequence_number = client->sequence;
    rep.server_major_version = XEVIE_MAJOR_VERSION;
    rep.server_minor_version = XEVIE_MINOR_VERSION;
    WriteToClient (client, sizeof (xXevieQueryVersionReply), (char *)&rep);
    return client->noClientException;
}

static
int ProcStart (client)
    register ClientPtr client;
{
    REQUEST (xXevieStartReq);
    xXevieStartReply rep;
    register int n;

    REQUEST_SIZE_MATCH (xXevieStartReq);
    rep.pad1 = 0;

    if(!xevieFlag){
        if (AddCallback(&ClientStateCallback,XevieClientStateCallback,NULL)) {
           xevieFlag = 1;
           rep.pad1 = 1;
           xevieClientIndex = client->index;
        } else
           return BadAlloc;
    } else
        return BadAccess;

    rep.type = X_Reply;
    rep.sequence_number = client->sequence;
    WriteToClient (client, sizeof (xXevieStartReply), (char *)&rep);
    return client->noClientException;
}

static
int ProcEnd (client)
    register ClientPtr client;
{
    xXevieEndReply rep;

    XevieEnd(xevieClientIndex);

    rep.type = X_Reply;
    rep.sequence_number = client->sequence;
    WriteToClient (client, sizeof (xXevieEndReply), (char *)&rep);
    return client->noClientException;
}

static
int ProcSend (client)
    register ClientPtr client;
{
    REQUEST (xXevieSendReq);
    xXevieSendReply rep;
    xEvent *xE;
    OsCommPtr oc;
    static unsigned char lastDetail = 0, lastType = 0;

    xE = (xEvent *)&stuff->event;
    rep.type = X_Reply;
    rep.sequence_number = client->sequence;
    WriteToClient (client, sizeof (xXevieSendReply), (char *)&rep);

    switch(xE->u.u.type) {
	case KeyPress:
        case KeyRelease:
	  xevieKBEventSent = 1;
#ifdef XKB
          if(noXkbExtension)
#endif
            CoreProcessKeyboardEvent (xE, xeviekb, 1);
	  break;
	case ButtonPress:
	case ButtonRelease:
	case MotionNotify:
	  xevieEventSent = 1;
	  CoreProcessPointerEvent(xE, xeviemouse, 1); 
	  break; 
	default:
	  break;
    }
    lastType = xE->u.u.type;
    lastDetail = xE->u.u.detail;
    return client->noClientException;
}

static
int ProcSelectInput (client)
    register ClientPtr client;
{
    REQUEST (xXevieSelectInputReq);
    xXevieSelectInputReply rep;

    xevieMask = (long)stuff->event_mask;
    rep.type = X_Reply;
    rep.sequence_number = client->sequence;
    WriteToClient (client, sizeof (xXevieSelectInputReply), (char *)&rep);
    return client->noClientException;
}

static 
int ProcDispatch (client)
    register ClientPtr	client;
{
    REQUEST (xReq);
    switch (stuff->data)
    {
    case X_XevieQueryVersion:
	return ProcQueryVersion (client);
    case X_XevieStart:
	return ProcStart (client);
    case X_XevieEnd:
	return ProcEnd (client);
    case X_XevieSend:
	return ProcSend (client);
    case X_XevieSelectInput:
	return ProcSelectInput(client);
    default:
	return BadRequest;
    }
}

static 
int SProcQueryVersion (client)
    register ClientPtr	client;
{
    register int n;

    REQUEST(xXevieQueryVersionReq);
    swaps(&stuff->length, n);
    return ProcQueryVersion(client);
}

static 
int SProcStart (client)
    ClientPtr client;
{
    register int n;

    REQUEST (xXevieStartReq);
    swaps (&stuff->length, n);
    swapl (&stuff->screen, n);
    REQUEST_AT_LEAST_SIZE (xXevieStartReq);
    return ProcStart (client);
}

static 
int SProcEnd (client)
    ClientPtr client;
{
    register int n;
    int count;
    xColorItem* pItem;

    REQUEST (xXevieEndReq);
    swaps (&stuff->length, n);
    REQUEST_AT_LEAST_SIZE (xXevieEndReq);
    swapl(&stuff->cmap, n);
    return ProcEnd (client);
}

static
int SProcSend (client)
    ClientPtr client;
{
    register int n;
    int count;

    REQUEST (xXevieSendReq);
    swaps (&stuff->length, n);
    REQUEST_AT_LEAST_SIZE (xXevieSendReq);
    swapl(&stuff->event, n);
    return ProcSend (client);
}

static
int SProcSelectInput (client)
    ClientPtr client;
{
    register int n;
    int count;

    REQUEST (xXevieSelectInputReq);
    swaps (&stuff->length, n);
    REQUEST_AT_LEAST_SIZE (xXevieSendReq);
    swapl(&stuff->event_mask, n);
    return ProcSelectInput (client);
}


static 
int SProcDispatch (client)
    register ClientPtr	client;
{
    REQUEST(xReq);
    switch (stuff->data)
    {
    case X_XevieQueryVersion:
	return SProcQueryVersion (client);
    case X_XevieStart:
	return SProcStart (client);
    case X_XevieEnd:
	return SProcEnd (client);
    case X_XevieSend:
	return SProcSend (client);
    case X_XevieSelectInput:
	return SProcSelectInput(client);
    default:
	return BadRequest;
    }
}

static void
XevieEnd(int clientIndex)
{
    if (!clientIndex || clientIndex == xevieClientIndex) {
       xevieFlag = 0;
       xevieClientIndex = 0;
       DeleteCallback (&ClientStateCallback, XevieClientStateCallback, NULL);
    }
}

static void
XevieClientStateCallback(CallbackListPtr *pcbl, pointer nulldata,
                        pointer calldata)
{
    NewClientInfoRec *pci = (NewClientInfoRec *)calldata;
    ClientPtr client = pci->client;
    if (client->clientState == ClientStateGone
       || client->clientState == ClientStateRetained)
       XevieEnd(client->index);
}

static void
XevieServerGrabStateCallback(CallbackListPtr *pcbl, pointer nulldata,
                            pointer calldata)
{
    ServerGrabInfoRec *grbinfo = (ServerGrabInfoRec *)calldata;
    if (grbinfo->grabstate == SERVER_GRABBED)
       xeviegrabState = TRUE;
    else
       xeviegrabState = FALSE;
}


