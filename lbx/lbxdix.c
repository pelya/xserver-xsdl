/* $Xorg: lbxdix.c,v 1.4 2001/02/09 02:05:16 xorgcvs Exp $ */
/*

Copyright 1998  The Open Group

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
 * Copyright 1993 Network Computing Devices, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name Network Computing Devices, Inc. not be
 * used in advertising or publicity pertaining to distribution of this
 * software without specific, written prior permission.
 *
 * THIS SOFTWARE IS PROVIDED `AS-IS'.  NETWORK COMPUTING DEVICES, INC.,
 * DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING WITHOUT
 * LIMITATION ALL IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NONINFRINGEMENT.  IN NO EVENT SHALL NETWORK
 * COMPUTING DEVICES, INC., BE LIABLE FOR ANY DAMAGES WHATSOEVER, INCLUDING
 * SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES, INCLUDING LOSS OF USE, DATA,
 * OR PROFITS, EVEN IF ADVISED OF THE POSSIBILITY THEREOF, AND REGARDLESS OF
 * WHETHER IN AN ACTION IN CONTRACT, TORT OR NEGLIGENCE, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $XFree86: xc/programs/Xserver/lbx/lbxdix.c,v 1.8 2001/12/14 19:59:59 dawes Exp $ */

/* various bits of DIX-level mangling */

#include <sys/types.h>
#include <stdio.h>
#define NEED_REPLIES
#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "os.h"
#include "dixstruct.h"
#include "resource.h"
#include "inputstr.h"
#include "servermd.h"
#include "dixfontstr.h"
#include "gcstruct.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "propertyst.h"
#define _XLBX_SERVER_
#include "lbxstr.h"
#include "lbxserve.h"
#include "lbxtags.h"
#include "lbxdata.h"
#include "Xfuncproto.h"
#ifdef XAPPGROUP
#include "Xagsrv.h"
#endif
#include "swaprep.h"

int         lbx_font_private = -1;

void
LbxDixInit(void)
{
    TagInit();
    lbx_font_private = AllocateFontPrivateIndex();
}

/* ARGSUSED */
void
LbxAllowMotion(ClientPtr       	client,
	       int		num)
{
    LbxProxyPtr proxy = LbxProxy(client);
    proxy->motion_allowed_events += num;
}

extern xConnSetupPrefix connSetupPrefix;
extern char *ConnectionInfo;
extern int  connBlockScreenStart;

