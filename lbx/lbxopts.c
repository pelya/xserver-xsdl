/* $Xorg: lbxopts.c,v 1.3 2000/08/17 19:53:31 cpqbld Exp $ */
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
/* $XFree86: xc/programs/Xserver/lbx/lbxopts.c,v 1.6 2001/10/28 03:34:12 tsi Exp $ */

#ifdef OPTDEBUG
#include <stdio.h>
#endif
#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "colormapst.h"
#include "propertyst.h"
#include "lbxserve.h"
#include "lbxstr.h"
#include "lbximage.h"
#include "lbxopts.h"
#include "lbxsrvopts.h"
#ifndef NO_ZLIB
#include "lbxzlib.h"
#endif /* NO_ZLIB */

static int LbxProxyDeltaOpt ( LbxNegOptsPtr pno, unsigned char *popt, 
			      int optlen, unsigned char *preply );
static int LbxServerDeltaOpt ( LbxNegOptsPtr pno, unsigned char *popt, 
			       int optlen, unsigned char *preply );
static int LbxDeltaOpt ( unsigned char *popt, int optlen, 
			 unsigned char *preply, short *pn, short *pmaxlen );
static int LbxStreamCompOpt ( LbxNegOptsPtr pno, unsigned char *popt, 
			      int optlen, unsigned char *preply );
static int ZlibParse ( LbxNegOptsPtr pno, unsigned char *popt, int optlen, 
		       unsigned char *preply );
static int LbxMessageCompOpt ( LbxNegOptsPtr pno, unsigned char *popt, 
			       int optlen, unsigned char *preply );
static int LbxUseTagsOpt ( LbxNegOptsPtr pno, unsigned char *popt, 
				  int optlen, unsigned char *preply );
static int LbxBitmapCompOpt ( LbxNegOptsPtr pno, unsigned char *popt, 
				     int optlen, unsigned char *preply );
static int LbxPixmapCompOpt ( LbxNegOptsPtr pno, unsigned char *popt, 
				     int optlen, unsigned char *preply );
static int MergeDepths ( int *depths, LbxPixmapCompMethod *method );
static int LbxCmapAllOpt ( LbxNegOptsPtr pno, unsigned char *popt, 
				  int optlen, unsigned char *preply );

/*
 * List of LBX options we recognize and are willing to negotiate
 */
static struct _LbxOptionParser {
    CARD8	optcode;
    int		(*parser)(LbxNegOptsPtr, unsigned char *, 
			  int, unsigned char *);
} LbxOptions[] = {
    { LBX_OPT_DELTA_PROXY, 	LbxProxyDeltaOpt },
    { LBX_OPT_DELTA_SERVER,	LbxServerDeltaOpt },
    { LBX_OPT_STREAM_COMP,	LbxStreamCompOpt },
    { LBX_OPT_BITMAP_COMP,	LbxBitmapCompOpt },
    { LBX_OPT_PIXMAP_COMP,	LbxPixmapCompOpt },
    { LBX_OPT_MSG_COMP,		LbxMessageCompOpt },
    { LBX_OPT_USE_TAGS,		LbxUseTagsOpt },
    { LBX_OPT_CMAP_ALL,		LbxCmapAllOpt }
};

#define LBX_N_OPTS	(sizeof(LbxOptions) / sizeof(struct _LbxOptionParser))

/*
 * Set option defaults
 */
void
LbxOptionInit(LbxNegOptsPtr pno)
{
    bzero(pno, sizeof(LbxNegOptsRec));
    pno->proxyDeltaN = pno->serverDeltaN = LBX_OPT_DELTA_NCACHE_DFLT;
    pno->proxyDeltaMaxLen = pno->serverDeltaMaxLen = LBX_OPT_DELTA_MSGLEN_DFLT;
    pno->squish = TRUE;
    pno->numBitmapCompMethods = 0;
    pno->bitmapCompMethods = NULL;
    pno->numPixmapCompMethods = 0;
    pno->pixmapCompMethods = NULL;
    pno->pixmapCompDepths = NULL;
    pno->useTags = TRUE;
}

