/* $Xorg: lbxmain.c,v 1.4 2001/02/09 02:05:16 xorgcvs Exp $ */
/*

Copyright 1996, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

*/
/*
 * Copyright 1992 Network Computing Devices
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of NCD. not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  NCD. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * NCD. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL NCD.
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $XFree86: xc/programs/Xserver/lbx/lbxmain.c,v 1.13 2001/12/14 20:00:00 dawes Exp $ */
 
#include <sys/types.h>
#define NEED_REPLIES
#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "Xos.h"
#include "misc.h"
#include "os.h"
#include "dixstruct.h"
#include "resource.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "gcstruct.h"
#include "extnsionst.h"
#include "servermd.h"
#define _XLBX_SERVER_
#include "lbxstr.h"
#include "lbxdeltastr.h"
#include "lbxserve.h"
#include "lbximage.h"
#include "lbxsrvopts.h"
#include "lbxtags.h"
#include "Xfuncproto.h"
#include <errno.h>
#ifndef Lynx
#include <sys/uio.h>
#else
#include <uio.h>
#endif
#include <stdio.h>

#ifndef X_NOT_POSIX
#include <unistd.h>
#endif

#define CloseLbxClient	0xff

#define MAXBYTESDIFF	8

int LbxWhoAmI = 1;		/*
				 * for lbx zlib library to know who we are
				 * server = 1
				 * proxy = 0
				 */


static void LbxResetProc ( ExtensionEntry *extEntry );
static void LbxFreeClient ( ClientPtr client );
static void LbxShutdownProxy ( LbxProxyPtr proxy );
static int DecodeLbxDelta ( ClientPtr client );

static LbxProxyPtr proxyList;
unsigned char LbxReqCode;
int 	LbxEventCode;
static int BadLbxClientCode;
static int	uid_seed;

static int	lbxCompressWorkProcCount;

LbxClientPtr	lbxClients[MAXCLIENTS];

extern xConnSetupPrefix connSetupPrefix;
extern char *ConnectionInfo;
extern int  (*LbxInitialVector[3])(ClientPtr);

#ifdef DEBUG
int lbxDebug = 0;
#endif


void
LbxExtensionInit(void)
{
    ExtensionEntry *extEntry;

    lbxCompressWorkProcCount = 0;
    proxyList = NULL;
    uid_seed = 0;
    if ((extEntry = AddExtension(LBXNAME, LbxNumberEvents, LbxNumberErrors,
				 ProcLbxDispatch, SProcLbxDispatch,
				 LbxResetProc, StandardMinorOpcode)))
    {
	LbxReqCode = (unsigned char)extEntry->base;
	LbxEventCode = extEntry->eventBase;
	BadLbxClientCode = extEntry->errorBase + BadLbxClient;
        LbxDixInit();

	LbxCmapInit ();
	DeclareExtensionSecurity(LBXNAME, TRUE); 
    }
}

/*ARGSUSED*/
static void
LbxResetProc (ExtensionEntry	*extEntry)
{
   LbxResetTags();
   uid_seed = 0;
}

void
LbxCloseClient (ClientPtr	client)
{
    xLbxCloseEvent  closeEvent;
    ClientPtr	    master;
    LbxProxyPtr	    proxy;
    LbxClientPtr    lbxClient = LbxClient(client);
    CARD32	    id;

    if (!lbxClient)
	return;
    id = lbxClient->id;
    proxy = lbxClient->proxy;

    DBG (DBG_CLIENT, (stderr, "Close client %d\n", client->index));
    LbxFreeClient (client);
    if (!id)
    {
	isItTimeToYield = TRUE;
	CloseDownFileDescriptor (client);
	LbxShutdownProxy (proxy);
    } 
    else
    {
	master = NULL;
	if (proxy->lbxClients[0])
	    master = LbxProxyClient(proxy);
	if (master && !master->clientGone)
	{
	    closeEvent.type = LbxEventCode;
	    closeEvent.lbxType = LbxCloseEvent;
	    closeEvent.client = id;
	    closeEvent.sequenceNumber = master->sequence;
	    closeEvent.pad1 = closeEvent.pad2 = closeEvent.pad3 =
		closeEvent.pad4 = closeEvent.pad5 = closeEvent.pad6 = 0;
	    if (master->swapped) {
		int	    n;

		swaps(&closeEvent.sequenceNumber, n);
		swapl(&closeEvent.client, n);
	    }
	    WriteToClient(master, sizeof (closeEvent), (char *)&closeEvent);
	    LbxForceOutput(proxy);
	}
    }
}