int
LbxSendConnSetup(ClientPtr   client,
		 char       *reason)
{
    int		dlength;
    int         i, ndex, lim, wndex;
    CARD32      dataBuf[16];
    xLbxConnSetupPrefix csp;
    NewClientInfoRec nci;
    LbxProxyPtr proxy = LbxProxy(client);

    if (reason) {
	SendConnSetup(client, reason);
	LbxForceOutput(proxy); /* expedient to avoid another state variable */
	return (client->noClientException);
    }

    IncrementClientCount();

    client->requestVector = client->swapped ? SwappedProcVector : ProcVector;
    client->sequence = 0;
    dataBuf[0] = client->clientAsMask;

    csp.success = TRUE;
    csp.majorVersion = connSetupPrefix.majorVersion;
    csp.minorVersion = connSetupPrefix.minorVersion;
    csp.tag = 0;
#ifdef XAPPGROUP
    if (!client->appgroup) {
#endif
    csp.changeType = 1; /* LbxNormalDeltas */
    csp.length = 2 +			/* tag + resource-id-base */
		 screenInfo.numScreens; /* input-mask per screen */
    wndex = 0; ndex = 1; lim = screenInfo.numScreens;
#ifdef XAPPGROUP
    } else {
	csp.changeType = 2; /* LbxAppGroupDeltas */
	csp.length = 7 + /* tag, res-id-base, root, visual, colormap, b&w-pix */
		     1 + screenInfo.numScreens - screenInfo.numVideoScreens;
	XagGetDeltaInfo (client, &dataBuf[1]);
	for (i = 0; i < MAXSCREENS; i++) {
	    if ((CARD32) WindowTable[i]->drawable.id == dataBuf[1]) {
		dataBuf[6] =  WindowTable[i]->eventMask | wOtherEventMasks(WindowTable[i]);
		break;
	    }
	}
	wndex = screenInfo.numVideoScreens;
	ndex = 7;
	lim = screenInfo.numScreens - screenInfo.numVideoScreens;
    }
#endif
    for (i = 0; i < lim; i++, ndex++, wndex++) {
	dataBuf[ndex] = 
	    WindowTable[wndex]->eventMask | wOtherEventMasks(WindowTable[wndex]);
    }
    dlength = (csp.length - 1) << 2;

    if (LbxProxyClient(proxy)->swapped) {
	swaps(&csp.length, i);
    }

    if (client->swapped) {
	LbxWriteSConnSetupPrefix(client, &csp);
	SwapLongs(dataBuf, (1 + screenInfo.numScreens));
	WriteToClient(client, dlength, (pointer) dataBuf);
    } else {
	WriteToClient(client, sizeof(xLbxConnSetupPrefix), (char *) &csp);
	WriteToClient(client, dlength, (pointer) dataBuf);
    }

    LbxForceOutput(proxy); /* expedient to avoid another state variable */
    client->clientState = ClientStateRunning;
    if (ClientStateCallback) {
	if (LbxProxyClient(proxy)->swapped != client->swapped) {
	    swaps(&csp.length, i);
	}
	nci.client = client;
	nci.prefix = (xConnSetupPrefix*) &csp;
	nci.setup = (xConnSetup *) ConnectionInfo;
	CallCallbacks(&ClientStateCallback, (pointer) &nci);
    }

    return client->noClientException;
}

static XID modifier_map_tag;

int
LbxGetModifierMapping(ClientPtr   client)
{
    TagData     td;
    pointer     tagdata;
    xLbxGetModifierMappingReply rep;
    register KeyClassPtr keyc = inputInfo.keyboard->key;
    int         dlength = keyc->maxKeysPerModifier << 3;
    Bool        tag_known = FALSE,
                send_data;
    int         n;

    if (!modifier_map_tag) {
	tagdata = (pointer) keyc->modifierKeyMap;
	TagSaveTag(LbxTagTypeModmap, dlength, tagdata, &modifier_map_tag);
    } else {
	td = TagGetTag(modifier_map_tag);
	tagdata = td->tdata;
	tag_known = TagProxyMarked(modifier_map_tag, LbxProxyID(client));
    }
    if (modifier_map_tag)
	TagMarkProxy(modifier_map_tag, LbxProxyID(client));

    send_data = (!modifier_map_tag || !tag_known);

    rep.type = X_Reply;
    rep.keyspermod = keyc->maxKeysPerModifier;
    rep.sequenceNumber = client->sequence;
    rep.tag = modifier_map_tag;
    rep.pad0 = rep.pad1 = rep.pad2 = rep.pad3 = rep.pad4 = 0;

    if (send_data)
	rep.length = dlength >> 2;
    else
	rep.length = 0;

    if (client->swapped) {
	swaps(&rep.sequenceNumber, n);
	swapl(&rep.length, n);
	swapl(&rep.tag, n);
    }
    WriteToClient(client, sizeof(xLbxGetModifierMappingReply), (char *)&rep);

    if (send_data)
	WriteToClient(client, dlength, (char *) tagdata);

    return client->noClientException;
}

void
LbxFlushModifierMapTag(void)
{

    if (modifier_map_tag)
	TagDeleteTag(modifier_map_tag);
}

static XID keyboard_map_tag;

