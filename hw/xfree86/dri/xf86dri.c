/* $XFree86: xc/programs/Xserver/GL/dri/xf86dri.c,v 1.12 2002/12/14 01:36:08 dawes Exp $ */
/**************************************************************************

Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
Copyright 2000 VA Linux Systems, Inc.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Kevin E. Martin <martin@valinux.com>
 *   Jens Owen <jens@tungstengraphics.com>
 *   Rickard E. (Rik) Faith <faith@valinux.com>
 *
 */

#ifdef XFree86LOADER
#include "xf86.h"
#include "xf86_ansic.h"
#endif

#define NEED_REPLIES
#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "dixstruct.h"
#include "extnsionst.h"
#include "colormapst.h"
#include "cursorstr.h"
#include "scrnintstr.h"
#include "servermd.h"
#define _XF86DRI_SERVER_
#include "xf86dristr.h"
#include "swaprep.h"
#include "dri.h"
#include "sarea.h"
#include "dristruct.h"
#include "xf86.h"
#include "xf86drm.h"

static int DRIErrorBase;

static DISPATCH_PROC(ProcXF86DRIQueryVersion);
static DISPATCH_PROC(ProcXF86DRIQueryDirectRenderingCapable);
static DISPATCH_PROC(ProcXF86DRIOpenConnection);
static DISPATCH_PROC(ProcXF86DRICloseConnection);
static DISPATCH_PROC(ProcXF86DRIGetClientDriverName);
static DISPATCH_PROC(ProcXF86DRICreateContext);
static DISPATCH_PROC(ProcXF86DRIDestroyContext);
static DISPATCH_PROC(ProcXF86DRICreateDrawable);
static DISPATCH_PROC(ProcXF86DRIDestroyDrawable);
static DISPATCH_PROC(ProcXF86DRIGetDrawableInfo);
static DISPATCH_PROC(ProcXF86DRIGetDeviceInfo);
static DISPATCH_PROC(ProcXF86DRIDispatch);
static DISPATCH_PROC(ProcXF86DRIAuthConnection);
static DISPATCH_PROC(ProcXF86DRIOpenFullScreen);
static DISPATCH_PROC(ProcXF86DRICloseFullScreen);

static DISPATCH_PROC(SProcXF86DRIQueryVersion);
static DISPATCH_PROC(SProcXF86DRIDispatch);

static void XF86DRIResetProc(ExtensionEntry* extEntry);

static unsigned char DRIReqCode = 0;

extern void XFree86DRIExtensionInit(void);

void
XFree86DRIExtensionInit(void)
{
    ExtensionEntry* extEntry;

#ifdef XF86DRI_EVENTS
    EventType = CreateNewResourceType(XF86DRIFreeEvents);
#endif

    if (
	DRIExtensionInit() &&
#ifdef XF86DRI_EVENTS
        EventType && ScreenPrivateIndex != -1 &&
#endif
	(extEntry = AddExtension(XF86DRINAME,
				 XF86DRINumberEvents,
				 XF86DRINumberErrors,
				 ProcXF86DRIDispatch,
				 SProcXF86DRIDispatch,
				 XF86DRIResetProc,
				 StandardMinorOpcode))) {
	DRIReqCode = (unsigned char)extEntry->base;
	DRIErrorBase = extEntry->errorBase;
    }
}

/*ARGSUSED*/
static void
XF86DRIResetProc (
    ExtensionEntry* extEntry
)
{
    DRIReset();
}

static int
ProcXF86DRIQueryVersion(
    register ClientPtr client
)
{
    xXF86DRIQueryVersionReply rep;
    register int n;

    REQUEST_SIZE_MATCH(xXF86DRIQueryVersionReq);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.majorVersion = XF86DRI_MAJOR_VERSION;
    rep.minorVersion = XF86DRI_MINOR_VERSION;
    rep.patchVersion = XF86DRI_PATCH_VERSION;
    if (client->swapped) {
    	swaps(&rep.sequenceNumber, n);
    	swapl(&rep.length, n);
    }
    WriteToClient(client, sizeof(xXF86DRIQueryVersionReply), (char *)&rep);
    return (client->noClientException);
}