static int
LbxReencodeEvent(ClientPtr	client,
		 LbxProxyPtr	proxy,
		 char 		*buf)
{
    xEvent *ev = (xEvent *)buf;
    int n;
    lbxMotionCache *motionCache = &proxy->motionCache;
    int motionDelta = 0;
    Bool swapCache;
    xEvent tev, *sev;

    if (ev->u.u.type != MotionNotify) {
	if (proxy->dosquishing)
	    return LbxSquishEvent(buf);
	return 0;
    }

    /*
     * Check if we can generate a motion delta event.
     *
     * The motion cache contains the last motion event the server sent.
     *
     * The following are always stored in the cache in the server's
     * byte order:
     *     sequenceNumber, time, rootX, rootY, eventX, eventY
     * This is because when determining if we can do a delta, all
     * arithmetic must be done using the server's byte order.
     *
     * The following are stored in the byte order of the latest client
     * receiving a motion event (indicated by motionCache->swapped):
     *     root, event, child, state
     * These fields do not need to be stored in the server's byte order
     * because we only use the '==' operator on them.
     */

    if (!proxy->motion_allowed_events) {
	DBG(DBG_CLIENT, (stderr, "throttling motion event for client %d\n", client->index));
	return sz_xEvent;
    }
    proxy->motion_allowed_events--;

    motionCache = &proxy->motionCache;

    if (!client->swapped)
    {
	swapCache = motionCache->swapped;
	sev = ev;
    }
    else
    {
	swapCache = !motionCache->swapped;
	sev = &tev;
	cpswaps (ev->u.keyButtonPointer.rootX,
		 sev->u.keyButtonPointer.rootX);
	cpswaps (ev->u.keyButtonPointer.rootY,
		 sev->u.keyButtonPointer.rootY);
	cpswaps (ev->u.keyButtonPointer.eventX,
		 sev->u.keyButtonPointer.eventX);
	cpswaps (ev->u.keyButtonPointer.eventY,
		 sev->u.keyButtonPointer.eventY);
	cpswaps (ev->u.u.sequenceNumber,
		 sev->u.u.sequenceNumber);
	cpswapl (ev->u.keyButtonPointer.time,
		 sev->u.keyButtonPointer.time);
    }

    if (swapCache)
    {
	swapl (&motionCache->root, n);
	swapl (&motionCache->event, n);
	swapl (&motionCache->child, n);
	swaps (&motionCache->state, n);

	motionCache->swapped = !motionCache->swapped;
    }

    motionDelta = 0;

    if (ev->u.u.detail == motionCache->detail &&
	ev->u.keyButtonPointer.root == motionCache->root &&
	ev->u.keyButtonPointer.event == motionCache->event &&
	ev->u.keyButtonPointer.child == motionCache->child &&
	ev->u.keyButtonPointer.state == motionCache->state &&
	ev->u.keyButtonPointer.sameScreen == motionCache->sameScreen) {

	int root_delta_x =
	    sev->u.keyButtonPointer.rootX - motionCache->rootX;
	int root_delta_y =
	    sev->u.keyButtonPointer.rootY - motionCache->rootY;
	int event_delta_x =
	    sev->u.keyButtonPointer.eventX - motionCache->eventX;
	int event_delta_y =
	    sev->u.keyButtonPointer.eventY - motionCache->eventY;
	unsigned long sequence_delta =
	    sev->u.u.sequenceNumber - motionCache->sequenceNumber;
	unsigned long time_delta =
	    sev->u.keyButtonPointer.time - motionCache->time;

	if (root_delta_x == event_delta_x &&
	    event_delta_x >= -128 && event_delta_x < 128 &&
	    root_delta_y == event_delta_y &&
	    event_delta_y >= -128 && event_delta_y < 128) {

	    if (sequence_delta == 0 && time_delta < 256) {

		lbxQuickMotionDeltaEvent *mev =
		    (lbxQuickMotionDeltaEvent *)(buf + sz_xEvent -
						 sz_lbxQuickMotionDeltaEvent);

		mev->type = LbxEventCode + LbxQuickMotionDeltaEvent;
		mev->deltaTime = time_delta;
		mev->deltaX = event_delta_x;
		mev->deltaY = event_delta_y;

		motionDelta = sz_xEvent - sz_lbxQuickMotionDeltaEvent;

	    } else if (sequence_delta < 65536 && time_delta < 65536) {

		lbxMotionDeltaEvent *mev =
		    (lbxMotionDeltaEvent *)(buf + sz_xEvent -
					    sz_lbxMotionDeltaEvent);

		mev->type = LbxEventCode;
		mev->lbxType = LbxMotionDeltaEvent;
		mev->deltaTime = time_delta;
		mev->deltaSequence = sequence_delta;
		mev->deltaX = event_delta_x;
		mev->deltaY = event_delta_y;

		if (LbxProxyClient(proxy)->swapped)
		{
		    swaps (&mev->deltaTime, n);
		    swaps (&mev->deltaSequence, n);
		}

		motionDelta = sz_xEvent - sz_lbxMotionDeltaEvent;
	    }
	}
    }

    motionCache->sequenceNumber = sev->u.u.sequenceNumber;
    motionCache->time = sev->u.keyButtonPointer.time;
    motionCache->rootX = sev->u.keyButtonPointer.rootX;
    motionCache->rootY = sev->u.keyButtonPointer.rootY;
    motionCache->eventX = sev->u.keyButtonPointer.eventX;
    motionCache->eventY = sev->u.keyButtonPointer.eventY;

    if (motionDelta)
	return motionDelta;

    ev->u.keyButtonPointer.pad1 = 0;
    motionCache->detail = ev->u.u.detail;
    motionCache->root = ev->u.keyButtonPointer.root;
    motionCache->event = ev->u.keyButtonPointer.event;
    motionCache->child = ev->u.keyButtonPointer.child;
    motionCache->state = ev->u.keyButtonPointer.state;
    motionCache->sameScreen = ev->u.keyButtonPointer.sameScreen;
    return 0;
}

static int
LbxComposeDelta(LbxProxyPtr	 proxy,
		char	 	*reply,
		int		 len,
		char	 	*buf)
{
    int		 diffs;
    int		 cindex;
    int		 n;
    xLbxDeltaReq *p = (xLbxDeltaReq *)buf;

    diffs = LBXDeltaMinDiffs(&proxy->outdeltas, (unsigned char *)reply, len,
			     min(MAXBYTESDIFF, (len - sz_xLbxDeltaReq) >> 1),
			     &cindex);
    if (diffs < 0) {
	LBXAddDeltaOut(&proxy->outdeltas, (unsigned char *)reply, len);
	return 0;
    }
    LBXEncodeDelta(&proxy->outdeltas, (unsigned char *)reply, diffs, cindex,
		   (unsigned char *)(&buf[sz_xLbxDeltaReq]));
    LBXAddDeltaOut(&proxy->outdeltas, (unsigned char *)reply, len);
    p->reqType = LbxEventCode;
    p->lbxReqType = LbxDeltaEvent;
    p->diffs = diffs;
    p->cindex = cindex;
    len = (sz_xLbxDeltaReq + sz_xLbxDiffItem * diffs + 3) & ~3;
    p->length = len >> 2;
    if (LbxProxyClient(proxy)->swapped) {
	swaps(&p->length, n);
    }
    return len;
}

void
LbxReencodeOutput(ClientPtr 	 client,
		  char 		*pbuf,
		  int 		*pcount,
		  char 		*cbuf,
		  int 		*ccount)
{
    LbxClientPtr lbxClient = LbxClient(client);
    LbxProxyPtr proxy = lbxClient->proxy;
    CARD32 len;
    int n;
    int count = *ccount;
    char *obuf = cbuf;

    if (client->clientState != ClientStateRunning) {
	if (DELTA_CACHEABLE(&proxy->outdeltas, count) &&
	    (n = LbxComposeDelta(proxy, cbuf, count, proxy->oDeltaBuf))) {
	    memcpy(obuf, proxy->oDeltaBuf, n);
	    *ccount -= (count - n);
	}
	return;
    }
    if (lbxClient->bytes_remaining) {
	if (count < lbxClient->bytes_remaining) {
	    lbxClient->bytes_remaining -= count;
	    return;
	}
	if (DELTA_CACHEABLE(&proxy->outdeltas, lbxClient->bytes_in_reply)) {
	    len = lbxClient->bytes_in_reply - lbxClient->bytes_remaining;
	    pbuf += (*pcount - len);
	    memcpy(proxy->replyBuf, pbuf, len);
	    memcpy(proxy->replyBuf + len, cbuf, lbxClient->bytes_remaining);
	    n = LbxComposeDelta(proxy, proxy->replyBuf,
				lbxClient->bytes_in_reply, proxy->oDeltaBuf);
	    if (!n)
		obuf += lbxClient->bytes_remaining;
	    else if (n <= len) {
		memcpy(pbuf, proxy->oDeltaBuf, n);
		*pcount -= (len - n);
		*ccount -= lbxClient->bytes_remaining;
	    } else {
		memcpy(pbuf, proxy->oDeltaBuf, len);
		memcpy(obuf, proxy->oDeltaBuf + len, n - len);
		*ccount -= lbxClient->bytes_remaining - (n - len);
		obuf += n - len;
	    }
	} else
	    obuf += lbxClient->bytes_remaining;
	cbuf += lbxClient->bytes_remaining;
	count -= lbxClient->bytes_remaining;
	lbxClient->bytes_remaining = 0;
    }
    while (count) {
	lbxClient->bytes_in_reply = sz_xEvent;
	if (((xGenericReply *)cbuf)->type == X_Reply) {
	    len = ((xGenericReply *)cbuf)->length;
	    if (client->swapped) {
		swapl(&len, n);
            }
	    lbxClient->bytes_in_reply += (len << 2);
	    if (LbxProxyClient(proxy)->swapped != client->swapped) {
		swapl(&((xGenericReply *)cbuf)->length, n);
	    }
	    if (count < lbxClient->bytes_in_reply) {
		lbxClient->bytes_remaining = lbxClient->bytes_in_reply - count;
		if (obuf != cbuf)
		    memmove(obuf, cbuf, count);
		return;
	    }
	} else if (((xGenericReply *)cbuf)->type > X_Reply &&
		   ((xGenericReply *)cbuf)->type < LASTEvent &&
		   (n = LbxReencodeEvent(client, proxy, cbuf))) {
	    cbuf += n;
	    *ccount -= n;
	    count -= n;
	    if (n == sz_xEvent)
		continue;
	    lbxClient->bytes_in_reply -= n;
	}
	if (DELTA_CACHEABLE(&proxy->outdeltas, lbxClient->bytes_in_reply) &&
	    (n = LbxComposeDelta(proxy, cbuf, lbxClient->bytes_in_reply,
				 proxy->oDeltaBuf))) {
	    memcpy(obuf, proxy->oDeltaBuf, n);
	    obuf += n;
	    *ccount -= (lbxClient->bytes_in_reply - n);
	} else {
	    if (obuf != cbuf)
		memmove(obuf, cbuf, lbxClient->bytes_in_reply);
	    obuf += lbxClient->bytes_in_reply;
	}
	cbuf += lbxClient->bytes_in_reply;
	count -= lbxClient->bytes_in_reply;
    }
}