int
LbxGetKeyboardMapping(ClientPtr   client)
{
    TagData     td;
    pointer     tagdata;
    xLbxGetKeyboardMappingReply rep;

    REQUEST(xLbxGetKeyboardMappingReq);
    KeySymsPtr  curKeySyms = &inputInfo.keyboard->key->curKeySyms;
    int         dlength;
    Bool        tag_known = FALSE,
                send_data;
    int         n;

    REQUEST_SIZE_MATCH(xLbxGetKeyboardMappingReq);

    if ((stuff->firstKeyCode < curKeySyms->minKeyCode) ||
	    (stuff->firstKeyCode > curKeySyms->maxKeyCode)) {
	client->errorValue = stuff->firstKeyCode;
	return BadValue;
    }
    if (stuff->firstKeyCode + stuff->count > curKeySyms->maxKeyCode + 1) {
	client->errorValue = stuff->count;
	return BadValue;
    }
    rep.type = X_Reply;
    rep.sequenceNumber = client->sequence;
    rep.keysperkeycode = curKeySyms->mapWidth;
    /* length is a count of 4 byte quantities and KeySyms are 4 bytes */

    if (!keyboard_map_tag) {
	tagdata = (pointer) &curKeySyms->map[(stuff->firstKeyCode -
			     curKeySyms->minKeyCode) * curKeySyms->mapWidth];
	dlength = (curKeySyms->mapWidth * stuff->count);
	TagSaveTag(LbxTagTypeKeymap, dlength, tagdata, &keyboard_map_tag);
    } else {
	td = TagGetTag(keyboard_map_tag);
	tagdata = td->tdata;
	tag_known = TagProxyMarked(keyboard_map_tag, LbxProxyID(client));
    }
    if (keyboard_map_tag)
	TagMarkProxy(keyboard_map_tag, LbxProxyID(client));

    send_data = (!keyboard_map_tag || !tag_known);
    rep.type = X_Reply;
    rep.keysperkeycode = curKeySyms->mapWidth;
    rep.sequenceNumber = client->sequence;
    rep.tag = keyboard_map_tag;
    rep.pad0 = rep.pad1 = rep.pad2 = rep.pad3 = rep.pad4 = 0;

    if (send_data)
	rep.length = (curKeySyms->mapWidth * stuff->count);
    else
	rep.length = 0;

    if (client->swapped) {
	swaps(&rep.sequenceNumber, n);
	swapl(&rep.length, n);
	swapl(&rep.tag, n);
    }
    WriteToClient(client, sizeof(xLbxGetKeyboardMappingReply), (char *)&rep);

    if (send_data) {
	client->pSwapReplyFunc = (ReplySwapPtr)CopySwap32Write;
	WriteSwappedDataToClient(client,
			curKeySyms->mapWidth * stuff->count * sizeof(KeySym),
	    &curKeySyms->map[(stuff->firstKeyCode - curKeySyms->minKeyCode) *
			     curKeySyms->mapWidth]);
    }
    return client->noClientException;
}

void
LbxFlushKeyboardMapTag(void)
{
    if (keyboard_map_tag)
	TagDeleteTag(keyboard_map_tag);
}

/* counts number of bits needed to hold value */
static int
_bitsize(int val)
{
    int         bits = 1;  /* always need one for sign bit */

    if (val == 0)
	return (bits);

    if (val < 0) {
	val = -val;
    }
    while (val) {
	bits++;
	val >>= 1;
    }

    return bits;

}

/*
 * squashes the font (if possible), returning the new length and
 * a pointer to the new data (which has been allocated).  if it can't
 * squish, it just returns a 0 and the data is sent in raw form.
 */
int  _lbx_fi_junklen = sizeof(BYTE) * 2 + sizeof(CARD16) + sizeof(CARD32);