int
LbxOptionParse(LbxNegOptsPtr  pno,
	       unsigned char *popt,
	       int	      optlen,
	       unsigned char *preply)
{
    int		  i;
    int		  nopts = *popt++;
    unsigned char *pout = preply;

    for (i = 0; i < nopts; i++) {
	int j;
	int len;
	int hdrlen;
	int replylen;

	LBX_OPT_DECODE_LEN(popt + 1, len, hdrlen);
	if (len < ++hdrlen || len > optlen) {
#ifdef OPTDEBUG
	    fprintf(stderr, "bad option length, len = %d, hdrlen = %d, optlen = %d\n", len, hdrlen, optlen);
#endif
	    return -1;
	}

	for (j = 0; j < LBX_N_OPTS; j++) {
	    if (popt[0] == LbxOptions[j].optcode) {
		replylen = (*LbxOptions[j].parser)(pno,
						   popt + hdrlen,
						   len - hdrlen,
						   pout + LBX_OPT_SMALLHDR_LEN);
		if (replylen < 0)
		    return -1;
		else if (replylen > 0) {
		    /*
		     * None of the current options require big headers,
		     * so this works for now.
		     */
		    *pout++ = i;
		    *pout++ = LBX_OPT_SMALLHDR_LEN + replylen;
		    pout += replylen;
		    pno->nopts++;
		}
		break;
	    }
	}

	optlen -= len;
	popt += len;
    }

    return (pout - preply);
}

static int
LbxProxyDeltaOpt(LbxNegOptsPtr  pno,
		 unsigned char *popt,
		 int		optlen,
		 unsigned char *preply)
{
    return LbxDeltaOpt(popt, optlen, preply,
		       &pno->proxyDeltaN, &pno->proxyDeltaMaxLen);
}

static int
LbxServerDeltaOpt(LbxNegOptsPtr  pno,
		  unsigned char *popt,
		  int		 optlen,
		  unsigned char *preply)
{
    return LbxDeltaOpt(popt, optlen, preply,
		       &pno->serverDeltaN, &pno->serverDeltaMaxLen);
}

static int
LbxDeltaOpt(unsigned char *popt,
	    int		   optlen,
	    unsigned char *preply,
	    short	  *pn,
	    short	  *pmaxlen)
{
    short	  n;
    short	  maxlen;

    /*
     * If there's more data than we expect, we just ignore it.
     */
    if (optlen < LBX_OPT_DELTA_REQLEN) {
#ifdef OPTDEBUG
	fprintf(stderr, "bad delta option length = %d\n", optlen);
#endif
	return -1;
    }

    /*
     * Accept whatever value the proxy prefers, so skip the
     * min/max offerings.  Note that the max message len value is
     * encoded as the number of 4-byte values.
     */
    popt += 2;
    n = *popt++;
    popt += 2;
    maxlen = *popt++;
    if ((maxlen <<= 2) == 0)
	n = 0;
    else if (maxlen < 32) {
#ifdef OPTDEBUG
	fprintf(stderr, "bad delta max msg length %d\n", maxlen);
#endif
	return -1;
     }

    /*
     * Put the response in the reply buffer
     */
    *preply++ = n;
    *preply++ = maxlen >> 2;

    *pn = n;
    *pmaxlen = maxlen;

    return LBX_OPT_DELTA_REPLYLEN;
}


static struct _LbxStreamCompParser {
    int		typelen;
    char	*type;
    int		(*parser)(LbxNegOptsPtr, unsigned char *, 
			  int, unsigned char *);
} LbxStreamComp[] = {
#ifndef NO_ZLIB
    { ZLIB_STRCOMP_OPT_LEN,	ZLIB_STRCOMP_OPT, 	ZlibParse },
#endif /* NO_ZLIB */
};

#define LBX_N_STRCOMP	\
    (sizeof(LbxStreamComp) / sizeof(struct _LbxStreamCompParser))

static int
LbxStreamCompOpt(LbxNegOptsPtr  pno,
		 unsigned char *popt,
		 int		optlen,
		 unsigned char *preply)
{
    int		  i;
    int		  typelen;
    int		  nopts = *popt++;

    for (i = 0; i < nopts; i++) {
	int j;
	int len;
	int lensize;
	int replylen;

	typelen = popt[0];
	for (j = 0; j < LBX_N_STRCOMP; j++) {
	    if (typelen == LbxStreamComp[j].typelen &&
		!strncmp((char *) popt + 1, LbxStreamComp[j].type, typelen))
		break;
	}

	popt += 1 + typelen;
	optlen -= 1 + typelen;
	LBX_OPT_DECODE_LEN(popt, len, lensize);

	if (j < LBX_N_STRCOMP) {
	    if (len > optlen)
		return -1;
	    replylen = (*LbxStreamComp[j].parser)(pno,
						  popt + lensize,
						  len - lensize,
						  preply + 1);
	    if (replylen == -1)
		return -1;
	    else if (replylen >= 0) {
		*preply = i;
		return replylen + 1;
	    }
	}

	optlen -= len;
	popt += len;
    }

    return 0;
}


