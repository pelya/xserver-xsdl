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
#include "opaque.h"
#define  _XEVIE_SERVER_
#include "Xeviestr.h"
#include "Xfuncproto.h"
#include "input.h"
#include "inputstr.h"
#include "windowstr.h"
#include "cursorstr.h"
#include "swaprep.h"
#include "panoramiX.h"
#include "panoramiXsrv.h"

#ifndef EXTMODULE
#include "../os/osdep.h"
#else
#include "xf86_ansic.h"
#endif

#if 0
#define DEBUG
#endif

#ifdef DEBUG
# define ERR(x) ErrorF(x)
#else
#define ERR
#endif

static int			xevieFlag = 0;
static int	 		xevieClientIndex = 0;
static Mask			xevieMask = 0;
static Bool			xeviegrabState = FALSE;
static unsigned int		xevieServerGeneration;
static int			xevieDevicePrivateIndex;
static Bool			xevieModifiersOn = FALSE;

#define XEVIEINFO(dev)  ((xevieDeviceInfoPtr)dev->devPrivates[xevieDevicePrivateIndex].ptr)
#define NoSuchEvent 0x80000000

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
#undef NoSuchEvent

typedef struct {
    ProcessInputProc processInputProc;
    ProcessInputProc realInputProc;
    DeviceUnwrapProc unwrapProc;
} xevieDeviceInfoRec, *xevieDeviceInfoPtr;

static int		ProcDispatch (ClientPtr), SProcDispatch (ClientPtr);
static void		ResetProc (ExtensionEntry*);

static unsigned char	ReqCode = 0;
static int		ErrorBase;

static Bool XevieStart(void);
static void XevieEnd(int clientIndex);
static void XevieClientStateCallback(CallbackListPtr *pcbl, pointer nulldata,
				    pointer calldata);
static void XevieServerGrabStateCallback(CallbackListPtr *pcbl,
					 pointer nulldata,
					 pointer calldata);

static Bool XevieAdd(DeviceIntPtr device, pointer data);
static void XevieWrap(DeviceIntPtr device, ProcessInputProc proc);
static Bool XevieRemove(DeviceIntPtr device, pointer data);
static void doSendEvent(xEvent *xE, DeviceIntPtr device);
static void XeviePointerProcessInputProc(xEvent *xE, DeviceIntPtr dev,
					 int count);
static void XevieKbdProcessInputProc(xEvent *xE, DeviceIntPtr dev, int count);

void
XevieExtensionInit (INITARGS)
{
    ExtensionEntry* extEntry;

    if (serverGeneration != xevieServerGeneration) {
	if ((xevieDevicePrivateIndex = AllocateDevicePrivateIndex()) == -1)
	    return;
	xevieServerGeneration = serverGeneration;
    }

    if (!AddCallback(&ServerGrabCallback,XevieServerGrabStateCallback,NULL))
	return;

    if ((extEntry = AddExtension (XEVIENAME,
				0,
				XevieNumberErrors,
				ProcDispatch,
				SProcDispatch,
				ResetProc,
				StandardMinorOpcode))) {
	ReqCode = (unsigned char)extEntry->base;
	ErrorBase = extEntry->errorBase;
    }
}

/*ARGSUSED*/
static 
void ResetProc (ExtensionEntry* extEntry)
{
}

