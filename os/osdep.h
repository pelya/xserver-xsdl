/***********************************************************

Copyright 1987, 1998  The Open Group

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


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
/* $Xorg: osdep.h,v 1.5 2001/02/09 02:05:23 xorgcvs Exp $ */

#define BOTIMEOUT 200 /* in milliseconds */
#define BUFSIZE 4096
#define BUFWATERMARK 8192
#define MAXBUFSIZE (1 << 22)

#ifndef sgi	    /* SGI defines OPEN_MAX in a useless way */
#ifndef X_NOT_POSIX
#ifdef _POSIX_SOURCE
#include <limits.h>
#else
#define _POSIX_SOURCE
#include <limits.h>
#undef _POSIX_SOURCE
#endif
#else /* X_NOT_POSIX */
#ifdef WIN32
#define _POSIX_
#include <limits.h>
#undef _POSIX_
#endif
#endif /* X_NOT_POSIX */
#endif

#ifndef OPEN_MAX
#ifdef SVR4
#define OPEN_MAX 256
#else
#include <sys/param.h>
#ifndef OPEN_MAX
#if defined(NOFILE) && !defined(NOFILES_MAX)
#define OPEN_MAX NOFILE
#else
#define OPEN_MAX NOFILES_MAX
#endif
#endif
#endif
#endif

#if OPEN_MAX <= 256
#define MAXSOCKS (OPEN_MAX - 1)
#else
#define MAXSOCKS 256
#endif

#ifndef NULL
#define NULL 0
#endif

typedef struct _connectionInput {
    struct _connectionInput *next;
    char *buffer;               /* contains current client input */
    char *bufptr;               /* pointer to current start of data */
    int  bufcnt;                /* count of bytes in buffer */
    int lenLastReq;
    int size;
} ConnectionInput, *ConnectionInputPtr;

typedef struct _connectionOutput {
    struct _connectionOutput *next;
    int size;
    unsigned char *buf;
    int count;
#ifdef LBX
    Bool nocompress;
#endif
} ConnectionOutput, *ConnectionOutputPtr;

#ifdef K5AUTH
typedef struct _k5_state {
    int		stageno;	/* current stage of auth protocol */
    pointer	srvcreds;	/* server credentials */
    pointer	srvname;	/* server principal name */
    pointer	ktname;		/* key table: principal-key pairs */
    pointer	skey;		/* session key */
}           k5_state;
#endif

#ifdef LBX
typedef struct _LbxProxy *OsProxyPtr;
#endif

typedef struct _osComm {
    int fd;
    ConnectionInputPtr input;
    ConnectionOutputPtr output;
    XID	auth_id;		/* authorization id */
#ifdef K5AUTH
    k5_state	authstate;	/* state of setup auth conversation */
#endif
    CARD32 conn_time;		/* timestamp if not established, else 0  */
    struct _XtransConnInfo *trans_conn; /* transport connection object */
#ifdef LBX
    OsProxyPtr proxy;
    ConnectionInputPtr largereq;
    void (*Close) ();
    int  (*Flush) ();
#endif
} OsCommRec, *OsCommPtr;

#ifdef LBX
#define FlushClient(who, oc, extraBuf, extraCount) \
    (*(oc)->Flush)(who, oc, extraBuf, extraCount)
extern int StandardFlushClient(
#if NeedFunctionPrototypes
    ClientPtr /*who*/,
    OsCommPtr /*oc*/,
    char* /*extraBuf*/,
    int /*extraCount*/
#endif
);
#else
extern int FlushClient(
#if NeedFunctionPrototypes
    ClientPtr /*who*/,
    OsCommPtr /*oc*/,
    char* /*extraBuf*/,
    int /*extraCount*/
#endif
);
#endif

extern void FreeOsBuffers(
#if NeedFunctionPrototypes
    OsCommPtr /*oc*/
#endif
);

extern ConnectionInputPtr AllocateInputBuffer(
#if NeedFunctionPrototypes
    void
#endif
);

extern ConnectionOutputPtr AllocateOutputBuffer(
#if NeedFunctionPrototypes
    void
#endif
);