static int
ZlibParse(LbxNegOptsPtr   pno,
	  unsigned char  *popt,
	  int		  optlen,
	  unsigned char  *preply)
{
    int level;		/* compression level */

    if (*popt++ != 1)	/* length should be 1 */
	return (-1);

    level = *popt;
    if (level < 1 || level > 9)
	return (-1);

    pno->streamOpts.streamCompInit =
	(LbxStreamCompHandle (*)(int, pointer))ZlibInit;
    pno->streamOpts.streamCompArg = (pointer)(long)level;
    pno->streamOpts.streamCompStuffInput = ZlibStuffInput;
    pno->streamOpts.streamCompInputAvail = ZlibInputAvail;
    pno->streamOpts.streamCompFlush = ZlibFlush;
    pno->streamOpts.streamCompRead = ZlibRead;
    pno->streamOpts.streamCompWriteV = ZlibWriteV;
    pno->streamOpts.streamCompOn = ZlibCompressOn;
    pno->streamOpts.streamCompOff = ZlibCompressOff;
    pno->streamOpts.streamCompFreeHandle =
	(void (*)(LbxStreamCompHandle))ZlibFree;

    return (0);
}

static int
LbxMessageCompOpt(LbxNegOptsPtr  pno,
		  unsigned char *popt,
		  int		 optlen,
		  unsigned char *preply)
{

    if (optlen == 0) {
#ifdef OPTDEBUG
	fprintf(stderr, "bad message-comp option length specified %d\n", optlen);
#endif
	return -1;
    }

    pno->squish = *preply = *popt;
    return 1;
}


static int
LbxUseTagsOpt(LbxNegOptsPtr  pno,
	      unsigned char *popt,
	      int 	     optlen,
	      unsigned char *preply)
{

    if (optlen == 0) {
#ifdef OPTDEBUG
	fprintf(stderr, "bad use-tags option length specified %d\n", optlen);
#endif
	return -1;
    }

    pno->useTags = *preply = *popt;
    return 1;
}


/*
 * Option negotiation for image compression
 */

LbxBitmapCompMethod
LbxBitmapCompMethods [] = {
  {
    "XC-FaxG42D",		/* compression method name */
    0,				/* inited */
    2,				/* method opcode */
    NULL,			/* init function */
    LbxImageEncodeFaxG42D,	/* encode function */
    LbxImageDecodeFaxG42D	/* decode function */
  }
};

#define NUM_BITMAP_METHODS \
	(sizeof (LbxBitmapCompMethods) / sizeof (LbxBitmapCompMethod))


#if 1
/*
 * Currently, we don't support any pixmap compression algorithms
 * because regular stream compression does much better than PackBits.
 * If we want to plug in a better pixmap image compression algorithm,
 * it would go here.
 */

#define NUM_PIXMAP_METHODS 0
LbxPixmapCompMethod LbxPixmapCompMethods [1]; /* dummy */

#else

LbxPixmapCompMethod
LbxPixmapCompMethods [] = {
  {
    "XC-PackBits",		/* compression method name */
    1 << ZPixmap,		/* formats supported */
    1, {8},			/* depths supported */
    0,				/* inited */
    1,				/* method opcode */
    NULL,			/* init function */
    LbxImageEncodePackBits,	/* encode function */
    LbxImageDecodePackBits	/* decode function */
  }
};

#define NUM_PIXMAP_METHODS \
	(sizeof (LbxPixmapCompMethods) / sizeof (LbxPixmapCompMethod))
#endif


static int
LbxImageCompOpt (Bool	         pixmap,
		 LbxNegOptsPtr   pno,
		 unsigned char  *popt,
		 int	         optlen,
		 unsigned char  *preply)