/*ARGSUSED*/
static void
LbxReplyCallback(CallbackListPtr *pcbl,
		 pointer 	  nulldata,
		 pointer 	  calldata)
{
    ReplyInfoRec *pri = (ReplyInfoRec *)calldata;
    ClientPtr client = pri->client;
    LbxClientPtr lbxClient;
    REQUEST(xReq);

    if (!pri->startOfReply || stuff->reqType > 127)
	return;
    lbxClient = LbxClient(client);
    if (lbxClient)
	ZeroReplyPadBytes(pri->replyData, stuff->reqType);
}

/*
 * XXX If you think this is moronic, you're in good company,
 * but things definitely hang if we don't have this.
 */
/* ARGSUSED */
static Bool
LbxCheckCompressInput (ClientPtr dummy1,
		       pointer   dummy2)
{
    LbxProxyPtr	    proxy;

    if (!lbxCompressWorkProcCount)
	return TRUE;

    for (proxy = proxyList; proxy; proxy = proxy->next) {
	if (proxy->compHandle &&
	    proxy->streamOpts.streamCompInputAvail(proxy->fd))
	    AvailableClientInput (LbxProxyClient(proxy));
    }
    return FALSE;
}

static Bool
LbxIsClientBlocked (LbxClientPtr lbxClient)
{
    LbxProxyPtr		proxy = lbxClient->proxy;
    
    return (lbxClient->ignored ||
	    (GrabInProgress && lbxClient->client->index != GrabInProgress &&
	     lbxClient != proxy->lbxClients[0]));
}

static void
LbxSwitchRecv (LbxProxyPtr  proxy,
	       LbxClientPtr lbxClient)
{
    ClientPtr	client;
    
    proxy->curRecv = lbxClient;
    if (!lbxClient || lbxClient->client->clientGone)
    {
	DBG(DBG_CLIENT, (stderr, "switching to dispose input\n"));
	lbxClient = proxy->lbxClients[0];
        if (!lbxClient)
            return;
    }
    client = lbxClient->client;
    DBG (DBG_SWITCH, (stderr, "switching input to client %d\n", client->index));

    SwitchClientInput (client, FALSE);
    proxy->curDix = lbxClient;
}

/* ARGSUSED */
static Bool
LbxWaitForUnblocked (ClientPtr	client,
		     pointer	closure)
{
    LbxClientPtr    lbxClient;
    LbxProxyPtr	    proxy;

    if (client->clientGone)
	return TRUE;
    lbxClient = LbxClient(client);
    if (!lbxClient)
	return TRUE;
    proxy = lbxClient->proxy;
    if (LbxIsClientBlocked (lbxClient) ||
	((lbxClient != proxy->curDix) && proxy->curDix->reqs_pending &&
	 !LbxIsClientBlocked(proxy->curDix)))
	return FALSE;
    lbxClient->input_blocked = FALSE;
    DBG (DBG_BLOCK, (stderr, "client %d no longer blocked, switching\n",
		     client->index));
    SwitchClientInput (client, TRUE);
    proxy->curDix = lbxClient;
    return TRUE;
}

void
LbxSetForBlock(LbxClientPtr lbxClient)
{
    lbxClient->reqs_pending++;
    if (!lbxClient->input_blocked)
    {
	lbxClient->input_blocked = TRUE;
	QueueWorkProc(LbxWaitForUnblocked, lbxClient->client, NULL);
    }
}

/* ARGSUSED */
static int
LbxWaitForUngrab (ClientPtr	client,
		  pointer	closure)
{
    LbxClientPtr lbxClient = LbxClient(client);
    LbxProxyPtr  proxy;
    xLbxListenToAllEvent ungrabEvent;

    if (client->clientGone || !lbxClient)
	return TRUE;
    if (GrabInProgress)
	return FALSE;
    proxy = lbxClient->proxy;
    proxy->grabClient = 0;
    ungrabEvent.type = LbxEventCode;
    ungrabEvent.lbxType = LbxListenToAll;
    ungrabEvent.pad1 = ungrabEvent.pad2 = ungrabEvent.pad3 =
	ungrabEvent.pad4 = ungrabEvent.pad5 = ungrabEvent.pad6 =
	ungrabEvent.pad7 = 0;
    WriteToClient (client,
		   sizeof(xLbxListenToAllEvent), (char *)&ungrabEvent);
    LbxForceOutput(proxy);
    return TRUE;
}

static void
LbxServerGrab(LbxProxyPtr proxy)
{
    LbxClientPtr	grabbingLbxClient;
    xLbxListenToOneEvent grabEvent;

    /*
     * If the current grabbing client has changed, then we need
     * to send a message to update the proxy.
     */

    grabEvent.type = LbxEventCode;
    grabEvent.lbxType = LbxListenToOne;
    if (!(grabbingLbxClient = lbxClients[GrabInProgress]) ||
	grabbingLbxClient->proxy != proxy)
	grabEvent.client = 0xffffffff; /* client other than a proxy client */
    else
	grabEvent.client = grabbingLbxClient->id;
    grabEvent.pad1 = grabEvent.pad2 = grabEvent.pad3 =
	grabEvent.pad4 = grabEvent.pad5 = grabEvent.pad6 = 0;
    if (LbxProxyClient(proxy)->swapped) {
	int n;
	swapl(&grabEvent.client, n);
    }
    WriteToClient(LbxProxyClient(proxy),
		  sizeof(xLbxListenToOneEvent), (char *)&grabEvent);
    LbxForceOutput(proxy);
    if (!proxy->grabClient)
	QueueWorkProc(LbxWaitForUngrab, LbxProxyClient(proxy), NULL);
    proxy->grabClient = GrabInProgress;
}

#define MAJOROP(client) ((xReq *)client->requestBuffer)->reqType
#define MINOROP(client) ((xReq *)client->requestBuffer)->data

