/* $Xorg: lbxgfx.c,v 1.3 2000/08/17 19:53:31 cpqbld Exp $ */
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
/* $XFree86: xc/programs/Xserver/lbx/lbxgfx.c,v 1.4 2001/08/23 14:46:57 alanh Exp $ */

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
#include "servermd.h"
#include "windowstr.h"
#include "scrnintstr.h"
#define _XLBX_SERVER_
#include "lbxstr.h"
#include "lbxserve.h"
#include "lbxtags.h"
#include "Xfuncproto.h"
#include "lbximage.h"
#include "lbxsrvopts.h"

#define DrawableCache(client)	(LbxClient(client)->drawableCache)
#define GContextCache(client)	(LbxClient(client)->gcontextCache)

static void
push (XID	    cache[GFX_CACHE_SIZE],
      XID	    xid)
{
    memmove (cache+1, cache, (GFX_CACHE_SIZE - 1) * sizeof (cache[0]));
    cache[0] = xid;
}

static XID
use (XID	    cache[GFX_CACHE_SIZE],
     int	    i)
{
    XID	tmp;

    tmp = cache[i];
    if (i != 0)
    {
	memmove (cache + 1, cache, i * sizeof (cache[0]));
	cache[0] = tmp;
    }
    return tmp;
}

extern char *ConnectionInfo;

static int
LbxDecodeGFXCache(ClientPtr	client,
		  CARD8		cacheEnts,
		  char		*after,
		  Drawable	*drawp,
		  GContext	*gcp)
{
    int	skip;
    int	dcache, gcache;
    
    dcache = GFXdCacheEnt (cacheEnts);
    gcache = GFXgCacheEnt (cacheEnts);
    skip = 0;
    if (dcache == GFXCacheNone)
    {
	memcpy (drawp, after, sizeof (Drawable));
	push (DrawableCache(client), *drawp);
	after += sizeof (Drawable);
	skip += sizeof (Drawable);
    }
    else
	*drawp = use (DrawableCache(client), dcache);
    if (gcache == GFXCacheNone)
    {
	memcpy (gcp, after, sizeof (GContext));
	push (GContextCache(client), *gcp);
	skip += sizeof (GContext);
    }
    else
	*gcp = use (GContextCache(client), gcache);
    return skip;
}

static int
LbxDecodeDrawableCache(ClientPtr	client,
		       CARD8		cacheEnts,
		       char		*after,
		       Drawable		*drawp)
{
    int	skip;
    int	dcache;
    
    dcache = GFXdCacheEnt (cacheEnts);
    skip = 0;
    if (dcache == GFXCacheNone)
    {
	memcpy (drawp, after, sizeof (Drawable));
	push (DrawableCache(client), *drawp);
	after += sizeof (Drawable);
	skip += sizeof (Drawable);
    }
    else
	*drawp = use (DrawableCache(client), dcache);
    return skip;
}

#ifdef notyet
static int
LbxDecodeGCCache(ClientPtr	client,
		 CARD8		cacheEnts,
		 char		*after,
		 GContext	*gcp)
{
    int	skip;
    int	gcache;
    
    gcache = GFXgCacheEnt (cacheEnts);
    skip = 0;
    if (gcache == GFXCacheNone)
    {
	memcpy (gcp, after, sizeof (GContext));
	push (GContextCache(client), *gcp);
	after += sizeof (GContext);
	skip += sizeof (GContext);
    }
    else
	*gcp = use (GContextCache(client), gcache);
    return skip;
}
#endif

#define GFX_GET_DRAWABLE_AND_GC(type,in,len) {\
    int	    skip;   \
    len = (client->req_len << 2) - SIZEOF(type); \
    in = ((char *) stuff) + SIZEOF(type);\
    skip = LbxDecodeGFXCache(client, stuff->cacheEnts, in, \
			     &drawable, &gc); \
    in += skip; \
    len -= skip; \
}
    
#define GFX_GET_DST_DRAWABLE_AND_GC(type,in,len) {\
    int	    skip;   \
    len = (client->req_len << 2) - SIZEOF(type); \
    in = ((char *) stuff) + SIZEOF(type);\
    skip = LbxDecodeGFXCache(client, stuff->cacheEnts, in, \
			     &dstDrawable, &gc); \
    in += skip; \
    len -= skip; \
}
    