static int
squish_font_info(xQueryFontReply *qfr,
		 int             rlen,
		 xLbxFontInfo    **sqrep)
{
    int         len,
                hlen;
    xLbxFontInfo *new;
    xCharInfo  *minb,
               *maxb,
		*ci,
                bbox;
    int         i;
    char	*t;
    xLbxCharInfo *chars;
    int	num_chars;

    num_chars = qfr->nCharInfos;

    if (num_chars == 0)
	return 0;

    minb = &qfr->minBounds;
    maxb = &qfr->maxBounds;
    /*
     * first do the quick check -- if the attribute fields aren't all the
     * same, punt
     */

    if (minb->attributes != maxb->attributes)
	return 0;

#define	compute(field)	\
    bbox.field = max(_bitsize(minb->field), _bitsize(maxb->field))

    compute(characterWidth);
    compute(leftSideBearing);
    compute(rightSideBearing);
    compute(ascent);
    compute(descent);

#undef compute

    /* make sure it fits */
    if (!((bbox.characterWidth <= LBX_WIDTH_BITS) &&
	  (bbox.leftSideBearing <= LBX_LEFT_BITS) &&
	  (bbox.rightSideBearing <= LBX_RIGHT_BITS) &&
	  (bbox.ascent <= LBX_ASCENT_BITS) &&
	  (bbox.descent <= LBX_DESCENT_BITS))) {
	return 0;
    }

    hlen = sizeof(xLbxFontInfo) + qfr->nFontProps * sizeof(xFontProp);

    len = hlen + (num_chars * sizeof(xLbxCharInfo));

    new = (xLbxFontInfo *) xalloc(len);
    if (!new)
	return 0;

    /* gross hack to avoid copying all the fields */
    t = (char *) qfr;
    t += _lbx_fi_junklen;

    /* copy all but the char infos */
    memcpy((char *) new, (char *) t, hlen);

    t = (char *) new;
    t += hlen;
    chars = (xLbxCharInfo *) t;

    t = (char *) qfr;
    t += sizeof(xQueryFontReply) + qfr->nFontProps * sizeof(xFontProp);
    ci = (xCharInfo *) t;

    /* now copy & pack the charinfos */
    for (i = 0; i < num_chars; i++, chars++, ci++) {
	chars->metrics = 0;
	chars->metrics |= (LBX_MASK_BITS(ci->characterWidth, LBX_WIDTH_BITS)
			   << LBX_WIDTH_SHIFT);
	chars->metrics |= (LBX_MASK_BITS(ci->leftSideBearing, LBX_LEFT_BITS)
			   << LBX_LEFT_SHIFT);
	chars->metrics |= (LBX_MASK_BITS(ci->rightSideBearing, LBX_RIGHT_BITS)
			   << LBX_RIGHT_SHIFT);
	chars->metrics |= (LBX_MASK_BITS(ci->ascent, LBX_ASCENT_BITS)
			   << LBX_ASCENT_SHIFT);
	chars->metrics |= (LBX_MASK_BITS(ci->descent, LBX_DESCENT_BITS)
			   << LBX_DESCENT_SHIFT);
    }

    *sqrep = new;
    return len;
}