static Bool lbxCacheable[] = {
	FALSE,	/* LbxQueryVersion	  0 */
	FALSE,	/* LbxStartProxy	  1 */
	TRUE,	/* LbxStopProxy		  2 */
	FALSE,	/* LbxSwitch		  3 */
	FALSE,	/* LbxNewClient		  4 */
	TRUE,	/* LbxCloseClient	  5 */
	TRUE,	/* LbxModifySequence	  6 */
	FALSE,	/* LbxAllowMotion	  7 */
	TRUE,	/* LbxIncrementPixel	  8 */
	FALSE,	/* LbxDelta		  9 */
	TRUE,	/* LbxGetModifierMapping 10 */
	FALSE,	/* nothing		 11 */
	TRUE,	/* LbxInvalidateTag	 12 */
	TRUE,	/* LbxPolyPoint		 13 */
	TRUE,	/* LbxPolyLine		 14 */
	TRUE,	/* LbxPolySegment	 15 */
	TRUE,	/* LbxPolyRectangle	 16 */
	TRUE,	/* LbxPolyArc		 17 */
	TRUE,	/* LbxFillPoly		 18 */
	TRUE,	/* LbxPolyFillRectangle	 19 */
	TRUE,	/* LbxPolyFillArc	 20 */
	TRUE,	/* LbxGetKeyboardMapping 21 */
	TRUE,	/* LbxQueryFont		 22 */
	TRUE,	/* LbxChangeProperty	 23 */
	TRUE,	/* LbxGetProperty	 24 */
	TRUE,	/* LbxTagData		 25 */
	TRUE,	/* LbxCopyArea		 26 */
	TRUE,	/* LbxCopyPlane		 27 */
	TRUE,	/* LbxPolyText8		 28 */
	TRUE,	/* LbxPolyText16	 29 */
	TRUE,	/* LbxImageText8	 30 */
	TRUE,	/* LbxImageText16	 31 */
	FALSE,	/* LbxQueryExtension	 32 */
	TRUE,	/* LbxPutImage		 33 */
	TRUE,	/* LbxGetImage		 34 */
	FALSE,	/* LbxBeginLargeRequest	 35 */
	FALSE,	/* LbxLargeRequestData	 36 */
	FALSE,	/* LbxEndLargeRequest	 37 */
	FALSE,	/* LbxInternAtoms	 38 */
	TRUE,	/* LbxGetWinAttrAndGeom  39 */
	TRUE,	/* LbxGrabCmap		 40 */
	TRUE,	/* LbxReleaseCmap	 41 */
	TRUE,	/* LbxAllocColor	 42 */
	TRUE,	/* LbxSync		 43 */
};

#define NUM(a)	(sizeof (a) / sizeof (a[0]))

static int
LbxReadRequestFromClient (ClientPtr	client)
{
    int		    ret;
    LbxClientPtr    lbxClient = LbxClient(client);
    LbxProxyPtr	    proxy = lbxClient->proxy;
    ClientPtr	    masterClient = LbxProxyClient(proxy);
    Bool	    isblocked;
    Bool	    cacheable;

    DBG (DBG_READ_REQ, (stderr, "Reading request from client %d\n", client->index));

    if (GrabInProgress && (proxy->grabClient != GrabInProgress))
	LbxServerGrab(proxy);
    isblocked = LbxIsClientBlocked(lbxClient);

    if (lbxClient->reqs_pending && !isblocked) {
	ret = StandardReadRequestFromClient(client);
	if (ret > 0 && (MAJOROP(client) == LbxReqCode) &&
	    (MINOROP(client) == X_LbxEndLargeRequest))
	    ret = PrepareLargeReqBuffer(client);
	if (!--lbxClient->reqs_pending && (lbxClient != proxy->curRecv))
	    LbxSwitchRecv (proxy, proxy->curRecv);
	return ret;
    }
    while (1) {
	ret = StandardReadRequestFromClient(masterClient);
	if (ret <= 0)
	    return ret;
	client->requestBuffer = masterClient->requestBuffer;
	client->req_len = masterClient->req_len;
	cacheable = client->clientState == ClientStateRunning;
	if (cacheable && (MAJOROP(client) == LbxReqCode)) {
	    /* Check to see if this request is delta cached */
	    if (MINOROP(client) < NUM(lbxCacheable))
		cacheable = lbxCacheable[MINOROP(client)];
	    switch (MINOROP(client)) {
	    case X_LbxSwitch:
		/* Switch is sent by proxy */
		if (masterClient->swapped)
		    SProcLbxSwitch (client);
		else
		    ProcLbxSwitch (client);
		return 0;
	    case X_LbxDelta:
		ret = DecodeLbxDelta (client);
		DBG(DBG_DELTA,
		    (stderr,"delta decompressed msg %d, len = %d\n",
		     (unsigned)((unsigned char *)client->requestBuffer)[0],
		     ret));
		break;
	    case X_LbxEndLargeRequest:
		if (!isblocked)
		    ret = PrepareLargeReqBuffer(client);
		break;
	    }
	}
	if (cacheable && DELTA_CACHEABLE(&proxy->indeltas, ret)) {
	    DBG(DBG_DELTA,
		(stderr, "caching msg %d, len = %d, index = %d\n",
		 (unsigned)((unsigned char *)client->requestBuffer)[0],
		 ret, proxy->indeltas.nextDelta));
	    LBXAddDeltaIn(&proxy->indeltas, client->requestBuffer, ret);
	}
	if (client->swapped != masterClient->swapped) {
	    char        n;
	    /* put length in client order */
	    swaps(&((xReq *)client->requestBuffer)->length, n);
	}
	if (!isblocked)
	    return ret;
	DBG (DBG_BLOCK, (stderr, "Stashing %d bytes for %d\n", 
			 ret, client->index));
	AppendFakeRequest (client, client->requestBuffer, ret);
	LbxSetForBlock(lbxClient);
    }
}

static LbxClientPtr
LbxInitClient (LbxProxyPtr	proxy,
	       ClientPtr	client,
	       CARD32		id)
{
    LbxClientPtr lbxClient;
    int i;
    
    lbxClient = (LbxClientPtr) xalloc (sizeof (LbxClientRec));
    if (!lbxClient)
	return NULL;
    lbxClient->id = id;
    lbxClient->client = client;
    lbxClient->proxy = proxy;
    lbxClient->ignored = FALSE;
    lbxClient->input_blocked = FALSE;
    lbxClient->reqs_pending = 0;
    lbxClient->bytes_in_reply = 0;
    lbxClient->bytes_remaining = 0;
    client->readRequest = LbxReadRequestFromClient;
    bzero (lbxClient->drawableCache, sizeof (lbxClient->drawableCache));
    bzero (lbxClient->gcontextCache, sizeof (lbxClient->gcontextCache));
    lbxClients[client->index] = lbxClient;
    for (i = 0; proxy->lbxClients[i]; i++)
	;
    if (i > proxy->maxIndex)
	proxy->maxIndex = i;
    proxy->lbxClients[i] = lbxClient;
    proxy->numClients++;
    lbxClient->gfx_buffer = (pointer) NULL;
    lbxClient->gb_size = 0;
    return lbxClient;
}

static void
LbxFreeClient (ClientPtr client)
{
    LbxClientPtr    lbxClient = LbxClient(client);
    LbxProxyPtr	    proxy = lbxClient->proxy;
    int		    i;

    if (lbxClient != proxy->lbxClients[0]) {
	if (lbxClient == proxy->curRecv)
	    LbxSwitchRecv(proxy, NULL);
	else if (lbxClient == proxy->curDix)
	    LbxSwitchRecv(proxy, proxy->curRecv);
    }
	
    --proxy->numClients;
    lbxClients[client->index] = NULL;
    for (i = 0; i <= proxy->maxIndex; i++) {
	if (proxy->lbxClients[i] == lbxClient) {
	    proxy->lbxClients[i] = NULL;
	    break;
	}
    }
    while (proxy->maxIndex >= 0 && !proxy->lbxClients[proxy->maxIndex])
	--proxy->maxIndex;
    xfree(lbxClient->gfx_buffer);
    client->readRequest = StandardReadRequestFromClient;
    xfree (lbxClient);
}