#define GFX_GET_SRC_DST_DRAWABLE_AND_GC(type, in, len) { \
    int	    skip; \
    len = (client->req_len << 2) - SIZEOF(type); \
    in = ((char *) stuff) + SIZEOF(type); \
    skip = LbxDecodeDrawableCache(client, stuff->srcCache, in, \
				  &srcDrawable); \
    in += skip; \
    len -= skip; \
    skip = LbxDecodeGFXCache(client, stuff->cacheEnts, in, \
			     &dstDrawable, &gc); \
    in += skip; \
    len -= skip; \
}

#define GFX_GET_GC(type, in, len) { \
    int	    skip; \
    len = (client->req_len << 2) - SIZEOF(type); \
    in = ((char *) stuff) + SIZEOF(type); \
    skip = LbxDecodeGCCache(client, stuff->gcCache, in, &gc); \
    in += skip; \
    len -= skip; \
}

int
LbxDecodePoly(ClientPtr client,
	      CARD8	xreqtype,
	      int 	(*decode_rtn)(char *, char *, short *))
{
    REQUEST(xLbxPolyPointReq);
    char		*in;
    xPolyPointReq	*xreq;
    int			len;
    int			retval;
    Drawable		drawable;
    GContext		gc;

    GFX_GET_DRAWABLE_AND_GC(xLbxPolyPointReq, in, len);
    if ((xreq = (xPolyPointReq *) 
	    xalloc(sizeof(xPolyPointReq) + (len << 1))) == NULL)
	return BadAlloc;
    len = (*decode_rtn)(in, in + len - stuff->padBytes, (short *)(&xreq[1]));
    xreq->reqType = xreqtype;
    xreq->coordMode = 1;
    xreq->drawable = drawable;
    xreq->gc = gc;
    xreq->length = client->req_len = (sizeof(xPolyPointReq) + len) >> 2;
    client->requestBuffer = (pointer)xreq;

    retval = (*ProcVector[xreqtype])(client);
    xfree(xreq);
    return retval;
}

int
LbxDecodeFillPoly(ClientPtr  client)
{
    REQUEST(xLbxFillPolyReq);
    char		*in;
    xFillPolyReq	*xreq;
    int			len;
    int			retval;
    Drawable		drawable;
    GContext		gc;
    
    GFX_GET_DRAWABLE_AND_GC(xLbxFillPolyReq, in, len);
    if ((xreq = (xFillPolyReq *) 
	    xalloc(sizeof(xFillPolyReq) + (len << 1))) == NULL)
	return BadAlloc;
    len = LbxDecodePoints(in, in + len - stuff->padBytes, (short *) &xreq[1]);
    xreq->reqType = X_FillPoly;
    xreq->drawable = drawable;
    xreq->gc = gc;
    xreq->shape = stuff->shape;
    xreq->coordMode = 1;
    xreq->length = client->req_len = (sizeof(xFillPolyReq) + len) >> 2;
    client->requestBuffer = (pointer)xreq;

    retval = (*ProcVector[X_FillPoly])(client);
    xfree(xreq);
    return retval;
}

/*
 * Routines for decoding line drawing requests
 */

#define DECODE_PSHORT(in, val) \
    if ((*(in) & 0xf0) != 0xf0) \
	(val) = *(CARD8 *)(in)++; \
    else { \
	(val) = ((*(CARD8 *)(in) & 0x0f) << 8) | *(CARD8 *)((in) + 1); \
	if ((val) >= 0xe00) \
	    (val) -= 0x1000; \
	else \
	    (val) += 0xf0; \
	(in) += 2; \
    }

#define DECODE_SHORT(in, val) \
    if ((*(in) & 0xf0) != 0x80) \
        (val) = *(INT8 *)(in)++; \
    else { \
	(val) = ((*(CARD8 *)(in) & 0x0f) << 8) | *(CARD8 *)((in) + 1); \
	if ((val) & 0x0800) \
	    (val) = ((val) | 0xf000) - 0x70; \
	else \
	    (val) += 0x80; \
	(in) += 2; \
    }

#define DECODE_USHORT(in, val) \
    if ((*(in) & 0xf0) != 0xf0) \
	(val) = *(CARD8 *)(in)++; \
    else { \
	(val) = (((*(CARD8 *)(in) & 0x0f) << 8) | *(CARD8 *)((in) + 1)) + 0xf0; \
	(in) += 2; \
    }