static int
ProcXF86DRIQueryDirectRenderingCapable(
    register ClientPtr client
)
{
    xXF86DRIQueryDirectRenderingCapableReply	rep;
    Bool isCapable;

    REQUEST(xXF86DRIQueryDirectRenderingCapableReq);
    REQUEST_SIZE_MATCH(xXF86DRIQueryDirectRenderingCapableReq);
    if (stuff->screen >= screenInfo.numScreens) {
	client->errorValue = stuff->screen;
	return BadValue;
    }

    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;

    if (!DRIQueryDirectRenderingCapable( screenInfo.screens[stuff->screen], 
					 &isCapable)) {
	return BadValue;
    }
    rep.isCapable = isCapable;

    if (!LocalClient(client))
	rep.isCapable = 0;

    WriteToClient(client, 
	sizeof(xXF86DRIQueryDirectRenderingCapableReply), (char *)&rep);
    return (client->noClientException);
}

static int
ProcXF86DRIOpenConnection(
    register ClientPtr client
)
{
    xXF86DRIOpenConnectionReply rep;
    drmHandle			hSAREA;
    char*			busIdString;

    REQUEST(xXF86DRIOpenConnectionReq);
    REQUEST_SIZE_MATCH(xXF86DRIOpenConnectionReq);
    if (stuff->screen >= screenInfo.numScreens) {
	client->errorValue = stuff->screen;
	return BadValue;
    }

    if (!DRIOpenConnection( screenInfo.screens[stuff->screen], 
			    &hSAREA,
			    &busIdString)) {
	return BadValue;
    }

    rep.type = X_Reply;
    rep.sequenceNumber = client->sequence;
    rep.busIdStringLength = 0;
    if (busIdString)
	rep.busIdStringLength = strlen(busIdString);
    rep.length = (SIZEOF(xXF86DRIOpenConnectionReply) - SIZEOF(xGenericReply) +
                  ((rep.busIdStringLength + 3) & ~3)) >> 2;

    rep.hSAREALow  = (CARD32)(hSAREA & 0xffffffff);
#ifdef LONG64
    rep.hSAREAHigh = (CARD32)(hSAREA >> 32);
#else
    rep.hSAREAHigh = 0;
#endif

    WriteToClient(client, sizeof(xXF86DRIOpenConnectionReply), (char *)&rep);
    if (rep.busIdStringLength)
	WriteToClient(client, rep.busIdStringLength, busIdString);
    return (client->noClientException);
}

static int
ProcXF86DRIAuthConnection(
    register ClientPtr client
)
{
    xXF86DRIAuthConnectionReply rep;
    
    REQUEST(xXF86DRIAuthConnectionReq);
    REQUEST_SIZE_MATCH(xXF86DRIAuthConnectionReq);
    if (stuff->screen >= screenInfo.numScreens) {
	client->errorValue = stuff->screen;
	return BadValue;
    }

    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.authenticated = 1;

    if (!DRIAuthConnection( screenInfo.screens[stuff->screen], stuff->magic)) {
        ErrorF("Failed to authenticate %u\n", stuff->magic);
	rep.authenticated = 0;
    }
    WriteToClient(client, sizeof(xXF86DRIAuthConnectionReply), (char *)&rep);
    return (client->noClientException);
}

static int
ProcXF86DRICloseConnection(
    register ClientPtr client
)
{
    REQUEST(xXF86DRICloseConnectionReq);
    REQUEST_SIZE_MATCH(xXF86DRICloseConnectionReq);
    if (stuff->screen >= screenInfo.numScreens) {
	client->errorValue = stuff->screen;
	return BadValue;
    }

    DRICloseConnection( screenInfo.screens[stuff->screen]);

    return (client->noClientException);
}

static int
ProcXF86DRIGetClientDriverName(
    register ClientPtr client
)
{
    xXF86DRIGetClientDriverNameReply	rep;
    char* clientDriverName;

    REQUEST(xXF86DRIGetClientDriverNameReq);
    REQUEST_SIZE_MATCH(xXF86DRIGetClientDriverNameReq);
    if (stuff->screen >= screenInfo.numScreens) {
	client->errorValue = stuff->screen;
	return BadValue;
    }

    DRIGetClientDriverName( screenInfo.screens[stuff->screen],
			    (int *)&rep.ddxDriverMajorVersion,
			    (int *)&rep.ddxDriverMinorVersion,
			    (int *)&rep.ddxDriverPatchVersion,
			    &clientDriverName);

    rep.type = X_Reply;
    rep.sequenceNumber = client->sequence;
    rep.clientDriverNameLength = 0;
    if (clientDriverName)
	rep.clientDriverNameLength = strlen(clientDriverName);
    rep.length = (SIZEOF(xXF86DRIGetClientDriverNameReply) - 
			SIZEOF(xGenericReply) +
			((rep.clientDriverNameLength + 3) & ~3)) >> 2;

    WriteToClient(client, 
	sizeof(xXF86DRIGetClientDriverNameReply), (char *)&rep);
    if (rep.clientDriverNameLength)
	WriteToClient(client, 
                      rep.clientDriverNameLength, 
                      clientDriverName);
    return (client->noClientException);
}