static void
LbxFreeProxy (LbxProxyPtr proxy)
{
    LbxProxyPtr *p;

    LBXFreeDeltaCache(&proxy->indeltas);
    LBXFreeDeltaCache(&proxy->outdeltas);
    LbxFreeOsBuffers(proxy);
    if (proxy->iDeltaBuf)
	xfree(proxy->iDeltaBuf);
    if (proxy->replyBuf)
	xfree(proxy->replyBuf);
    if (proxy->oDeltaBuf)
	xfree(proxy->oDeltaBuf);
    if (proxy->compHandle)
	proxy->streamOpts.streamCompFreeHandle(proxy->compHandle);
    if (proxy->bitmapCompMethods)
	xfree (proxy->bitmapCompMethods);
    if (proxy->pixmapCompMethods)
	xfree (proxy->pixmapCompMethods);
    if (proxy->pixmapCompDepths)
    {
	int i;
	for (i = 0; i < proxy->numPixmapCompMethods; i++)
	    xfree (proxy->pixmapCompDepths[i]);
	xfree (proxy->pixmapCompDepths);
    }

    for (p = &proxyList; *p; p = &(*p)->next) {
	if (*p == proxy) {
	    *p = proxy->next;
	    break;
	}
    }
    if (!proxyList)
	DeleteCallback(&ReplyCallback, LbxReplyCallback, NULL);

    xfree (proxy);
}

LbxProxyPtr
LbxPidToProxy(int pid)
{
    LbxProxyPtr proxy;

    for (proxy = proxyList; proxy; proxy = proxy->next) {
	if (proxy->pid == pid)
	    return proxy;
    }
    return NULL;
}

static void
LbxShutdownProxy (LbxProxyPtr proxy)
{
    int		    i;
    ClientPtr	    client;

    if (proxy->compHandle)
	--lbxCompressWorkProcCount;
    while (proxy->grabbedCmaps)
	LbxReleaseCmap(proxy->grabbedCmaps, FALSE);
    for (i = 0; i <= proxy->maxIndex; i++)
    {
	if (proxy->lbxClients[i])
	{
	    client = proxy->lbxClients[i]->client;
	    if (!client->clientGone)
		CloseDownClient (client);
	}
    }
    LbxFlushTags(proxy);
    LbxFreeProxy(proxy);
}


int
ProcLbxQueryVersion (ClientPtr client)
{
    /* REQUEST(xLbxQueryVersionReq); */
    xLbxQueryVersionReply rep;
    register int n;

    REQUEST_SIZE_MATCH(xLbxQueryVersionReq);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.majorVersion = LBX_MAJOR_VERSION;
    rep.minorVersion = LBX_MINOR_VERSION;
    rep.pad0 = rep.pad1 = rep.pad2 = rep.pad3 = rep.pad4 = 0;

    if (client->swapped) {
    	swaps(&rep.sequenceNumber, n);
    	swapl(&rep.length, n);
	swaps(&rep.majorVersion, n);
	swaps(&rep.minorVersion, n);
    }
    WriteToClient(client, sizeof(xLbxQueryVersionReply), (char *)&rep);
    return (client->noClientException);
}

static int
NextProxyID (void)
{
    LbxProxyPtr proxy;
    int         id;

    for (id = 1; id < MAX_NUM_PROXIES; id++) {
	for (proxy = proxyList; proxy && proxy->pid != id; proxy = proxy->next)
	    ;
	if (!proxy)
	    return id;
    }
    return -1;
}

int
ProcLbxStartProxy (ClientPtr	client)
{
    REQUEST(xLbxStartProxyReq);
    LbxProxyPtr	    proxy;
    LbxClientPtr    lbxClient;
    int		    reqlen;
    int		    replylen;
    xLbxStartReply  *replybuf;
    LbxNegOptsRec   negopt;
    register int    n;
    pointer	    compHandle = NULL;

    REQUEST_AT_LEAST_SIZE(xLbxStartProxyReq);
    if (lbxClients[client->index])
	return BadLbxClientCode;
    proxy = (LbxProxyPtr) xalloc (sizeof (LbxProxyRec));
    if (!proxy)
	return BadAlloc;
    bzero(proxy, sizeof (LbxProxyRec));
    proxy->pid = NextProxyID();
    if (proxy->pid < 0) {	/* too many proxies */
	xfree(proxy);
	return BadAlloc;
    }
    proxy->uid = ++uid_seed;
    if (!proxyList)
	AddCallback(&ReplyCallback, LbxReplyCallback, NULL);

    if(!proxyList)
	proxyList = proxy;
    else{
	proxy->next = proxyList;
	proxyList = proxy;
    }

    /*
     * Don't know exactly how big the reply will be, but it won't be
     * bigger than the request
     */
    reqlen = client->req_len << 2;
    replybuf = (xLbxStartReply *) xalloc(max(reqlen, sz_xLbxStartReply));
    if (!replybuf) {
	LbxFreeProxy(proxy);
	return BadAlloc;
    }

    LbxOptionInit(&negopt);

    replylen = LbxOptionParse(&negopt,
			      (unsigned char *)&stuff[1],
			      reqlen - sz_xLbxStartProxyReq,
			      (unsigned char *)&replybuf->optDataStart);
    if (replylen < 0) {
	/*
	 * Didn't understand option format, so we'll just end up
	 * using the defaults.  Set nopts so that the proxy will
	 * be informed that we rejected the options because of
	 * decoding problems.
	 */
	LbxOptionInit(&negopt);
	negopt.nopts = 0xff;
	replylen = 0;
    }

    if (LBXInitDeltaCache(&proxy->indeltas, negopt.proxyDeltaN,
			  negopt.proxyDeltaMaxLen) < 0
			||
	LBXInitDeltaCache(&proxy->outdeltas, negopt.serverDeltaN,
			  negopt.serverDeltaMaxLen) < 0) {
	LbxFreeProxy(proxy);
	xfree(replybuf);
	return BadAlloc;
    }

    n = 0;
    if (negopt.proxyDeltaN)
	n = negopt.proxyDeltaMaxLen;
    if (negopt.serverDeltaN && negopt.serverDeltaMaxLen > n)
	n = negopt.serverDeltaMaxLen;
    if (n &&
	(!(proxy->iDeltaBuf = (char *)xalloc (n)) ||
	 !(proxy->replyBuf = (char *)xalloc (n)) ||
	 !(proxy->oDeltaBuf = (char *)xalloc (n)))) {
	LbxFreeProxy(proxy);
	xfree(replybuf);
	return BadAlloc;
    }

    MakeClientGrabImpervious(client);	/* proxy needs to be grab-proof */
    proxy->fd = ClientConnectionNumber(client);
    if (negopt.streamOpts.streamCompInit) {
	compHandle =
	    (*negopt.streamOpts.streamCompInit)(proxy->fd, negopt.streamOpts.streamCompArg);
	if (!compHandle) {
	    LbxFreeProxy(proxy);
	    xfree(replybuf);
	    return BadAlloc;
	}
    }
    proxy->ofirst = NULL;
    proxy->olast = NULL;
    if (!LbxInitClient (proxy, client, 0))
    {
	LbxFreeProxy(proxy);
	xfree(replybuf);
	return BadAlloc;
    }
    proxy->dosquishing = negopt.squish;
    proxy->numBitmapCompMethods = negopt.numBitmapCompMethods;
    proxy->bitmapCompMethods = negopt.bitmapCompMethods;
    proxy->numPixmapCompMethods = negopt.numPixmapCompMethods;
    proxy->pixmapCompMethods = negopt.pixmapCompMethods;
    proxy->pixmapCompDepths = negopt.pixmapCompDepths;

    proxy->streamOpts = negopt.streamOpts;
    proxy->useTags = negopt.useTags;

    proxy->grabbedCmaps = NULL;

    /* send reply */
    replybuf->type = X_Reply;
    replybuf->nOpts = negopt.nopts;
    replybuf->sequenceNumber = client->sequence;

    replylen += sz_xLbxStartReplyHdr;
    if (replylen < sz_xLbxStartReply)
	replylen = sz_xLbxStartReply;
    replybuf->length = (replylen - sz_xLbxStartReply + 3) >> 2;
    if (client->swapped) {
	swaps(&replybuf->sequenceNumber, n);
	swapl(&replybuf->length, n);
    }
    lbxClient = LbxClient(client);
    WriteToClient(client, replylen, (char *)replybuf);

    LbxProxyConnection(client, proxy);
    lbxClient = proxy->lbxClients[0];
    proxy->curDix = lbxClient;
    proxy->curRecv = lbxClient;
    proxy->compHandle = compHandle;

    if (proxy->compHandle && !lbxCompressWorkProcCount++)
	QueueWorkProc(LbxCheckCompressInput, NULL, NULL);

    xfree(replybuf);
    return Success;
}