#define DECODE_ANGLE(in, val) \
    if (*(INT8 *)(in) >= 0x6e) \
	(val) = (*(INT8 *)(in)++ - 0x67) * (15 << 6); \
    else if (*(INT8 *)(in) >= 0x5a) \
	(val) = (*(INT8 *)(in)++ - 0x5a) * (5 << 6); \
    else if (*(INT8 *)(in) <= (INT8)0x91) \
	(val) = (*(INT8 *)(in)++ - (INT8)0x98) * (15 << 6); \
    else if (*(INT8 *)(in) <= (INT8)0xa5) \
	(val) = (*(INT8 *)(in)++ - (INT8)0xa6) * (5 << 6); \
    else { \
	(val) = (*(CARD8 *)(in) << 8) | *(CARD8 *)((in) + 1); \
	(in) += 2; \
    }

int
LbxDecodePoints(char  	*in,
		char	*inend,
		short 	*out)
{
    char	   *start_out = (char *)out;

    while (in < inend) {
	DECODE_SHORT(in, *out);
	out++;
	DECODE_SHORT(in, *out);
	out++;
    }
    return ((char *)out - start_out);
}

int
LbxDecodeSegment(char	*in,
		 char	*inend,
		 short	*out)
{
    short 	   diff;
    short	   last_x = 0;
    short	   last_y = 0;
    char	   *start_out = (char *)out;

    while (in < inend) {
	DECODE_SHORT(in, diff);
	*out = last_x + diff;
	last_x += diff;
	out++;
	DECODE_SHORT(in, diff);
	*out = last_y + diff;
	last_y += diff;
	out++;

	DECODE_SHORT(in, diff);
	*out = last_x + diff;
	out++;
	DECODE_SHORT(in, diff);
	*out = last_y + diff;
	out++;
    }
    return ((char *)out - start_out);
}

int
LbxDecodeRectangle(char	 *in,
		   char	 *inend,
		   short *out)
{
    short	   diff;
    short	   last_x = 0;
    short	   last_y = 0;
    char	   *start_out = (char *)out;

    while (in < inend) {
	DECODE_SHORT(in, diff);
	*out = last_x + diff;
	last_x += diff;
	out++;
	DECODE_SHORT(in, diff);
	*out = last_y + diff;
	last_y += diff;
	out++;

	DECODE_USHORT(in, *(unsigned short *)out);
	out++;
	DECODE_USHORT(in, *(unsigned short *)out);
	out++;
    }
    return ((char *)out - start_out);
}

int
LbxDecodeArc(char  *in,
	     char  *inend,
	     short *out)
{
    short 	   diff;
    short	   last_x = 0;
    short	   last_y = 0;
    char	   *start_out = (char *)out;

    while (in < inend) {
	DECODE_SHORT(in, diff);
	*out = last_x + diff;
	last_x += diff;
	out++;
	DECODE_SHORT(in, diff);
	*out = last_y + diff;
	last_y += diff;
	out++;

	DECODE_USHORT(in, *(unsigned short *)out);
	out++;
	DECODE_USHORT(in, *(unsigned short *)out);
	out++;

	DECODE_ANGLE(in, *out);
	out++;
	DECODE_ANGLE(in, *out);
	out++;
    }
    return ((char *)out - start_out);
}

int
LbxDecodeCopyArea (ClientPtr	client)
{
    REQUEST(xLbxCopyAreaReq);
    char		*in;
    xCopyAreaReq    	req;
    int			len;
    Drawable		srcDrawable, dstDrawable;
    GContext		gc;
    
    GFX_GET_SRC_DST_DRAWABLE_AND_GC(xLbxCopyAreaReq, in, len);
    req.reqType = X_CopyArea;
    req.length = client->req_len = SIZEOF(xCopyAreaReq) >> 2;
    req.srcDrawable = srcDrawable;
    req.dstDrawable = dstDrawable;
    req.gc = gc;
    DECODE_PSHORT (in, req.srcX);
    DECODE_PSHORT (in, req.srcY);
    DECODE_PSHORT (in, req.dstX);
    DECODE_PSHORT (in, req.dstY);
    DECODE_USHORT (in, req.width);
    DECODE_USHORT (in, req.height);
    client->requestBuffer = (pointer) &req;
    return (*ProcVector[X_CopyArea])(client);
}