int
LbxQueryFont(ClientPtr   client)
{
    xQueryFontReply *reply;
    xLbxQueryFontReply lbxrep;
    FontPtr     pFont;
    register GC *pGC;
    Bool        send_data = FALSE;
    Bool        free_data = FALSE;
    int         rlength = 0;
    FontTagInfoPtr ftip;
    int         sqlen = 0;
    xLbxFontInfo *sqrep,
               *sreply = NULL;

    REQUEST(xLbxQueryFontReq);

    REQUEST_SIZE_MATCH(xLbxQueryFontReq);

    client->errorValue = stuff->fid;	/* EITHER font or gc */
    pFont = (FontPtr) SecurityLookupIDByType(client, stuff->fid, RT_FONT,
					     SecurityReadAccess);
    if (!pFont) {
	/* can't use VERIFY_GC because it might return BadGC */
	pGC = (GC *) SecurityLookupIDByType(client, stuff->fid, RT_GC,
					    SecurityReadAccess);
	if (!pGC || !pGC->font) {	/* catch a non-existent builtin font */
	    client->errorValue = stuff->fid;
	    return (BadFont);	/* procotol spec says only error is BadFont */
	}
	pFont = pGC->font;
    }

    /* get tag (if any) */
    ftip = (FontTagInfoPtr) FontGetPrivate(pFont, lbx_font_private);

    if (!ftip) {
	xCharInfo  *pmax = FONTINKMAX(pFont);
	xCharInfo  *pmin = FONTINKMIN(pFont);
	int         nprotoxcistructs;

	nprotoxcistructs = (
			  pmax->rightSideBearing == pmin->rightSideBearing &&
			    pmax->leftSideBearing == pmin->leftSideBearing &&
			    pmax->descent == pmin->descent &&
			    pmax->ascent == pmin->ascent &&
			    pmax->characterWidth == pmin->characterWidth) ?
	    0 : N2dChars(pFont);

	rlength = sizeof(xQueryFontReply) +
	    FONTINFONPROPS(FONTCHARSET(pFont)) * sizeof(xFontProp) +
	    nprotoxcistructs * sizeof(xCharInfo);
	reply = (xQueryFontReply *) xalloc(rlength);
	if (!reply) {
	    return (BadAlloc);
	}
	free_data = TRUE;
	send_data = TRUE;
	QueryFont(pFont, reply, nprotoxcistructs);

	sqlen = squish_font_info(reply, rlength, &sqrep);
	if (!sqlen) {		/* if it failed to squish, send it raw */
	    char *t;

	    lbxrep.compression = 0;

	    sqlen = rlength - _lbx_fi_junklen;
	    t = (char *) reply;
	    sqrep = (xLbxFontInfo *) (t + _lbx_fi_junklen);
	} else {
	    lbxrep.compression = 1;
	    xfree(reply);	/* no longer needed */
	}
    } else {			/* just get data from tag */
	sqrep = ftip->fontinfo;
	sqlen = ftip->size;
	lbxrep.compression = ftip->compression;
    }

    if (!ftip) {
	/* data allocation is done when font is first queried */
	ftip = (FontTagInfoPtr) xalloc(sizeof(FontTagInfoRec));
	if (ftip &&
	    TagSaveTag(LbxTagTypeFont, sqlen, (pointer) ftip, &ftip->tid)) {
	    FontSetPrivate(pFont, lbx_font_private, (pointer) ftip);
	    ftip->pfont = pFont;
	    ftip->size = sqlen;
	    ftip->fontinfo = sqrep;
	    ftip->compression = lbxrep.compression;
	    free_data = FALSE;
	} else {
	    xfree(ftip);
	}
    }
    if (ftip) {
	if (!TagProxyMarked(ftip->tid, LbxProxyID(client)))
	    send_data = TRUE;
	TagMarkProxy(ftip->tid, LbxProxyID(client));
	lbxrep.tag = ftip->tid;
    } else {
	lbxrep.tag = 0;
	send_data = TRUE;
    }

    lbxrep.type = X_Reply;
    lbxrep.sequenceNumber = client->sequence;
    lbxrep.pad0 = lbxrep.pad1 = lbxrep.pad2 = lbxrep.pad3 = lbxrep.pad4 = 0;

    if (send_data)
	lbxrep.length = sqlen >> 2;
    else
	lbxrep.length = 0;

    if (client->swapped) {
	int         n;

	swaps(&lbxrep.sequenceNumber, n);
	swapl(&lbxrep.length, n);
	swapl(&lbxrep.tag, n);
	sreply = (xLbxFontInfo *) ALLOCATE_LOCAL(sqlen);
	if (!sreply)
	    return BadAlloc;
	memcpy((char *) sreply, (char *) sqrep, sqlen);
	LbxSwapFontInfo(sreply, lbxrep.compression);
	sqrep = sreply;
    }
    WriteToClient(client, sizeof(xLbxQueryFontReply), (char *) &lbxrep);
    if (send_data)
	WriteToClient(client, sqlen, (char *)sqrep);
    if (free_data)
	xfree(sqrep);
    if (sreply)
	DEALLOCATE_LOCAL(sreply);
    return (client->noClientException);
}