{
    unsigned char *preplyStart = preply;
    int numMethods = *popt++;
    unsigned char *myIndices, *hisIndices;
    unsigned int *retFormats = NULL;
    int **retDepths = NULL;
    int replyCount = 0;
    int status, i, j;

    if (numMethods == 0)
    {
	if (pixmap)
	    pno->numPixmapCompMethods = 0;
	else
	    pno->numBitmapCompMethods = 0;

	*preply++ = 0;
	return (1);
    }

    myIndices = (unsigned char *) xalloc (numMethods);
    hisIndices = (unsigned char *) xalloc (numMethods);

    if (!myIndices || !hisIndices)
    {
	if (myIndices)
	    xfree (myIndices);
	if (hisIndices)
	    xfree (hisIndices);
	return -1;
    }

    if (pixmap)
    {
	retFormats = (unsigned *) xalloc (numMethods);
	retDepths = (int **) xalloc (numMethods * sizeof (int *));

	if (!retFormats || !retDepths)
	{
	    if (retFormats)
		xfree (retFormats);
	    if (retDepths)
		xfree (retDepths);
	    xfree (myIndices);
	    xfree (hisIndices);
	    return -1;
	}
    }

    /*
     * For each method in the list sent by the proxy, see if the server
     * supports this method.  If YES, update the following lists:
     *
     * myIndices[] is a list of indices into the server's
     * LbxBit[Pix]mapCompMethods table.
     *
     * hisIndices[] is a list of indices into the list of
     * method names sent by the proxy.
     *
     * retFormats[] indicates for each pixmap compression method,
     * the pixmap formats supported.
     *
     * retDepths[] indicates for each pixmap compression method,
     * the pixmap depths supported.
     */

    for (i = 0; i < numMethods; i++)
    {
	unsigned int formatMask = 0, newFormatMask = 0;
	int depthCount, *depths = NULL, len;
	int freeDepths;
	char *methodName;

	freeDepths = 0;
	len = *popt++;
	methodName = (char *) popt;
	popt += len;

	if (pixmap)
	{
	    formatMask = *popt++;
	    depthCount = *popt++;
	    depths = (int *) xalloc ((depthCount + 1) * sizeof (int));
	    freeDepths = 1;
	    depths[0] = depthCount;
	    for (j = 1; j <= depthCount; j++)
		depths[j] = *popt++;
	}

	for (j = 0;
	    j < (pixmap ? NUM_PIXMAP_METHODS : NUM_BITMAP_METHODS); j++)
	{

	    status = strncmp (methodName,
		(pixmap ? LbxPixmapCompMethods[j].methodName :
		          LbxBitmapCompMethods[j].methodName),
		len);

	    if (status == 0 && pixmap)
	    {
		newFormatMask =
		    formatMask & LbxPixmapCompMethods[j].formatMask;

		depthCount = MergeDepths (depths, &LbxPixmapCompMethods[j]);
		
		if (newFormatMask == 0 || depthCount == 0)
		    status = 1;
	    }

	    if (status == 0)
	    {
		myIndices[replyCount] = j;
		hisIndices[replyCount] = i;

		if (pixmap)
		{
		    retFormats[replyCount] = newFormatMask;
		    retDepths[replyCount] = depths;
		    freeDepths = 0;
		}

		replyCount++;
		break;
	    }
	}

	if (freeDepths)
	    xfree (depths);
    }

    *preply++ = replyCount;

    /*
     * Sort the lists by LBX server preference (increasing myIndices[] vals)
     */

    for (i = 0; i <= replyCount - 2; i++)
	for (j = replyCount - 1; j >= i; j--)
	    if (myIndices[j - 1] > myIndices[j])
	    {
		char temp1 = myIndices[j - 1];
		char temp2 = hisIndices[j - 1];

		myIndices[j - 1] = myIndices[j];
		myIndices[j] = temp1;

		hisIndices[j - 1] = hisIndices[j];
		hisIndices[j] = temp2;

		if (pixmap)
		{
		    unsigned temp3 = retFormats[j - 1];
		    int *temp4 = retDepths[j - 1];

		    retFormats[j - 1] = retFormats[j];
		    retFormats[j] = temp3;

		    retDepths[j - 1] = retDepths[j];
		    retDepths[j] = temp4;
		}
	    }

    /*
     * For each method supported, return to the proxy an index into
     * the list sent by the proxy, the opcode to be used for the method,
     * the pixmap formats supported, and the list of depths supported.
     */

    for (i = 0; i < replyCount; i++)
    {
	*preply++ = hisIndices[i];

	if (pixmap)
	{
	    int left;
	    *preply++ = LbxPixmapCompMethods[myIndices[i]].methodOpCode;
	    *preply++ = retFormats[i];
	    *preply++ = left = retDepths[i][0];
	    j = 1;
	    while (left > 0)
	    {
		*preply++ = retDepths[i][j];
		left--;
	    }
	}
	else
	{
	    *preply++ = LbxBitmapCompMethods[myIndices[i]].methodOpCode;
	}
    }

    if (pixmap)
    {
	pno->numPixmapCompMethods = replyCount;
	pno->pixmapCompMethods = myIndices;
	pno->pixmapCompDepths = retDepths;
    }
    else
    {
	pno->numBitmapCompMethods = replyCount;
	pno->bitmapCompMethods = myIndices;
    }

    if (hisIndices)
	xfree (hisIndices);

    if (pixmap)
    {
	if (retFormats)
	    xfree (retFormats);
    }

    return (preply - preplyStart);
}