int
LbxDecodeCopyPlane (ClientPtr	client)
{
    REQUEST(xLbxCopyPlaneReq);
    char		*in;
    xCopyPlaneReq 	req;
    int			len;
    Drawable		srcDrawable, dstDrawable;
    GContext		gc;
    
    GFX_GET_SRC_DST_DRAWABLE_AND_GC(xLbxCopyPlaneReq, in, len);
    req.reqType = X_CopyPlane;
    req.length = client->req_len = SIZEOF(xCopyPlaneReq) >> 2;
    req.srcDrawable = srcDrawable;
    req.dstDrawable = dstDrawable;
    req.gc = gc;
    DECODE_PSHORT (in, req.srcX);
    DECODE_PSHORT (in, req.srcY);
    DECODE_PSHORT (in, req.dstX);
    DECODE_PSHORT (in, req.dstY);
    DECODE_USHORT (in, req.width);
    DECODE_USHORT (in, req.height);
    req.bitPlane = stuff->bitPlane;
    client->requestBuffer = (pointer) &req;
    return (*ProcVector[X_CopyPlane])(client);
}

static pointer
get_gfx_buffer(ClientPtr	client,
	       int		len)
{
    LbxClientPtr    lbxClient = LbxClient(client);
    pointer	tmp;

    /* XXX should probably shrink this sucker too */
    if (len > lbxClient->gb_size) {
	tmp = (pointer) xrealloc(lbxClient->gfx_buffer, len);
	if (!tmp)
	    return (pointer) NULL;
	lbxClient->gfx_buffer = tmp;
	lbxClient->gb_size = len;
    }
    return lbxClient->gfx_buffer;
}

int
LbxDecodePolyText (ClientPtr	client)
{
    REQUEST(xLbxPolyTextReq);
    char		*in, *pos;
    xPolyTextReq	*xreq;
    int			len;
    Drawable		drawable;
    GContext		gc;
    
    GFX_GET_DRAWABLE_AND_GC(xLbxPolyTextReq, in, len);
    xreq = (xPolyTextReq *) get_gfx_buffer(client, sizeof (xPolyTextReq) + len);
    if (!xreq)
	return BadAlloc;
    xreq->reqType = stuff->lbxReqType == X_LbxPolyText8? X_PolyText8 : X_PolyText16;
    xreq->drawable = drawable;
    xreq->gc = gc;
    pos = in;
    DECODE_PSHORT(in, xreq->x);
    DECODE_PSHORT(in, xreq->y);
    len -= (in - pos);
    memmove ((char *) (xreq + 1), in, len);
    xreq->length = client->req_len = (sizeof (xPolyTextReq) + len) >> 2;
    client->requestBuffer = (pointer) xreq;
    return (*ProcVector[xreq->reqType])(client);
}

int
LbxDecodeImageText (ClientPtr	client)
{
    REQUEST(xLbxImageTextReq);
    char		*in, *pos;
    xImageTextReq	*xreq;
    int			len;
    Drawable		drawable;
    GContext		gc;

    GFX_GET_DRAWABLE_AND_GC(xLbxImageTextReq, in, len);
    xreq = (xImageTextReq *) get_gfx_buffer(client, sizeof (xImageTextReq) + len);
    if (!xreq)
	return BadAlloc;
    xreq->reqType = stuff->lbxReqType == X_LbxImageText8? X_ImageText8 : X_ImageText16;
    xreq->drawable = drawable;
    xreq->gc = gc;
    xreq->nChars = stuff->nChars;
    pos = in;
    DECODE_PSHORT(in, xreq->x);
    DECODE_PSHORT(in, xreq->y);
    len -= (in - pos);
    memmove ((char *) (xreq + 1), in, len);
    xreq->length = client->req_len = (sizeof (xImageTextReq) + len) >> 2;
    client->requestBuffer = (pointer) xreq;
    return (*ProcVector[xreq->reqType])(client);
}