int
ProcLbxStopProxy(ClientPtr client)
{
    /* REQUEST(xLbxStopProxyReq); */
    LbxProxyPtr	    proxy;
    LbxClientPtr    lbxClient = LbxClient(client);

    REQUEST_SIZE_MATCH(xLbxStopProxyReq);

    if (!lbxClient)
	return BadLbxClientCode;
    if (lbxClient->id)
	return BadLbxClientCode;
    
    proxy = lbxClient->proxy;
    LbxFreeClient (client);
    LbxShutdownProxy (proxy);
    return Success;
}
    
int
ProcLbxSwitch(ClientPtr	client)
{
    REQUEST(xLbxSwitchReq);
    LbxProxyPtr	proxy = LbxMaybeProxy(client);
    LbxClientPtr lbxClient;
    int i;

    REQUEST_SIZE_MATCH(xLbxSwitchReq);
    if (!proxy)
	return BadLbxClientCode;
    for (i = 0; i <= proxy->maxIndex; i++) {
	lbxClient = proxy->lbxClients[i];
	if (lbxClient && lbxClient->id == stuff->client) {
	    LbxSwitchRecv (proxy, lbxClient);
	    return Success;
	}
    }
    LbxSwitchRecv (proxy, NULL);
    return BadLbxClientCode;
}

int
ProcLbxBeginLargeRequest(ClientPtr client)
{
    REQUEST(xLbxBeginLargeRequestReq);

    client->sequence--;
    REQUEST_SIZE_MATCH(xLbxBeginLargeRequestReq);
    if (!AllocateLargeReqBuffer(client, stuff->largeReqLength << 2))
	return BadAlloc;
    return Success;
}


int
ProcLbxLargeRequestData(ClientPtr client)
{
    REQUEST(xLbxLargeRequestDataReq);

    client->sequence--;
    REQUEST_AT_LEAST_SIZE(xLbxLargeRequestDataReq);
    if (!AddToLargeReqBuffer(client, (char *) (stuff + 1),
			     (client->req_len - 1) << 2))
	return BadAlloc;
    return Success;
}


int
ProcLbxEndLargeRequest(ClientPtr client)
{
    /* REQUEST(xReq); */

    client->sequence--;
    REQUEST_SIZE_MATCH(xReq);
    return BadAlloc;
}


int
ProcLbxInternAtoms(ClientPtr client)
{
    REQUEST(xLbxInternAtomsReq);
    LbxClientPtr lbxClient = LbxClient(client);
    xLbxInternAtomsReply *replyRet;
    char *ptr = (char *) stuff + sz_xLbxInternAtomsReq;
    Atom *atomsRet;
    int replyLen, i;
    char lenbuf[2];
    CARD16 len;
    char n;

    REQUEST_AT_LEAST_SIZE(xLbxInternAtomsReq);

    if (!lbxClient)
	return BadLbxClientCode;
    if (lbxClient->id)
	return BadLbxClientCode;

    replyLen = sz_xLbxInternAtomsReplyHdr + stuff->num * sizeof (Atom);
    if (replyLen < sz_xLbxInternAtomsReply)
	replyLen = sz_xLbxInternAtomsReply;

    if (!(replyRet = (xLbxInternAtomsReply *) xalloc (replyLen)))
	return BadAlloc;

    atomsRet = (Atom *) ((char *) replyRet + sz_xLbxInternAtomsReplyHdr);

    for (i = 0; i < stuff->num; i++)
    {
	lenbuf[0] = ptr[0];
	lenbuf[1] = ptr[1];
	len = *((CARD16 *) lenbuf);
	ptr += 2;

	if ((atomsRet[i] = MakeAtom (ptr, len, TRUE)) == BAD_RESOURCE)
	{
	    xfree (replyRet);
	    return BadAlloc;
	}	    

	ptr += len;
    }

    if (client->swapped)
	for (i = 0; i < stuff->num; i++)
	    swapl (&atomsRet[i], n);

    replyRet->type = X_Reply;
    replyRet->sequenceNumber = client->sequence;
    replyRet->length = (replyLen - sz_xLbxInternAtomsReply + 3) >> 2;

    if (client->swapped) {
	swaps(&replyRet->sequenceNumber, n);
	swapl(&replyRet->length, n);
    }

    WriteToClient (client, replyLen, (char *) replyRet);

    xfree (replyRet);

    return Success;
}