static int
LbxBitmapCompOpt (LbxNegOptsPtr   pno,
		  unsigned char  *popt,
		  int	          optlen,
		  unsigned char  *preply)

{
    return (LbxImageCompOpt (0 /* bitmap */, pno, popt, optlen, preply));
}


static int
LbxPixmapCompOpt (LbxNegOptsPtr   pno,
		  unsigned char  *popt,
		  int	          optlen,
		  unsigned char  *preply)

{
    return (LbxImageCompOpt (1 /* Pixmap */, pno, popt, optlen, preply));
}


LbxBitmapCompMethod *
LbxSrvrLookupBitmapCompMethod (LbxProxyPtr proxy,
			       int methodOpCode)

{
    int i;

    for (i = 0; i < proxy->numBitmapCompMethods; i++)
    {
	LbxBitmapCompMethod *method;

	method = &LbxBitmapCompMethods[proxy->bitmapCompMethods[i]];

	if (method->methodOpCode == methodOpCode)
	    return (method);
    }

    return (NULL);
}


LbxPixmapCompMethod *
LbxSrvrLookupPixmapCompMethod (LbxProxyPtr proxy,
			       int methodOpCode)

{
    int i;

    for (i = 0; i < proxy->numPixmapCompMethods; i++)
    {
	LbxPixmapCompMethod *method;

	method = &LbxPixmapCompMethods[proxy->pixmapCompMethods[i]];

	if (method->methodOpCode == methodOpCode)
	    return (method);
    }

    return (NULL);
}


LbxBitmapCompMethod *
LbxSrvrFindPreferredBitmapCompMethod (LbxProxyPtr proxy)

{
    if (proxy->numBitmapCompMethods == 0)
	return NULL;
    else
	return (&LbxBitmapCompMethods[proxy->bitmapCompMethods[0]]);
}



LbxPixmapCompMethod *
LbxSrvrFindPreferredPixmapCompMethod (LbxProxyPtr proxy,
				      int format,
				      int depth)

{
    if (proxy->numPixmapCompMethods == 0)
	return NULL;
    else
    {
	LbxPixmapCompMethod *method;
	int i, j;

	for (i = 0; i < proxy->numPixmapCompMethods; i++)
	{
	    method = &LbxPixmapCompMethods[proxy->pixmapCompMethods[i]];

	    if ((method->formatMask & (1 << format)))
	    {
		int n = proxy->pixmapCompDepths[i][0];
		j = 1;
		while (n > 0)
		{
		    if (depth == proxy->pixmapCompDepths[i][j])
			return method;
		    else
			n--;
		}
	    }
	}

	return NULL;
    }
}


static int 
MergeDepths (int *depths,
	     LbxPixmapCompMethod *method)

{
    int i, j, count;
    int temp[LBX_MAX_DEPTHS + 1];

    temp[0] = count = 0;

    for (i = 1; i <= depths[0]; i++)
    {
	for (j = 0; j < method->depthCount; j++)
	    if (method->depths[j] == depths[i])
	    {
		temp[0]++;
		temp[++count] = depths[i];
		break;
	    }
    }

    memcpy (depths, temp, (count + 1) * sizeof (int));

    return (count);
}


#define LbxCmapAllMethod "XC-CMAP"

static int
LbxCmapAllOpt (LbxNegOptsPtr   pno,
	       unsigned char  *popt,
	       int	       optlen,
	       unsigned char  *preply)

{
    int numMethods = *popt++;
    int i;

    for (i = 0; i < numMethods; i++)
    {
	int len;
	char *methodName;

	len = *popt++;
	methodName = (char *) popt;
	popt += len;
	if (!strncmp(methodName, LbxCmapAllMethod, len))
	    break;
    }
    if (i >= numMethods)
	i = 0; /* assume first one is proxy's favorite */
    *preply = i;
    return 1;
}
