/* $Xorg: lbxserve.h,v 1.4 2001/02/09 02:05:17 xorgcvs Exp $ */
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

#ifndef _LBXSERVE_H_
#define _LBXSERVE_H_
#include "lbxdeltastr.h"
#define _XLBX_SERVER_
#include "lbxstr.h"
#include "lbxopts.h"

#define MAX_LBX_CLIENTS	MAXCLIENTS
#define	MAX_NUM_PROXIES	(MAXCLIENTS >> 1)

typedef struct _LbxClient *LbxClientPtr;
typedef struct _LbxProxy *LbxProxyPtr;

typedef struct _LbxClient {
    CARD32	id;
    ClientPtr   client;
    LbxProxyPtr proxy;
    Bool	ignored;
    Bool        input_blocked;
    int         reqs_pending;
    long	bytes_in_reply;
    long	bytes_remaining;
    Drawable	drawableCache[GFX_CACHE_SIZE];
    GContext	gcontextCache[GFX_CACHE_SIZE];
    pointer	gfx_buffer;	/* tmp buffer for unpacking gfx requests */
    unsigned long	gb_size;
}           LbxClientRec;

typedef struct _connectionOutput *OSBufPtr;

typedef struct _LbxProxy {
    LbxProxyPtr next;
    /* this array is indexed by lbx proxy index */
    LbxClientPtr lbxClients[MAX_LBX_CLIENTS];
    LbxClientPtr curRecv,
                curDix;
    int         fd;
    int         pid;		/* proxy ID */
    int		uid;
    int         numClients;
    int		maxIndex;
    Bool        aborted;
    int		grabClient;
    pointer	compHandle;
    Bool        dosquishing;
    Bool        useTags;
    LBXDeltasRec indeltas;
    LBXDeltasRec outdeltas;
    char	*iDeltaBuf;
    char	*replyBuf;
    char	*oDeltaBuf;
    OSBufPtr    ofirst;
    OSBufPtr    olast;
    CARD32      cur_send_id;

    LbxStreamOpts streamOpts;

    int		numBitmapCompMethods;
    char	*bitmapCompMethods;   /* array of indices */
    int		numPixmapCompMethods;
    char	*pixmapCompMethods;   /* array of indices */
    int		**pixmapCompDepths;   /* depths supported from each method */

    struct _ColormapRec *grabbedCmaps; /* chained via lbx private */
    int		motion_allowed_events;
    lbxMotionCache motionCache;
}           LbxProxyRec;

/* This array is indexed by server client index, not lbx proxy index */

extern LbxClientPtr lbxClients[MAXCLIENTS];

#define LbxClient(client)   (lbxClients[(client)->index])
#define LbxProxy(client)    (LbxClient(client)->proxy)
#define LbxMaybeProxy(client)	(LbxClient(client) ? LbxProxy(client) : 0)
#define	LbxProxyID(client)  (LbxProxy(client)->pid)
#define LbxProxyClient(proxy) ((proxy)->lbxClients[0]->client)

extern int LbxEventCode;

extern void LbxDixInit(
#if NeedFunctionPrototypes
    void
#endif
);

extern LbxProxyPtr LbxPidToProxy(
#if NeedFunctionPrototypes
    int /* pid */
#endif
);

extern void LbxReencodeOutput(
#if NeedFunctionPrototypes
    ClientPtr /*client*/,
    char* /*pbuf*/,
    int* /*pcount*/,
    char* /*cbuf*/,
    int* /*ccount*/
#endif
);

extern int UncompressedWriteToClient(
#if NeedFunctionPrototypes
    ClientPtr /*who*/,
    int /*count*/,
    char* /*buf*/
#endif
);

extern ClientPtr AllocLbxClientConnection(
#if NeedFunctionPrototypes
    ClientPtr /* client */,
    LbxProxyPtr /* proxy */
#endif
);

extern void LbxProxyConnection(
#if NeedFunctionPrototypes
    ClientPtr /* client */,
    LbxProxyPtr /* proxy */
#endif
);

#endif				/* _LBXSERVE_H_ */