int
ProcLbxGetWinAttrAndGeom(ClientPtr client)
{
    REQUEST(xLbxGetWinAttrAndGeomReq);
    xGetWindowAttributesReply wa;
    xGetGeometryReply wg;
    xLbxGetWinAttrAndGeomReply reply;
    WindowPtr pWin;
    int status;

    REQUEST_SIZE_MATCH(xLbxGetWinAttrAndGeomReq);
    pWin = (WindowPtr)SecurityLookupWindow(stuff->id, client,
					   SecurityReadAccess);
    if (!pWin)
        return(BadWindow);
    GetWindowAttributes(pWin, client, &wa);

    if ((status = GetGeometry(client, &wg)) != Success)
	return status;

    reply.type = X_Reply;
    reply.length = (sz_xLbxGetWinAttrAndGeomReply - 32) >> 2;
    reply.sequenceNumber = client->sequence;

    reply.backingStore = wa.backingStore;
    reply.visualID = wa.visualID;
#if defined(__cplusplus) || defined(c_plusplus)
    reply.c_class = wa.c_class;
#else
    reply.class = wa.class;
#endif
    reply.bitGravity = wa.bitGravity;
    reply.winGravity = wa.winGravity;
    reply.backingBitPlanes = wa.backingBitPlanes;
    reply.backingPixel = wa.backingPixel;
    reply.saveUnder = wa.saveUnder;
    reply.mapInstalled = wa.mapInstalled;
    reply.mapState = wa.mapState;
    reply.override = wa.override;
    reply.colormap = wa.colormap;
    reply.allEventMasks = wa.allEventMasks;
    reply.yourEventMask = wa.yourEventMask;
    reply.doNotPropagateMask = wa.doNotPropagateMask;
    reply.pad1 = 0;
    reply.root = wg.root;
    reply.x = wg.x;
    reply.y = wg.y;
    reply.width = wg.width;
    reply.height = wg.height;
    reply.borderWidth = wg.borderWidth;
    reply.depth = wg.depth;
    reply.pad2 = 0;

    if (client->swapped)
    {
	register char n;

	swaps(&reply.sequenceNumber, n);
	swapl(&reply.length, n);
	swapl(&reply.visualID, n);
	swaps(&reply.class, n);
	swapl(&reply.backingBitPlanes, n);
	swapl(&reply.backingPixel, n);
	swapl(&reply.colormap, n);
	swapl(&reply.allEventMasks, n);
	swapl(&reply.yourEventMask, n);
	swaps(&reply.doNotPropagateMask, n);
	swapl(&reply.root, n);
	swaps(&reply.x, n);
	swaps(&reply.y, n);
	swaps(&reply.width, n);
	swaps(&reply.height, n);
	swaps(&reply.borderWidth, n);
    }

    WriteToClient(client, sizeof(xLbxGetWinAttrAndGeomReply), (char *)&reply);
    return(client->noClientException);
}

int
ProcLbxNewClient(ClientPtr client)
{
    REQUEST(xLbxNewClientReq);
    ClientPtr	    newClient;
    LbxProxyPtr	    proxy = LbxMaybeProxy(client);
    CARD32	    id;
    int		    len, i;
    char	    *setupbuf;
    LbxClientPtr    lbxClient;

    REQUEST_AT_LEAST_SIZE(xLbxNewClientReq);

    /* save info before our request disappears */
    id = stuff->client;
    if (!proxy || !id)
	return BadLbxClientCode;
    if (proxy->numClients == MAX_LBX_CLIENTS)
	return BadAlloc;
    for (i = 1; i <= proxy->maxIndex; i++) {
	if (proxy->lbxClients[i] && proxy->lbxClients[i]->id == id)
	    return BadLbxClientCode;
    }
    len = (client->req_len << 2) - sizeof(xLbxNewClientReq);
    setupbuf = (char *)xalloc (len);
    if (!setupbuf)
	return BadAlloc;
    memcpy (setupbuf, (char *)&stuff[1], len);

    newClient = AllocLbxClientConnection (client, proxy);
    if (!newClient)
	return BadAlloc;
    newClient->requestVector = LbxInitialVector;
    lbxClient = LbxInitClient (proxy, newClient, id);
    if (!lbxClient)
    {
	CloseDownClient (newClient);
	return BadAlloc;
    }
    
    AppendFakeRequest (newClient, setupbuf, len);
    xfree (setupbuf);
    LbxSetForBlock(lbxClient);

    DBG (DBG_CLIENT, (stderr, "lbxNewClient X %d\n", newClient->index));
    return Success;
}

int
ProcLbxEstablishConnection(ClientPtr client)
{
    char *reason = NULL;
    char *auth_proto, *auth_string;
    register xConnClientPrefix *prefix;
    REQUEST(xReq);

    prefix = (xConnClientPrefix *)((char *)stuff + sz_xReq);
    auth_proto = (char *)prefix + sz_xConnClientPrefix;
    auth_string = auth_proto + ((prefix->nbytesAuthProto + 3) & ~3);
    if ((prefix->majorVersion != X_PROTOCOL) ||
	(prefix->minorVersion != X_PROTOCOL_REVISION))
	reason = "Protocol version mismatch";
    else
	reason = ClientAuthorized(client,
				  prefix->nbytesAuthProto,
				  auth_proto,
				  prefix->nbytesAuthString,
				  auth_string);

    if (client->clientState == ClientStateCheckingSecurity ||
	client->clientState == ClientStateAuthenticating)
	return (client->noClientException = -1); /* XXX some day */
    return(LbxSendConnSetup(client, reason));
}

int
ProcLbxCloseClient (ClientPtr client)
{
    REQUEST(xLbxCloseClientReq);
    LbxClientPtr lbxClient = LbxClient(client);

    REQUEST_SIZE_MATCH(xLbxCloseClientReq);
    if (!lbxClient || lbxClient->id != stuff->client)
	return BadLbxClientCode;

    /* this will cause the client to be closed down back in Dispatch() */
    return(client->noClientException = CloseLbxClient);
}

int
ProcLbxModifySequence (ClientPtr client)
{
    REQUEST(xLbxModifySequenceReq);

    REQUEST_SIZE_MATCH(xLbxModifySequenceReq);
    client->sequence += (stuff->adjust - 1);	/* Dispatch() adds 1 */
    return Success;
}

int
ProcLbxAllowMotion (ClientPtr client)
{
    REQUEST(xLbxAllowMotionReq);

    client->sequence--;
    REQUEST_SIZE_MATCH(xLbxAllowMotionReq);
    LbxAllowMotion(client, stuff->num);
    return Success;
}


static int
DecodeLbxDelta (ClientPtr client)
{
    REQUEST(xLbxDeltaReq);
    LbxClientPtr    lbxClient = LbxClient(client);
    LbxProxyPtr	    proxy = lbxClient->proxy;
    int		    len;
    unsigned char  *buf;

    /* Note that LBXDecodeDelta decodes and adds current msg to the cache */
    len = LBXDecodeDelta(&proxy->indeltas, 
			 (xLbxDiffItem *)(((char *)stuff) + sz_xLbxDeltaReq),
			 stuff->diffs, stuff->cindex, &buf);
    /*
     * Some requests, such as FillPoly, result in the protocol input
     * buffer being modified.  So we need to copy the request
     * into a temporary buffer where a write would be harmless.
     * Maybe some day do this copying on a case by case basis,
     * since not all requests are guilty of this.
     */
    memcpy(proxy->iDeltaBuf, buf, len);

    client->requestBuffer = proxy->iDeltaBuf;
    client->req_len = len >> 2;
    return len;
}

int
ProcLbxGetModifierMapping(ClientPtr client)
{
    /* REQUEST(xLbxGetModifierMappingReq); */

    REQUEST_SIZE_MATCH(xLbxGetModifierMappingReq);
    return LbxGetModifierMapping(client);
}

int
ProcLbxGetKeyboardMapping(ClientPtr client)
{
    /* REQUEST(xLbxGetKeyboardMappingReq); */

    REQUEST_SIZE_MATCH(xLbxGetKeyboardMappingReq);
    return LbxGetKeyboardMapping(client);
}

int
ProcLbxQueryFont(ClientPtr client)
{
    /* REQUEST(xLbxQueryFontReq); */

    REQUEST_SIZE_MATCH(xLbxQueryFontReq);
    return LbxQueryFont(client);
}

int
ProcLbxChangeProperty(ClientPtr	client)
{
    /* REQUEST(xLbxChangePropertyReq); */

    REQUEST_SIZE_MATCH(xLbxChangePropertyReq);
    return LbxChangeProperty(client);
}