int
LbxDecodePutImage (ClientPtr  client)
{
    REQUEST		(xLbxPutImageReq);
    char		*in, *data;
    xPutImageReq	xreq;
    int			ppl, bpl, nbytes;
    int			retval;
    int			n;

    xreq.reqType = X_PutImage;

    in = (char *) stuff + sz_xLbxPutImageReq;

    if (stuff->bitPacked & 0x80) {
	xreq.format = (stuff->bitPacked >> 5) & 0x3;
	xreq.depth = ((stuff->bitPacked >> 2) & 0x7) + 1;
	xreq.leftPad = 0;
    } else {
	xreq.depth = (stuff->bitPacked >> 2) + 1;
	xreq.format = (*in >> 5) & 0x3;
	xreq.leftPad = *in++ & 0x1f;
    }
    DECODE_USHORT(in, xreq.width);
    DECODE_USHORT(in, xreq.height);
    DECODE_PSHORT(in, xreq.dstX);
    DECODE_PSHORT(in, xreq.dstY);
    if (client->swapped) {
	if (GFXdCacheEnt (stuff->cacheEnts) == GFXCacheNone ||
	    GFXgCacheEnt (stuff->cacheEnts) == GFXCacheNone)
	{
	    swapl (in, n);
	    if (GFXdCacheEnt (stuff->cacheEnts) == GFXCacheNone &&
		GFXgCacheEnt (stuff->cacheEnts) == GFXCacheNone)
		swapl (in + 4, n);
	}
    }
    in += LbxDecodeGFXCache(client, stuff->cacheEnts, in,
			    &xreq.drawable, &xreq.gc);

    ppl = xreq.width + xreq.leftPad;
    if (xreq.format != ZPixmap ||
	(xreq.depth == 1 && screenInfo.formats->bitsPerPixel == 1)) {
#ifdef INTERNAL_VS_EXTERNAL_PADDING
	bpl = BitmapBytePadProto(ppl);
#else
	bpl = BitmapBytePad(ppl);
#endif
	nbytes = bpl;
	if (xreq.format == XYPixmap)
	    nbytes *= xreq.depth;
    } else {
#ifdef INTERNAL_VS_EXTERNAL_PADDING
	bpl = PixmapBytePadProto(ppl, xreq.depth);
#else
	bpl = PixmapBytePad(ppl, xreq.depth);
#endif
	nbytes = bpl;
    }
    nbytes *= xreq.height;
    xreq.length = ((nbytes + 3) >> 2) + (sz_xPutImageReq >> 2);
    /* +1 is because fillspan in DecodeFaxG42D seems to go 1 byte too far,
     * and I don't want to mess with that code */
    if ((data = (char *) xalloc ((xreq.length << 2) + 1)) == NULL)
	return BadAlloc;

    *((xPutImageReq *)data) = xreq;

    if (!stuff->compressionMethod)
    {
	memcpy(data + sz_xPutImageReq, in, nbytes);
    }
    else if (xreq.format != ZPixmap ||
	     (xreq.depth == 1 && screenInfo.formats->bitsPerPixel == 1))
    {
	LbxBitmapCompMethod *compMethod;

	compMethod = LbxSrvrLookupBitmapCompMethod (LbxProxy(client),
	    stuff->compressionMethod);

	if (!compMethod)
	{
	    xfree (data);
	    return (BadValue);
	}
	else
	{
	    if (!compMethod->inited)
	    {
		if (compMethod->compInit)
		    (*compMethod->compInit)();
		compMethod->inited = 1;
	    }

	    (*compMethod->decompFunc) (
	        (unsigned char *) in, (unsigned char *) data + sz_xPutImageReq,
	        nbytes,
#if BITMAP_BIT_ORDER != IMAGE_BYTE_ORDER
		(ppl + BITMAP_SCANLINE_UNIT - 1) & ~BITMAP_SCANLINE_UNIT,
#else
	        ppl,
#endif
		bpl,
	        ((xConnSetup *) ConnectionInfo)->bitmapBitOrder == LSBFirst);
	}
    }
    else
    {
	LbxPixmapCompMethod *compMethod;

	compMethod = LbxSrvrLookupPixmapCompMethod (LbxProxy(client),
	    stuff->compressionMethod);

	if (!compMethod)
	{
	    xfree (data);
	    return (BadValue);
	}
	else
	{
	    if (!compMethod->inited)
	    {
		if (compMethod->compInit)
		    (*compMethod->compInit)();
		compMethod->inited = 1;
	    }

	    (*compMethod->decompFunc) (
		in, (char *) data + sz_xPutImageReq,
		(int) xreq.height, bpl);
	}
    }

    client->req_len = xreq.length;
    client->requestBuffer = (pointer) data;

    retval = (*ProcVector[X_PutImage])(client);
    xfree(data);
    return retval;
}