void
LbxFreeFontTag(FontPtr     pfont)
{
    FontTagInfoPtr ftip;

    ftip = (FontTagInfoPtr) FontGetPrivate(pfont, lbx_font_private);
    if (ftip)
	TagDeleteTag(ftip->tid);
}

int
LbxInvalidateTag(ClientPtr   client,
		 XID         tag)
{
    TagClearProxy(tag, LbxProxyID(client));
    return client->noClientException;
}

void
LbxSendInvalidateTag(ClientPtr   client,
		     XID         tag,
		     int         tagtype)
{
    xLbxInvalidateTagEvent ev;
    int         n;

    ev.type = LbxEventCode;
    ev.lbxType = LbxInvalidateTagEvent;
    ev.sequenceNumber = client->sequence;
    ev.tag = tag;
    ev.tagType = tagtype;
    ev.pad1 = ev.pad2 = ev.pad3 = ev.pad4 = ev.pad5 = 0;

    if (client->swapped) {
	swaps(&ev.sequenceNumber, n);
	swapl(&ev.tag, n);
	swapl(&ev.tagType, n);
    }
    DBG(DBG_CLIENT, (stderr, "Invalidating tag  %d\n", tag));
    WriteToClient(client, sizeof(xLbxInvalidateTagEvent), (char *) &ev);
    LbxForceOutput(LbxProxy(client));
}

static void
LbxSendSendTagData(int         pid,
		   XID         tag,
		   int         tagtype)
{
    xLbxSendTagDataEvent ev;
    int         n;
    LbxProxyPtr proxy;
    ClientPtr   client;
    LbxClientPtr lbxcp;

    proxy = LbxPidToProxy(pid);
    lbxcp = (proxy != NULL) ? proxy->lbxClients[0] : NULL;
    if (lbxcp && (client = lbxcp->client)) {
	ev.type = LbxEventCode;
	ev.lbxType = LbxSendTagDataEvent;
	ev.sequenceNumber = client->sequence;
	ev.tag = tag;
	ev.tagType = tagtype;
	ev.pad1 = ev.pad2 = ev.pad3 = ev.pad4 = ev.pad5 = 0;

	if (client->swapped) {
	    swaps(&ev.sequenceNumber, n);
	    swapl(&ev.tag, n);
	    swapl(&ev.tagType, n);
	}
	DBG(DBG_CLIENT, (stderr, "Requesting tag %d\n", tag));
	WriteToClient(client, sizeof(xLbxSendTagDataEvent), (char *) &ev);
	LbxForceOutput(proxy);
    }
}

/*
 * keep track of clients stalled waiting for tags to come back from
 * a proxy.  since multiple clinets can be waiting for the same tag,
 * we have to keep a list of all of them.
 */

typedef struct _sendtagq {
    XID         tag;
    int         num_stalled;
    ClientPtr  *stalled_clients;
    struct _sendtagq *next;
}           SendTagQRec, *SendTagQPtr;

static SendTagQPtr queried_tags = NULL;

#define	LbxSendTagFailed	-1
#define	LbxSendTagSendIt	0
#define	LbxSendTagAlreadySent	1

static Bool
LbxQueueSendTag(ClientPtr   client,
		XID         tag)
{
    SendTagQPtr stqp, *prev, new;
    ClientPtr  *newlist;


    /* see if we're asking for one already in the pipeline */
    for (prev = &queried_tags; (stqp = *prev); prev = &stqp->next) {
	if (stqp->tag == tag) {
	    /* add new client to list */
	    newlist = (ClientPtr *) xrealloc(stqp->stalled_clients,
			      (sizeof(ClientPtr) * (stqp->num_stalled + 1)));
	    if (!newlist)
		return LbxSendTagFailed;
	    newlist[stqp->num_stalled++] = client;
	    stqp->stalled_clients = newlist;
	    DBG(DBG_CLIENT, (stderr, "Additional client requesting tag %d\n", tag));
	    return LbxSendTagAlreadySent;
	}
    }

    /* make new one */
    new = (SendTagQPtr) xalloc(sizeof(SendTagQRec));
    newlist = (ClientPtr *) xalloc(sizeof(ClientPtr));
    if (!new || !newlist) {
	xfree(new);
	xfree(newlist);
	return LbxSendTagFailed;
    }
    *newlist = client;
    new->stalled_clients = newlist;
    new->num_stalled = 1;
    new->tag = tag;
    new->next = NULL;

    /* stick on end of list */
    *prev = new;
    return LbxSendTagSendIt;
}