int
ProcLbxGetProperty(ClientPtr client)
{
    /* REQUEST(xLbxGetPropertyReq); */

    REQUEST_SIZE_MATCH(xLbxGetPropertyReq);
    return LbxGetProperty(client);
}

int
ProcLbxTagData(ClientPtr client)
{
    REQUEST(xLbxTagDataReq);

    client->sequence--;		/* not a counted request */
    REQUEST_AT_LEAST_SIZE(xLbxTagDataReq);

    return LbxTagData(client, stuff->tag, stuff->real_length,
    		 (pointer)&stuff[1]);	/* better not give any errors */
}

int
ProcLbxInvalidateTag(ClientPtr client)
{
    REQUEST(xLbxInvalidateTagReq);

    client->sequence--;
    REQUEST_SIZE_MATCH(xLbxInvalidateTagReq);
    return LbxInvalidateTag(client, stuff->tag);
}

int
ProcLbxPolyPoint(ClientPtr client)
{
    return LbxDecodePoly(client, X_PolyPoint, LbxDecodePoints);
}

int
ProcLbxPolyLine(ClientPtr client)
{
    return LbxDecodePoly(client, X_PolyLine, LbxDecodePoints);
}

int
ProcLbxPolySegment(ClientPtr client)
{
    return LbxDecodePoly(client, X_PolySegment, LbxDecodeSegment);
}

int
ProcLbxPolyRectangle(ClientPtr client)
{
    return LbxDecodePoly(client, X_PolyRectangle, LbxDecodeRectangle);
}

int
ProcLbxPolyArc(ClientPtr client)
{
    return LbxDecodePoly(client, X_PolyArc, LbxDecodeArc);
}

int
ProcLbxFillPoly(ClientPtr client)
{
    return LbxDecodeFillPoly(client);
}

int
ProcLbxPolyFillRectangle(ClientPtr client)
{
    return LbxDecodePoly(client, X_PolyFillRectangle, LbxDecodeRectangle);
}

int
ProcLbxPolyFillArc(ClientPtr client)
{
    return LbxDecodePoly(client, X_PolyFillArc, LbxDecodeArc);
}

int
ProcLbxCopyArea(ClientPtr client)
{
    return LbxDecodeCopyArea(client);
}

int
ProcLbxCopyPlane(ClientPtr client)
{
    return LbxDecodeCopyPlane(client);
}


int
ProcLbxPolyText(ClientPtr client)
{
    return LbxDecodePolyText(client);
}

int
ProcLbxImageText(ClientPtr client)
{
    return LbxDecodeImageText(client);
}

int
ProcLbxQueryExtension(ClientPtr	client)
{
    REQUEST(xLbxQueryExtensionReq);
    char	*ename;

    REQUEST_AT_LEAST_SIZE(xLbxQueryExtensionReq);
    ename = (char *) &stuff[1];
    return LbxQueryExtension(client, ename, stuff->nbytes);
}

int
ProcLbxPutImage(ClientPtr client)
{
    return LbxDecodePutImage(client);
}

int
ProcLbxGetImage(ClientPtr client)
{
    return LbxDecodeGetImage(client);
}


int
ProcLbxSync(ClientPtr client)
{
    xLbxSyncReply reply;

    client->sequence--;		/* not a counted request */

#ifdef COLOR_DEBUG
    fprintf (stderr, "Got LBX sync, seq = 0x%x\n", client->sequence);
#endif

    reply.type = X_Reply;
    reply.length = 0;
    reply.sequenceNumber = client->sequence;
    reply.pad0 = reply.pad1 = reply.pad2 = reply.pad3 = reply.pad4 = 
        reply.pad5 = reply.pad6 = 0;

    if (client->swapped)
    {
	register char n;
	swaps (&reply.sequenceNumber, n);
    }

    WriteToClient (client, sz_xLbxSyncReply, (char *)&reply);

    return (client->noClientException);
}


int
ProcLbxDispatch(ClientPtr client)
{
    REQUEST(xReq);
    switch (stuff->data)
    {
    case X_LbxQueryVersion:
	return ProcLbxQueryVersion(client);
    case X_LbxStartProxy:
	return ProcLbxStartProxy(client);
    case X_LbxStopProxy:
	return ProcLbxStopProxy(client);
    case X_LbxNewClient:
	return ProcLbxNewClient(client);
    case X_LbxCloseClient:
	return ProcLbxCloseClient(client);
    case X_LbxModifySequence:
	return ProcLbxModifySequence(client);
    case X_LbxAllowMotion:
	return ProcLbxAllowMotion(client);
    case X_LbxIncrementPixel:
	return ProcLbxIncrementPixel(client);
    case X_LbxGrabCmap:
	return ProcLbxGrabCmap(client);
    case X_LbxReleaseCmap:
	return ProcLbxReleaseCmap(client);
    case X_LbxAllocColor:
	return ProcLbxAllocColor(client);
    case X_LbxGetModifierMapping:
	return ProcLbxGetModifierMapping(client);
    case X_LbxGetKeyboardMapping:
	return ProcLbxGetKeyboardMapping(client);
    case X_LbxInvalidateTag:
	return ProcLbxInvalidateTag(client);
    case X_LbxPolyPoint:
	return ProcLbxPolyPoint (client);
    case X_LbxPolyLine:
	return ProcLbxPolyLine (client);
    case X_LbxPolySegment:
	return ProcLbxPolySegment (client);
    case X_LbxPolyRectangle:
	return ProcLbxPolyRectangle (client);
    case X_LbxPolyArc:
	return ProcLbxPolyArc (client);
    case X_LbxFillPoly:
	return ProcLbxFillPoly (client);
    case X_LbxPolyFillRectangle:
	return ProcLbxPolyFillRectangle (client);
    case X_LbxPolyFillArc:
	return ProcLbxPolyFillArc (client);
    case X_LbxQueryFont:
	return ProcLbxQueryFont (client);
    case X_LbxChangeProperty:
	return ProcLbxChangeProperty (client);
    case X_LbxGetProperty:
	return ProcLbxGetProperty (client);
    case X_LbxTagData:
	return ProcLbxTagData (client);
    case X_LbxCopyArea:
	return ProcLbxCopyArea (client);
    case X_LbxCopyPlane:
	return ProcLbxCopyPlane (client);
    case X_LbxPolyText8:
    case X_LbxPolyText16:
	return ProcLbxPolyText (client);
    case X_LbxImageText8:
    case X_LbxImageText16:
	return ProcLbxImageText (client);
    case X_LbxQueryExtension:
	return ProcLbxQueryExtension (client);
    case X_LbxPutImage:
	return ProcLbxPutImage (client);
    case X_LbxGetImage:
	return ProcLbxGetImage (client);
    case X_LbxInternAtoms:
	return ProcLbxInternAtoms(client);
    case X_LbxGetWinAttrAndGeom:
	return ProcLbxGetWinAttrAndGeom(client);
    case X_LbxSync:
	return ProcLbxSync(client);
    case X_LbxBeginLargeRequest:
	return ProcLbxBeginLargeRequest(client);
    case X_LbxLargeRequestData:
	return ProcLbxLargeRequestData(client);
    case X_LbxEndLargeRequest:
	return ProcLbxLargeRequestData(client);
    default:
	return BadRequest;
    }
}