int
LbxDecodeGetImage (ClientPtr  client)
{
    REQUEST		(xLbxGetImageReq);
    xLbxGetImageReply	*reply = NULL;
    int			lbxLen, xLen, n;
    int			method = 0, bytes, status;
    xGetImageReply	*theImage;

    REQUEST_SIZE_MATCH(xLbxGetImageReq);

    status = DoGetImage(client, stuff->format, stuff->drawable,
			stuff->x, stuff->y,
			(int)stuff->width, (int)stuff->height,
			stuff->planeMask, &theImage);

    if (status != Success)
	return (status);

    if ((reply = (xLbxGetImageReply *) xalloc (
	sz_xLbxGetImageReply + theImage->length)) == NULL)
    {
	xfree(theImage);
	return (BadAlloc);
    }

    if (stuff->format != ZPixmap ||
	(theImage->depth == 1 && screenInfo.formats->bitsPerPixel == 1))
    {
	LbxBitmapCompMethod *compMethod;

	compMethod = LbxSrvrFindPreferredBitmapCompMethod (LbxProxy(client));

	if (!compMethod)
	    status = LBX_IMAGE_COMPRESS_NO_SUPPORT;
	else
	{
	    if (!compMethod->inited)
	    {
		if (compMethod->compInit)
		    (*compMethod->compInit)();
		compMethod->inited = 1;
	    }

	    status = (*compMethod->compFunc) (
	        (unsigned char *) &theImage[1],
	        (unsigned char *) &reply[1],
	        theImage->length,
		theImage->length,
#if BITMAP_BIT_ORDER != IMAGE_BYTE_ORDER
		(int) (stuff->width + BITMAP_SCANLINE_UNIT - 1) &
		      ~BITMAP_SCANLINE_UNIT,
#else
		(int) stuff->width,
#endif
#ifdef INTERNAL_VS_EXTERNAL_PADDING
		BitmapBytePadProto(stuff->width),
#else
		BitmapBytePad(stuff->width),
#endif
	        ((xConnSetup *) ConnectionInfo)->bitmapBitOrder == LSBFirst,
	        &bytes);

	    method = compMethod->methodOpCode;
	}
    }
    else
    {
	LbxPixmapCompMethod *compMethod;

	compMethod = LbxSrvrFindPreferredPixmapCompMethod (
	    LbxProxy(client), (int) stuff->format, theImage->depth);

	if (!compMethod)
	    status = LBX_IMAGE_COMPRESS_NO_SUPPORT;
	else
	{
	    if (!compMethod->inited)
	    {
		if (compMethod->compInit)
		    (*compMethod->compInit)();
		compMethod->inited = 1;
	    }

	    status = (*compMethod->compFunc) (
	        (char *) &theImage[1],
	        (char *) &reply[1],
	        theImage->length,
	        (int) stuff->format,
	        theImage->depth,
	        (int) stuff->height,
#ifdef INTERNAL_VS_EXTERNAL_PADDING
		PixmapBytePadProto(stuff->width, theImage->depth),
#else
		PixmapBytePad(stuff->width, theImage->depth),
#endif
		&bytes);

	    method = compMethod->methodOpCode;
	}
    }

    reply->type = X_Reply;
    reply->depth = theImage->depth;
    reply->sequenceNumber = client->sequence;
    reply->visual = theImage->visual;
    reply->pad1 = reply->pad2 = reply->pad3 = reply->pad4 = reply->pad5 = 0;

    if (status != LBX_IMAGE_COMPRESS_SUCCESS)
    {
	reply->compressionMethod = LbxImageCompressNone;
	reply->lbxLength = reply->xLength = (theImage->length + 3) >> 2;
    }
    else
    {
	reply->compressionMethod = method;
	reply->lbxLength = (bytes + 3) >> 2;
	reply->xLength = (theImage->length + 3) >> 2;
    }

    lbxLen = reply->lbxLength;
    xLen = reply->xLength;

    if (client->swapped)
    {
	swaps (&reply->sequenceNumber, n);
	swapl (&reply->lbxLength, n);
	swapl (&reply->xLength, n);
	swapl (&reply->visual, n);
    }

    if (reply->compressionMethod != LbxImageCompressNone)
    {
	/*
	 * If the compressed image is greater that 25% of the original
	 * image, run the GetImage reply through the regular stream
	 * compressor.  Otherwise, just write the compressed image.
	 */
	
	if (lbxLen > (xLen / 4))
	{
	    WriteToClient (client,
	        sz_xLbxGetImageReply + (lbxLen << 2), (char *)reply);
	}
	else
	{
	    UncompressedWriteToClient (client,
	        sz_xLbxGetImageReply + (lbxLen << 2), (char *)reply);
	}
    }
    else
    {
	/*
	 * Write back the original uncompressed image.
	 */

	WriteToClient (client, sz_xLbxGetImageReply, (char *)reply);
	WriteToClient (client, lbxLen << 2, (char *)&theImage[1]);
    }

    xfree (theImage);

    if (reply)
	xfree ((char *) reply);

    return (Success);
}