static SendTagQPtr
LbxFindQTag(XID tag)
{
    SendTagQPtr stqp;

    for (stqp = queried_tags; stqp; stqp = stqp->next) {
	if (stqp->tag == tag)
	    return stqp;
    }
    return NULL;
}

static void
LbxFreeQTag(SendTagQPtr stqp)
{
    xfree(stqp->stalled_clients);
    xfree(stqp);
}

static void
LbxRemoveQTag(XID	tag)
{
    SendTagQPtr stqp, *prev;

    for (prev = &queried_tags; (stqp = *prev); prev = &stqp->next) {
	if (stqp->tag == tag) {
	    *prev = stqp->next;
	    LbxFreeQTag(stqp);
	    return;
	}
    }
}

Bool
LbxFlushQTag(XID tag)
{
    SendTagQPtr stqp;
    ClientPtr *cp;

    stqp = LbxFindQTag(tag);
    if (!stqp)
	return FALSE;
    for (cp = stqp->stalled_clients; --stqp->num_stalled >= 0; cp++)
	AttendClient(*cp);
    LbxRemoveQTag(tag);
    return TRUE;
}

void
ProcessQTagZombies(void)
{
    SendTagQPtr stqp;
    ClientPtr *out, *in;
    int i;

    for (stqp = queried_tags; stqp; stqp = stqp->next) {
	out = stqp->stalled_clients;
	for (in = out, i = stqp->num_stalled; --i >= 0; in++) {
	    if ((*in)->clientGone)
		--stqp->num_stalled;
	    else
		*out++ = *in;
	}
    }
}

/*
 * server sends this
 */

void
LbxQueryTagData(ClientPtr   client,
		int         owner_pid,
		XID         tag,
		int         tagtype)
{
    /* save the info and the client being stalled */
    if (LbxQueueSendTag(client, tag) == LbxSendTagSendIt)
	LbxSendSendTagData(owner_pid, tag, tagtype);
}

/*
 * server recieves this
 */
int
LbxTagData(ClientPtr     client,
	   XID           tag,
	   unsigned long len,
	   pointer       data)
{
    TagData     td;
    PropertyPtr pProp;

    td = TagGetTag(tag);
    if (!td || td->data_type != LbxTagTypeProperty)
	return Success;
    if (!td->global) {
	/* somebody changed contents while we were querying */
	TagDeleteTag(tag);
	return Success;
    }
    LbxFlushQTag(tag);
    pProp = (PropertyPtr) td->tdata;
    if (pProp->tag_id != tag || pProp->owner_pid != LbxProxyID(client))
	return Success;
    pProp->owner_pid = 0;
    if (len != td->size)
	pProp->size = len / (pProp->format >> 3);
    pProp->data = xrealloc(pProp->data, len);
    if (!pProp->data) {
	pProp->size = 0;
	return Success;
    }
    if (client->swapped) {
	switch (pProp->format) {
	case 32:
	    SwapLongs((CARD32 *) data, len >> 2);
	    break;
	case 16:
	    SwapShorts((short *) data, len >> 1);
	    break;
	default:
	    break;
	}
    }
    memmove((char *) pProp->data, (char *) data, len);
    return Success;
}

/* when server resets, need to reset global tags */
void
LbxResetTags(void)
{
    SendTagQPtr stqp;

    modifier_map_tag = 0;
    keyboard_map_tag = 0;

    /* clean out any pending tag requests */
    while ((stqp = queried_tags)) {
	queried_tags = stqp->next;
	LbxFreeQTag(stqp);
    }
}
