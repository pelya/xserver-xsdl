/* $Xorg: lbxtags.c,v 1.4 2001/02/09 02:05:17 xorgcvs Exp $ */
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
/* $XFree86: xc/programs/Xserver/lbx/lbxtags.c,v 1.4 2001/12/14 20:00:01 dawes Exp $ */

#include "X.h"
#include "misc.h"
#include "lbxdata.h"
#include "resource.h"
#include "colormapst.h"
#include "propertyst.h"
#include "lbxtags.h"
#define _XLBX_SERVER_
#include "lbxstr.h"
#include "propertyst.h"

static RESTYPE TagResType;

extern int _lbx_fi_junklen;

/* ARGSUSED */
static int
tag_free(pointer     data,
	 XID         id)
{
    TagData     td = (TagData) data;
    FontTagInfoPtr	ftip;
    char *t;

    if (td->global)
	*(td->global) = 0;
    /* some types need to be freed, others are shared */
    if (td->data_type == LbxTagTypeFont) {
	/* remove any back links */
	ftip = (FontTagInfoPtr) td->tdata;
	FontSetPrivate(ftip->pfont, lbx_font_private, NULL);
	t = (char *) ftip->fontinfo;
	if (!ftip->compression)	/* points to xQueryFont, so back up to it */
	    t -= _lbx_fi_junklen;
	xfree(t);
	xfree(ftip);
    }
    xfree(data);
    return 0;
}

void
TagInit(void)
{
    TagResType = CreateNewResourceType(tag_free);
}

XID
TagNewTag(void)
{
    return FakeClientID(0);
}

void
TagClearProxy(XID         tid,
	      int         pid)
{
    TagData     td;

    td = (TagData) LookupIDByType(tid, TagResType);
    if (td)
	td->sent_to_proxy[pid >> 3] &= ~(1 << (pid & 7));
}

void
TagMarkProxy(XID         tid,
	     int         pid)
{
    TagData     td;

    td = (TagData) LookupIDByType(tid, TagResType);
    td->sent_to_proxy[pid >> 3] |= 1 << (pid & 7);
}

Bool
TagProxyMarked(XID         tid,
	       int         pid)
{
    TagData     td;

    td = (TagData) LookupIDByType(tid, TagResType);
    return (td->sent_to_proxy[pid >> 3] & (1 << (pid & 7))) != 0;
}

XID
TagSaveTag(int         dtype,
	   int         size,
	   pointer     data,
	   XID         *global)
{
    TagData     td;

    td = (TagData) xalloc(sizeof(TagDataRec));
    if (!td) {
	if (global)
	    *global = 0;
	return 0;
    }
    bzero((char *) td->sent_to_proxy, (MAX_NUM_PROXIES + 7) / 8);
    td->tid = TagNewTag();
    td->data_type = dtype;
    td->tdata = data;
    td->size = size;
    td->global = global;
    if (!AddResource(td->tid, TagResType, (pointer) td))
	return 0;
    if (global)
	*global = td->tid;
    return td->tid;
}

void
TagDeleteTag(XID         tid)
{
    int		pid;
    TagData     td;
    LbxProxyPtr proxy;
    ClientPtr   client;
    LbxClientPtr lbxcp;

    td = (TagData) LookupIDByType(tid, TagResType);
    if (!td) /* shouldn't happen, but play it safe */
	return;
    for (pid = 1; pid < MAX_NUM_PROXIES; pid++) {
	if (td->sent_to_proxy[pid >> 3] & (1 << (pid & 7))) {
	    proxy = LbxPidToProxy(pid);
            lbxcp = (proxy != NULL) ? proxy->lbxClients[0] : NULL;
	    if (lbxcp && (client = lbxcp->client))
		LbxSendInvalidateTag(client, tid, td->data_type);
	    td->sent_to_proxy[pid >> 3] &= ~(1 << (pid & 7));
	}
    }
    if (td->data_type != LbxTagTypeProperty || !LbxFlushQTag(tid))
	FreeResource(tid, 0);
    else if (td->global) {
	*(td->global) = 0;
	td->global = NULL;
    }
}

TagData
TagGetTag(XID         tid)
{
    TagData     td;

    td = (TagData) LookupIDByType(tid, TagResType);
    return td;
}

static void
LbxFlushTag(pointer value,
	    XID tid,
	    pointer cdata)
{
    TagData td = (TagData)value;
    LbxProxyPtr proxy = (LbxProxyPtr)cdata;
    int i;

    if ((td->data_type == LbxTagTypeProperty) && td->global) {
	PropertyPtr pProp = (PropertyPtr)td->tdata;
	if ((pProp->tag_id == tid) && (pProp->owner_pid == proxy->pid)) {
	    LbxFlushQTag(tid);
	    pProp->size = 0;
	    FreeResource(tid, 0);
	    return;
	}
    }
    td->sent_to_proxy[proxy->pid >> 3] &= ~(1 << (proxy->pid & 7));
    for (i = 0; i < (MAX_NUM_PROXIES + 7) / 8; i++) {
	if (td->sent_to_proxy[i])
	    return;
    }
    FreeResource(tid, 0);
}

/*
 * clear out markers for proxies
 */
void
LbxFlushTags(LbxProxyPtr proxy)
{
    FindClientResourcesByType(NULL, TagResType, LbxFlushTag, (pointer)proxy);
}