static int
ProcXF86DRICreateContext(
    register ClientPtr client
)
{
    xXF86DRICreateContextReply	rep;
    ScreenPtr pScreen;
    VisualPtr visual;
    int i;

    REQUEST(xXF86DRICreateContextReq);
    REQUEST_SIZE_MATCH(xXF86DRICreateContextReq);
    if (stuff->screen >= screenInfo.numScreens) {
	client->errorValue = stuff->screen;
	return BadValue;
    }

    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;

    pScreen = screenInfo.screens[stuff->screen];
    visual = pScreen->visuals;

    /* Find the requested X visual */
    for (i = 0; i < pScreen->numVisuals; i++, visual++)
	if (visual->vid == stuff->visual)
	    break;
    if (i == pScreen->numVisuals) {
	/* No visual found */
	return BadValue;
    }

    if (!DRICreateContext( pScreen,
			   visual,
			   stuff->context,
			   (drmContextPtr)&rep.hHWContext)) {
	return BadValue;
    }

    WriteToClient(client, sizeof(xXF86DRICreateContextReply), (char *)&rep);
    return (client->noClientException);
}

static int
ProcXF86DRIDestroyContext(
    register ClientPtr client
)
{
    REQUEST(xXF86DRIDestroyContextReq);
    REQUEST_SIZE_MATCH(xXF86DRIDestroyContextReq);
    if (stuff->screen >= screenInfo.numScreens) {
	client->errorValue = stuff->screen;
	return BadValue;
    }

    if (!DRIDestroyContext( screenInfo.screens[stuff->screen],
			    stuff->context)) {
	return BadValue;
    }

    return (client->noClientException);
}

static int
ProcXF86DRICreateDrawable(
    ClientPtr client
)
{
    xXF86DRICreateDrawableReply	rep;
    DrawablePtr pDrawable;

    REQUEST(xXF86DRICreateDrawableReq);
    REQUEST_SIZE_MATCH(xXF86DRICreateDrawableReq);
    if (stuff->screen >= screenInfo.numScreens) {
	client->errorValue = stuff->screen;
	return BadValue;
    }

    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;

    if (!(pDrawable = (DrawablePtr)SecurityLookupDrawable(
						(Drawable)stuff->drawable,
						client, 
						SecurityReadAccess))) {
	return BadValue;
    }

    if (!DRICreateDrawable( screenInfo.screens[stuff->screen],
			    (Drawable)stuff->drawable,
			    pDrawable,
			    (drmDrawablePtr)&rep.hHWDrawable)) {
	return BadValue;
    }

    WriteToClient(client, sizeof(xXF86DRICreateDrawableReply), (char *)&rep);
    return (client->noClientException);
}

static int
ProcXF86DRIDestroyDrawable(
    register ClientPtr client
)
{
    REQUEST(xXF86DRIDestroyDrawableReq);
    DrawablePtr pDrawable;
    REQUEST_SIZE_MATCH(xXF86DRIDestroyDrawableReq);
    if (stuff->screen >= screenInfo.numScreens) {
	client->errorValue = stuff->screen;
	return BadValue;
    }

    if (!(pDrawable = (DrawablePtr)SecurityLookupDrawable(
						(Drawable)stuff->drawable,
						client, 
						SecurityReadAccess))) {
	return BadValue;
    }

    if (!DRIDestroyDrawable( screenInfo.screens[stuff->screen], 
			     (Drawable)stuff->drawable,
			     pDrawable)) {
	return BadValue;
    }

    return (client->noClientException);
}

