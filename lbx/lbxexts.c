/* $Xorg: lbxexts.c,v 1.3 2000/08/17 19:53:31 cpqbld Exp $ */
/*
 * Copyright 1994 Network Computing Devices, Inc.
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
/* $XFree86: xc/programs/Xserver/lbx/lbxexts.c,v 1.4 2001/02/16 13:24:10 eich Exp $ */

#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "dixstruct.h"
#include "colormapst.h"
#include "propertyst.h"
#define _XLBX_SERVER_
#include "lbxstr.h"
#include "lbxserve.h"
#ifdef XCSECURITY
#define _SECURITY_SERVER
#include "extensions/security.h"
#endif

typedef struct _lbxext {
    char       *name;
    char      **aliases;
    int         num_aliases;
    int         idx;
    int         opcode;
    int         ev_base;
    int         err_base;
    int         num_reqs;
    CARD8      *rep_mask;
    CARD8      *ev_mask;
#ifdef XCSECURITY
    Bool	secure;
#endif
}           LbxExtensionEntry;

static LbxExtensionEntry **lbx_extensions = NULL;
static int  num_exts = 0;


Bool
LbxAddExtension(char       *name,
		int         opcode,
		int         ev_base,
                int	    err_base)
{
    int         i;
    LbxExtensionEntry *ext,
	**newexts;

    ext = (LbxExtensionEntry *) xalloc(sizeof(LbxExtensionEntry));
    if (!ext)
	return FALSE;
    ext->name = (char *) xalloc(strlen(name) + 1);
    ext->num_aliases = 0;
    ext->aliases = (char **) NULL;
    if (!ext->name) {
	xfree(ext);
	return FALSE;
    }
    strcpy(ext->name, name);
    i = num_exts;
    newexts = (LbxExtensionEntry **) xrealloc(lbx_extensions,
				      (i + 1) * sizeof(LbxExtensionEntry *));
    if (!newexts) {
	xfree(ext->name);
	xfree(ext);
	return FALSE;
    }
    num_exts++;
    lbx_extensions = newexts;
    lbx_extensions[i] = ext;
    ext->idx = i;

    ext->opcode = opcode;;
    ext->ev_base = ev_base;;
    ext->err_base = err_base;
    ext->ev_mask = NULL;
    ext->rep_mask = NULL;
    ext->num_reqs = 0;
#ifdef XCSECURITY
    ext->secure = FALSE;
#endif

    return TRUE;
}

Bool
LbxAddExtensionAlias(int         idx,
		     char       *alias)
{
    char       *name;
    char      **aliases;
    LbxExtensionEntry *ext = lbx_extensions[idx];

    aliases = (char **) xrealloc(ext->aliases,
				 (ext->num_aliases + 1) * sizeof(char *));
    if (!aliases)
	return FALSE;
    ext->aliases = aliases;
    name = (char *) xalloc(strlen(alias) + 1);
    if (!name)
	return FALSE;
    strcpy(name, alias);
    ext->aliases[ext->num_aliases] = name;
    ext->num_aliases++;
    return TRUE;
}

static int
LbxFindExtension(char *extname,
		 int len)
{
    int i, j;

    for (i = 0; i < num_exts; i++) {
	if ((strlen(lbx_extensions[i]->name) == len) &&
		(strncmp(lbx_extensions[i]->name, extname, len) == 0))
	    return i;
	for (j = lbx_extensions[i]->num_aliases; --j >= 0;) {
	    if ((strlen(lbx_extensions[i]->aliases[j]) == len) &&
		(strncmp(lbx_extensions[i]->aliases[j], extname, len) == 0))
		return i;
	}
    }
    return -1;
}

void
LbxDeclareExtensionSecurity(char *extname,
			    Bool secure)
{
#ifdef XCSECURITY
    int i = LbxFindExtension(extname, strlen(extname));
    if (i >= 0)
	lbx_extensions[i]->secure = secure;
#endif
}

Bool
LbxRegisterExtensionGenerationMasks(int         idx,
				    int         num_reqs,
				    char       *rep_mask,
				    char       *ev_mask)
{
    LbxExtensionEntry *ext = lbx_extensions[idx];
    CARD8      *nrm,
               *nem;
    int         mlen = mlen = num_reqs / (8 * sizeof(CARD8)) + 1;

    nrm = (CARD8 *) xalloc(sizeof(CARD8) * mlen);
    nem = (CARD8 *) xalloc(sizeof(CARD8) * mlen);

    if (!nrm || !nem) {
	xfree(nrm);
	xfree(nem);
	return FALSE;
    }
    memcpy((char *) nrm, (char *) rep_mask, mlen);
    memcpy((char *) nem, (char *) ev_mask, mlen);
    ext->rep_mask = nrm;
    ext->ev_mask = nem;
    ext->num_reqs = num_reqs;

    return TRUE;
}

int
LbxQueryExtension(ClientPtr   client,
		  char       *ename,
		  int         nlen)
{
    xLbxQueryExtensionReply rep;
    int         i;
    int         mlen = 0;

    rep.type = X_Reply;
    rep.sequenceNumber = client->sequence;
    rep.major_opcode = 0;
    rep.present = FALSE;
    rep.length = 0;
    rep.pad0 = rep.pad1 = rep.pad2 = rep.pad3 = rep.pad4 = 0;

    i = LbxFindExtension(ename, nlen);

    if (i < 0
#ifdef XCSECURITY
	    /* don't show insecure extensions to untrusted clients */
	    || (client->trustLevel == XSecurityClientUntrusted &&
		!lbx_extensions[i]->secure)
#endif
	)
	rep.present = FALSE;
    else {
	rep.present = TRUE;
	rep.major_opcode = lbx_extensions[i]->opcode;
	rep.first_event = lbx_extensions[i]->ev_base;
	rep.first_error = lbx_extensions[i]->err_base;
	rep.numReqs = lbx_extensions[i]->num_reqs;
	if (lbx_extensions[i]->rep_mask) {
	    mlen = (lbx_extensions[i]->num_reqs + 7) >> 3;
	    rep.length = ((mlen + 3) >> 2) * 2;
	}
    }
    if (client->swapped) {
    	char	n;

	swaps(&rep.sequenceNumber, n);
	swapl(&rep.length, n);
    }
    WriteToClient(client, sizeof(xLbxQueryExtensionReply), (char *)&rep);
    if (mlen) {
	WriteToClient(client, mlen, (char *)lbx_extensions[i]->rep_mask);
	WriteToClient(client, mlen, (char *)lbx_extensions[i]->ev_mask);
    }
    return Success;
}

void
LbxCloseDownExtensions(void)
{
    int         i,j;

    for (i = 0; i < num_exts; i++) {
	xfree(lbx_extensions[i]->name);
	for (j = 0; j < lbx_extensions[i]->num_aliases; j++)
	  xfree(lbx_extensions[i]->aliases[j]);
	xfree(lbx_extensions[i]->aliases);
	xfree(lbx_extensions[i]->rep_mask);
	xfree(lbx_extensions[i]->ev_mask);
	xfree(lbx_extensions[i]);
    }
    xfree(lbx_extensions);
    lbx_extensions = NULL;
    num_exts = 0;

}

void
LbxSetReqMask(CARD8      *mask,
	      int         req,
	      Bool        on)
{
    int         mword = req / (8 * sizeof(CARD8));

    req = req % (8 * sizeof(CARD8));
    if (on) {
	mask[mword] |= (1 << req);
    } else {
	mask[mword] &= ~(1 << req);
    }
}