static 
int ProcQueryVersion (ClientPtr client)
{
    xXevieQueryVersionReply rep;

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
int ProcStart (ClientPtr client)
{
    xXevieStartReply rep;

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
    
    if (!XevieStart()) {
	DeleteCallback(&ClientStateCallback,XevieClientStateCallback,NULL);
	return BadAlloc;
    }

    xevieModifiersOn = FALSE;
    
    rep.type = X_Reply;
    rep.sequence_number = client->sequence;
    WriteToClient (client, sizeof (xXevieStartReply), (char *)&rep);
    return client->noClientException;
}

static
int ProcEnd (ClientPtr client)
{
    xXevieEndReply rep;

    if (xevieFlag) {
	if (client->index != xevieClientIndex)
	    return BadAccess;

	DeleteCallback(&ClientStateCallback,XevieClientStateCallback,NULL);
	XevieEnd(xevieClientIndex);
    }
    
    rep.type = X_Reply;
    rep.sequence_number = client->sequence;
    WriteToClient (client, sizeof (xXevieEndReply), (char *)&rep);
    return client->noClientException;
}

static
int ProcSend (ClientPtr client)
{
    REQUEST (xXevieSendReq);
    xXevieSendReply rep;
    xEvent *xE;
    static unsigned char lastDetail = 0, lastType = 0;

    ERR("ProcSend\n");
    
    if (client->index != xevieClientIndex)
	return BadAccess;

    xE = (xEvent *)&stuff->event;
    rep.type = X_Reply;
    rep.sequence_number = client->sequence;
    WriteToClient (client, sizeof (xXevieSendReply), (char *)&rep);

    switch(xE->u.u.type) {
	case KeyPress:
        case KeyRelease:
	    doSendEvent(xE, inputInfo.keyboard);
	  break;
	case ButtonPress:
	case ButtonRelease:
	case MotionNotify:
	    doSendEvent(xE, inputInfo.pointer);
	  break; 
	default:
	  break;
    }
    lastType = xE->u.u.type;
    lastDetail = xE->u.u.detail;

    return client->noClientException;
}

static
int ProcSelectInput (ClientPtr client)
{
    REQUEST (xXevieSelectInputReq);
    xXevieSelectInputReply rep;

    if (client->index != xevieClientIndex)
	return BadAccess;

    xevieMask = (long)stuff->event_mask;
    rep.type = X_Reply;
    rep.sequence_number = client->sequence;
    WriteToClient (client, sizeof (xXevieSelectInputReply), (char *)&rep);
    return client->noClientException;
}

static 
int ProcDispatch (ClientPtr client)
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
int SProcQueryVersion (ClientPtr client)
{
    register int n;

    REQUEST(xXevieQueryVersionReq);
    swaps(&stuff->length, n);
    return ProcQueryVersion(client);
}

static 
int SProcStart (ClientPtr client)
{
    register int n;

    REQUEST (xXevieStartReq);
    swaps (&stuff->length, n);
    swapl (&stuff->screen, n);
    REQUEST_AT_LEAST_SIZE (xXevieStartReq);
    return ProcStart (client);
}

static 
int SProcEnd (ClientPtr client)
{
    register int n;

    REQUEST (xXevieEndReq);
    swaps (&stuff->length, n);
    REQUEST_AT_LEAST_SIZE (xXevieEndReq);
    swapl(&stuff->cmap, n);
    return ProcEnd (client);
}

static
int SProcSend (ClientPtr client)
{
    register int n;

    REQUEST (xXevieSendReq);
    swaps (&stuff->length, n);
    REQUEST_AT_LEAST_SIZE (xXevieSendReq);
    swapl(&stuff->event, n);
    return ProcSend (client);
}

static
int SProcSelectInput (ClientPtr client)
{
    register int n;

    REQUEST (xXevieSelectInputReq);
    swaps (&stuff->length, n);
    REQUEST_AT_LEAST_SIZE (xXevieSendReq);
    swapl(&stuff->event_mask, n);
    return ProcSelectInput (client);
}


static 
int SProcDispatch (ClientPtr client)
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

/*=====================================================*/


#define WRAP_INPUTPROC(dev,store,inputProc) \
   store->processInputProc = dev->public.processInputProc; \
   dev->public.processInputProc = inputProc; \
   store->realInputProc = dev->public.realInputProc; \
   dev->public.realInputProc = inputProc;

#define COND_WRAP_INPUTPROC(dev,store,inputProc) \
   if (dev->public.processInputProc == dev->public.realInputProc) \
          dev->public.processInputProc = inputProc; \
   store->processInputProc =  \
   store->realInputProc = dev->public.realInputProc; \
   dev->public.realInputProc = inputProc;
   
#define UNWRAP_INPUTPROC(dev,restore) \
   dev->public.processInputProc = restore->processInputProc; \
   dev->public.realInputProc = restore->realInputProc;

#define UNWRAP_INPUTPROC(dev,restore) \
   dev->public.processInputProc = restore->processInputProc; \
   dev->public.realInputProc = restore->realInputProc;

#define XEVIE_EVENT(xE) \
      (xevieFlag \
       && !xeviegrabState \
       && clients[xevieClientIndex] \
       && (xevieMask & xevieFilters[xE->u.u.type]))


static void
sendEvent(ClientPtr pClient, xEvent *xE)
{
    if(pClient->swapped) {
	xEvent    eventTo;
	
	/* Remember to strip off the leading bit of type in case
	   this event was sent with "SendEvent." */
	(*EventSwapVector[xE->u.u.type & 0177]) (xE, &eventTo);
	(void)WriteToClient(pClient, sizeof(xEvent), (char *)&eventTo);
    } else {
	(void)WriteToClient(pClient, sizeof(xEvent), (char *) xE);
    }
}

static void
XevieKbdProcessInputProc(xEvent *xE, DeviceIntPtr dev, int count)
{
    int             key, bit;
    BYTE   *kptr;
    ProcessInputProc tmp;
    KeyClassPtr keyc = dev->key;
    xevieDeviceInfoPtr xeviep = XEVIEINFO(dev);

    if(XEVIE_EVENT(xE)) {
	ERR("XevieKbdProcessInputProc\n");
    
	key = xE->u.u.detail;
	kptr = &keyc->down[key >> 3];
	bit = 1 << (key & 7);

	/*
	 * This is a horrible hack: with xkb on we must zero the modifiers
	 * before sending an event sent  back by xevie to
	 * CoreProcessKeyboardEvent.
	 * Since we cannot probe for xkb directly we need to check if the
	 * modifers are set at this point. If they are we know that xkb
	 * isn't active.
	 */
	if (dev->key->modifierMap[xE->u.u.detail])
	    xevieModifiersOn = TRUE;
	
	xE->u.keyButtonPointer.event = xeviewin->drawable.id;
	xE->u.keyButtonPointer.root = GetCurrentRootWindow()->drawable.id;
	xE->u.keyButtonPointer.child = (xeviewin->firstChild)
	    ? xeviewin->firstChild->drawable.id:0;
	xE->u.keyButtonPointer.rootX = xeviehot.x;
	xE->u.keyButtonPointer.rootY = xeviehot.y;
	xE->u.keyButtonPointer.state = keyc->state;
	/* fix bug: sequence lost in Xlib */
	xE->u.u.sequenceNumber = clients[xevieClientIndex]->sequence;
	sendEvent(clients[xevieClientIndex], xE);
	return;
    }
    
    tmp = dev->public.realInputProc;
    UNWRAP_INPUTPROC(dev,xeviep);
    dev->public.processInputProc(xE,dev,count);
    COND_WRAP_INPUTPROC(dev,xeviep,tmp);
}

static void
XeviePointerProcessInputProc(xEvent *xE, DeviceIntPtr dev, int count)
{
    xevieDeviceInfoPtr xeviep = XEVIEINFO(dev);
    ProcessInputProc tmp;

    if (XEVIE_EVENT(xE)) {
	ERR("XeviePointerProcessInputProc\n");
	/* fix bug: sequence lost in Xlib */
	xE->u.u.sequenceNumber = clients[xevieClientIndex]->sequence;
	sendEvent(clients[xevieClientIndex], xE);
	return;
    }

    tmp = dev->public.realInputProc;
    UNWRAP_INPUTPROC(dev,xeviep);
    dev->public.processInputProc(xE,dev,count);
   COND_WRAP_INPUTPROC(dev,xeviep,tmp);
}

static Bool
XevieStart(void)
{
    ProcessInputProc prp;
    prp = XevieKbdProcessInputProc;
    if (!XevieAdd(inputInfo.keyboard,&prp))
	return FALSE;
    prp = XeviePointerProcessInputProc;
    if (!XevieAdd(inputInfo.pointer,&prp))
	return FALSE;

    return TRUE;
}

static void
XevieEnd(int clientIndex)
{
    if (!clientIndex || clientIndex == xevieClientIndex) {
	XevieRemove(inputInfo.keyboard,NULL);
	XevieRemove(inputInfo.pointer,NULL);
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

#define UNWRAP_UNWRAPPROC(device,proc_store) \
    device->unwrapProc = proc_store;

#define WRAP_UNWRAPPROC(device,proc_store,proc) \
    proc_store = device->unwrapProc; \
    device->unwrapProc = proc;

static void
xevieUnwrapProc(DeviceIntPtr device, DeviceHandleProc proc, pointer data)
{
    xevieDeviceInfoPtr xeviep = XEVIEINFO(device);
    ProcessInputProc tmp = device->public.processInputProc;
    
    UNWRAP_INPUTPROC(device,xeviep);
    UNWRAP_UNWRAPPROC(device,xeviep->unwrapProc);
    proc(device,data);
    WRAP_INPUTPROC(device,xeviep,tmp);
    WRAP_UNWRAPPROC(device,xeviep->unwrapProc,xevieUnwrapProc);
}

static Bool
XevieUnwrapAdd(DeviceIntPtr device, void* data)
{
    if (device->unwrapProc)
	device->unwrapProc(device,XevieUnwrapAdd,data);
    else {
	ProcessInputProc *ptr = data;
	XevieWrap(device,*ptr);
    }

    return TRUE;
}

static Bool
XevieAdd(DeviceIntPtr device, void* data)
{
    xevieDeviceInfoPtr xeviep;
        
    if (!AllocateDevicePrivate(device, xevieDevicePrivateIndex))
	return FALSE;
	
    xeviep = xcalloc (sizeof (xevieDeviceInfoRec),1);
    if (!xeviep)
	    return FALSE;

    device->devPrivates[xevieDevicePrivateIndex].ptr = xeviep;
    XevieUnwrapAdd(device, data);

    return TRUE;
}

static Bool
XevieRemove(DeviceIntPtr device,pointer data)
{
    xevieDeviceInfoPtr xeviep = XEVIEINFO(device);
	
    if (!xeviep) return TRUE;
    
    UNWRAP_INPUTPROC(device,xeviep);
    UNWRAP_UNWRAPPROC(device,xeviep->unwrapProc);

    xfree(xeviep);
    device->devPrivates[xevieDevicePrivateIndex].ptr = NULL;
    return TRUE;
}

static void
XevieWrap(DeviceIntPtr device, ProcessInputProc proc)
{
    xevieDeviceInfoPtr xeviep = XEVIEINFO(device);

    WRAP_INPUTPROC(device,xeviep,proc);
    WRAP_UNWRAPPROC(device,xeviep->unwrapProc,xevieUnwrapProc);
}

static void
doSendEvent(xEvent *xE, DeviceIntPtr dev)
{
    xevieDeviceInfoPtr xeviep = XEVIEINFO(dev);
    ProcessInputProc tmp = dev->public.realInputProc;
    if (((xE->u.u.type==KeyPress)||(xE->u.u.type==KeyRelease))
	&& !xevieModifiersOn) {
	CARD8 realModes = dev->key->modifierMap[xE->u.u.detail];
	dev->key->modifierMap[xE->u.u.detail] = 0;
	
	UNWRAP_INPUTPROC(dev,xeviep);
	dev->public.processInputProc(xE,dev,1);
	COND_WRAP_INPUTPROC(dev,xeviep,tmp);
	dev->key->modifierMap[xE->u.u.detail] = realModes;
    } else {
	UNWRAP_INPUTPROC(dev,xeviep);
	dev->public.processInputProc(xE,dev,1);
	COND_WRAP_INPUTPROC(dev,xeviep,tmp);
    }
}