static int
ProcXF86DRIGetDrawableInfo(
    register ClientPtr client
)
{
    xXF86DRIGetDrawableInfoReply	rep;
    DrawablePtr pDrawable;
    int X, Y, W, H;
    XF86DRIClipRectPtr pClipRects;
    XF86DRIClipRectPtr pBackClipRects;
    int backX, backY;

    REQUEST(xXF86DRIGetDrawableInfoReq);
    REQUEST_SIZE_MATCH(xXF86DRIGetDrawableInfoReq);
    if (stuff->screen >= screenInfo.numScreens) {
	client->errorValue = stuff->screen;
	return BadValue;
    }

    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;

    if (!(pDrawable = (DrawablePtr)SecurityLookupDrawable(
						(Drawable)stuff->drawable,
						client, 
						SecurityReadAccess))) {
	return BadValue;
    }

    if (!DRIGetDrawableInfo( screenInfo.screens[stuff->screen],
			     pDrawable,
			     (unsigned int*)&rep.drawableTableIndex,
			     (unsigned int*)&rep.drawableTableStamp,
			     (int*)&X,
			     (int*)&Y,
			     (int*)&W,
			     (int*)&H,
			     (int*)&rep.numClipRects,
			     &pClipRects,
			     &backX, 
			     &backY,
			     (int*)&rep.numBackClipRects,
			     &pBackClipRects)) {
	return BadValue;
    }

    rep.drawableX = X;
    rep.drawableY = Y;
    rep.drawableWidth = W;
    rep.drawableHeight = H;
    rep.length = (SIZEOF(xXF86DRIGetDrawableInfoReply) - 
		  SIZEOF(xGenericReply));

    rep.backX = backX;
    rep.backY = backY;
        
    if (rep.numBackClipRects) 
       rep.length += sizeof(XF86DRIClipRectRec) * rep.numBackClipRects;    

    if (rep.numClipRects) 
       rep.length += sizeof(XF86DRIClipRectRec) * rep.numClipRects;
    
    rep.length = ((rep.length + 3) & ~3) >> 2;

    WriteToClient(client, sizeof(xXF86DRIGetDrawableInfoReply), (char *)&rep);

    if (rep.numClipRects) {
	WriteToClient(client,  
		      sizeof(XF86DRIClipRectRec) * rep.numClipRects,
		      (char *)pClipRects);
    }

    if (rep.numBackClipRects) {
       WriteToClient(client, 
		     sizeof(XF86DRIClipRectRec) * rep.numBackClipRects,
		     (char *)pBackClipRects);
    }

    return (client->noClientException);
}

static int
ProcXF86DRIGetDeviceInfo(
    register ClientPtr client
)
{
    xXF86DRIGetDeviceInfoReply	rep;
    drmHandle hFrameBuffer;
    void *pDevPrivate;

    REQUEST(xXF86DRIGetDeviceInfoReq);
    REQUEST_SIZE_MATCH(xXF86DRIGetDeviceInfoReq);
    if (stuff->screen >= screenInfo.numScreens) {
	client->errorValue = stuff->screen;
	return BadValue;
    }

    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;

    if (!DRIGetDeviceInfo( screenInfo.screens[stuff->screen],
			   &hFrameBuffer,
			   (int*)&rep.framebufferOrigin,
			   (int*)&rep.framebufferSize,
			   (int*)&rep.framebufferStride,
			   (int*)&rep.devPrivateSize,
			   &pDevPrivate)) {
	return BadValue;
    }

    rep.hFrameBufferLow  = (CARD32)(hFrameBuffer & 0xffffffff);
#ifdef LONG64
    rep.hFrameBufferHigh = (CARD32)(hFrameBuffer >> 32);
#else
    rep.hFrameBufferHigh = 0;
#endif

    rep.length = 0;
    if (rep.devPrivateSize) {
	rep.length = (SIZEOF(xXF86DRIGetDeviceInfoReply) - 
		      SIZEOF(xGenericReply) +
		      ((rep.devPrivateSize + 3) & ~3)) >> 2;
    }

    WriteToClient(client, sizeof(xXF86DRIGetDeviceInfoReply), (char *)&rep);
    if (rep.length) {
	WriteToClient(client, rep.devPrivateSize, (char *)pDevPrivate);
    }
    return (client->noClientException);
}

static int
ProcXF86DRIOpenFullScreen (
    register ClientPtr client
)
{
    REQUEST(xXF86DRIOpenFullScreenReq);
    xXF86DRIOpenFullScreenReply rep;
    DrawablePtr                 pDrawable;

    REQUEST_SIZE_MATCH(xXF86DRIOpenFullScreenReq);
    if (stuff->screen >= screenInfo.numScreens) {
	client->errorValue = stuff->screen;
	return BadValue;
    }

    rep.type           = X_Reply;
    rep.length         = 0;
    rep.sequenceNumber = client->sequence;

    if (!(pDrawable = SecurityLookupDrawable(stuff->drawable,
					     client, 
					     SecurityReadAccess)))
	return BadValue;

    rep.isFullScreen = DRIOpenFullScreen(screenInfo.screens[stuff->screen],
					 pDrawable);
    
    WriteToClient(client, sizeof(xXF86DRIOpenFullScreenReply), (char *)&rep);
    return client->noClientException;
}

static int
ProcXF86DRICloseFullScreen (
    register ClientPtr client
)
{
    REQUEST(xXF86DRICloseFullScreenReq);
    xXF86DRICloseFullScreenReply rep;
    DrawablePtr                  pDrawable;

    REQUEST_SIZE_MATCH(xXF86DRICloseFullScreenReq);
    if (stuff->screen >= screenInfo.numScreens) {
	client->errorValue = stuff->screen;
	return BadValue;
    }

    rep.type           = X_Reply;
    rep.length         = 0;
    rep.sequenceNumber = client->sequence;

    if (!(pDrawable = SecurityLookupDrawable(stuff->drawable,
					     client, 
					     SecurityReadAccess)))
	return BadValue;
    
    DRICloseFullScreen(screenInfo.screens[stuff->screen], pDrawable);
    
    WriteToClient(client, sizeof(xXF86DRICloseFullScreenReply), (char *)&rep);
    return (client->noClientException);
}

static int
ProcXF86DRIDispatch (
    register ClientPtr	client
)
{
    REQUEST(xReq);

    switch (stuff->data)
    {
    case X_XF86DRIQueryVersion:
	return ProcXF86DRIQueryVersion(client);
    case X_XF86DRIQueryDirectRenderingCapable:
	return ProcXF86DRIQueryDirectRenderingCapable(client);
    }

    if (!LocalClient(client))
	return DRIErrorBase + XF86DRIClientNotLocal;

    switch (stuff->data)
    {
    case X_XF86DRIOpenConnection:
	return ProcXF86DRIOpenConnection(client);
    case X_XF86DRICloseConnection:
	return ProcXF86DRICloseConnection(client);
    case X_XF86DRIGetClientDriverName:
	return ProcXF86DRIGetClientDriverName(client);
    case X_XF86DRICreateContext:
	return ProcXF86DRICreateContext(client);
    case X_XF86DRIDestroyContext:
	return ProcXF86DRIDestroyContext(client);
    case X_XF86DRICreateDrawable:
	return ProcXF86DRICreateDrawable(client);
    case X_XF86DRIDestroyDrawable:
	return ProcXF86DRIDestroyDrawable(client);
    case X_XF86DRIGetDrawableInfo:
	return ProcXF86DRIGetDrawableInfo(client);
    case X_XF86DRIGetDeviceInfo:
	return ProcXF86DRIGetDeviceInfo(client);
    case X_XF86DRIAuthConnection:
	return ProcXF86DRIAuthConnection(client);
    case X_XF86DRIOpenFullScreen:
	return ProcXF86DRIOpenFullScreen(client);
    case X_XF86DRICloseFullScreen:
	return ProcXF86DRICloseFullScreen(client);
    default:
	return BadRequest;
    }
}

static int
SProcXF86DRIQueryVersion(
    register ClientPtr	client
)
{
    register int n;
    REQUEST(xXF86DRIQueryVersionReq);
    swaps(&stuff->length, n);
    return ProcXF86DRIQueryVersion(client);
}

static int
SProcXF86DRIDispatch (
    register ClientPtr	client
)
{
    REQUEST(xReq);

    /* It is bound to be non-local when there is byte swapping */
    if (!LocalClient(client))
	return DRIErrorBase + XF86DRIClientNotLocal;

    /* only local clients are allowed DRI access */
    switch (stuff->data)
    {
    case X_XF86DRIQueryVersion:
	return SProcXF86DRIQueryVersion(client);
    default:
	return BadRequest;
    }
}
