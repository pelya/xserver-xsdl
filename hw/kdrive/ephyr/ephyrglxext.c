/*
 * Xephyr - A kdrive X server thats runs in a host X window.
 *          Authored by Matthew Allum <mallum@openedhand.com>
 * 
 * Copyright © 2007 OpenedHand Ltd 
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of OpenedHand Ltd not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission. OpenedHand Ltd makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * OpenedHand Ltd DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL OpenedHand Ltd BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors:
 *    Dodji Seketeli <dodji@openedhand.com>
 */
#ifdef HAVE_CONFIG_H
#include <kdrive-config.h>
#endif

#include "extnsionst.h"
#include "ephyrglxext.h"
#include "ephyrhostglx.h"
#define _HAVE_XALLOC_DECLS
#include "ephyrlog.h"
#include <GL/glxproto.h>
#include "GL/glx/glxserver.h"
#include "GL/glx/indirect_table.h"
#include "GL/glx/unpack.h"


#ifdef XEPHYR_DRI

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif


int ephyrGLXQueryVersion (__GLXclientState *cl, GLbyte *pc) ;
int ephyrGLXQueryVersionSwap (__GLXclientState *cl, GLbyte *pc) ;
int ephyrGLXGetVisualConfigs (__GLXclientState *cl, GLbyte *pc) ;
int ephyrGLXGetVisualConfigsSwap (__GLXclientState *cl, GLbyte *pc) ;
int ephyrGLXClientInfo(__GLXclientState *cl, GLbyte *pc) ;
int ephyrGLXClientInfoSwap(__GLXclientState *cl, GLbyte *pc) ;
int ephyrGLXQueryServerString(__GLXclientState *a_cl, GLbyte *a_pc) ;
int ephyrGLXQueryServerStringSwap(__GLXclientState *a_cl, GLbyte *a_pc) ;

Bool
ephyrHijackGLXExtension (void)
{
    const void *(*dispatch_functions)[2];

    EPHYR_LOG ("Got GLX extension\n") ;
    if (!Single_dispatch_info.dispatch_functions) {
        EPHYR_LOG_ERROR ("could not get dispatch functions table\n") ;
        return FALSE ;
    }
    dispatch_functions = Single_dispatch_info.dispatch_functions ;
    EPHYR_RETURN_VAL_IF_FAIL (dispatch_functions, FALSE) ;

    dispatch_functions[X_GLXQueryVersion][0] = ephyrGLXQueryVersion ;
    dispatch_functions[X_GLXQueryVersion][1] = ephyrGLXQueryVersionSwap ;

    dispatch_functions[X_GLXGetVisualConfigs][0] = ephyrGLXGetVisualConfigs ;
    dispatch_functions[X_GLXGetVisualConfigs][1] = ephyrGLXGetVisualConfigsSwap ;

    dispatch_functions[X_GLXClientInfo][0] = ephyrGLXClientInfo ;
    dispatch_functions[X_GLXClientInfo][1] = ephyrGLXClientInfoSwap ;

    dispatch_functions[X_GLXQueryServerString][0] = ephyrGLXQueryServerString ;
    dispatch_functions[X_GLXQueryServerString][1] = ephyrGLXQueryServerStringSwap ;

    EPHYR_LOG ("hijacked mesa GL to forward requests to host X\n") ;

    return TRUE ;
}

/*********************
 * implementation of
 * hijacked GLX entry
 * points
 ********************/

int
ephyrGLXQueryVersion(__GLXclientState *a_cl, GLbyte *a_pc)
{
    ClientPtr client = a_cl->client;
    xGLXQueryVersionReq *req = (xGLXQueryVersionReq *) a_pc;
    xGLXQueryVersionReply reply;
    int major, minor;
    int res = BadImplementation ;

    EPHYR_LOG ("enter\n") ;

    major = req->majorVersion ;
    minor = req->minorVersion ;

    if (!ephyrHostGLXQueryVersion (&major, &minor)) {
        EPHYR_LOG_ERROR ("ephyrHostGLXQueryVersion() failed\n") ;
        goto out ;
    }
    reply.majorVersion = major ;
    reply.minorVersion = minor ;
    reply.length = 0 ;
    reply.type = X_Reply ;
    reply.sequenceNumber = client->sequence ;

    if (client->swapped) {
        __glXSwapQueryVersionReply(client, &reply);
    } else {
        WriteToClient(client, sz_xGLXQueryVersionReply, (char *)&reply);
    }

    res = Success ;
out:
    EPHYR_LOG ("leave\n") ;
    return res;
}

int
ephyrGLXQueryVersionSwap (__GLXclientState *a_cl, GLbyte *a_pc)
{
    xGLXQueryVersionReq *req = (xGLXQueryVersionReq *) a_pc;
    __GLX_DECLARE_SWAP_VARIABLES;

    __GLX_SWAP_SHORT (&req->length);
    __GLX_SWAP_INT (&req->majorVersion);
    __GLX_SWAP_INT (&req->minorVersion);
    return ephyrGLXQueryVersion (a_cl, a_pc) ;
}

static int
ephyrGLXGetVisualConfigsReal (__GLXclientState *a_cl, GLbyte *a_pc, Bool a_do_swap)
{
    xGLXGetVisualConfigsReq *req = (xGLXGetVisualConfigsReq *) a_pc;
    ClientPtr client = a_cl->client;
    xGLXGetVisualConfigsReply reply;
    int32_t *props_buf=NULL, num_visuals=0,
            num_props=0, res=BadImplementation, i=0,
            props_per_visual_size=0,
            props_buf_size=0;
    __GLX_DECLARE_SWAP_VARIABLES;
    __GLX_DECLARE_SWAP_ARRAY_VARIABLES;

    EPHYR_LOG ("enter\n") ;

    if (!ephyrHostGLXGetVisualConfigs (req->screen,
                                       &num_visuals,
                                       &num_props,
                                       &props_buf_size,
                                       &props_buf)) {
        EPHYR_LOG_ERROR ("ephyrHostGLXGetVisualConfigs() failed\n") ;
        goto out ;
    }
    EPHYR_LOG ("num_visuals:%d, num_props:%d\n", num_visuals, num_props) ;

    reply.numVisuals = num_visuals;
    reply.numProps = num_props;
    reply.length = (num_visuals *__GLX_SIZE_CARD32 * num_props) >> 2;
    reply.type = X_Reply;
    reply.sequenceNumber = client->sequence;

    if (a_do_swap) {
        __GLX_SWAP_SHORT(&reply.sequenceNumber);
        __GLX_SWAP_INT(&reply.length);
        __GLX_SWAP_INT(&reply.numVisuals);
        __GLX_SWAP_INT(&reply.numProps);
        __GLX_SWAP_INT_ARRAY (props_buf, num_props) ;
    }
    WriteToClient(client, sz_xGLXGetVisualConfigsReply, (char*)&reply);
    props_per_visual_size = props_buf_size/num_visuals ;
    for (i=0; i < num_visuals; i++) {
        WriteToClient (client,
                       props_per_visual_size,
                       (char*)props_buf +i*props_per_visual_size);
    }
    res = Success ;

out:
    EPHYR_LOG ("leave\n") ;
    if (props_buf) {
        xfree (props_buf) ;
        props_buf = NULL ;
    }
    return res ;
}

int
ephyrGLXGetVisualConfigs (__GLXclientState *a_cl, GLbyte *a_pc)
{
    return ephyrGLXGetVisualConfigsReal (a_cl, a_pc, FALSE) ;
}

int
ephyrGLXGetVisualConfigsSwap (__GLXclientState *a_cl, GLbyte *a_pc)
{
    return ephyrGLXGetVisualConfigsReal (a_cl, a_pc, TRUE) ;
}


int
ephyrGLXClientInfo(__GLXclientState *a_cl, GLbyte *a_pc)
{
    int res=BadImplementation ;
    xGLXClientInfoReq *req = (xGLXClientInfoReq *) a_pc;

    EPHYR_LOG ("enter\n") ;
    if (!ephyrHostGLXSendClientInfo (req->major, req->minor, (char*)req+1)) {
        EPHYR_LOG_ERROR ("failed to send client info to host\n") ;
        goto out ;
    }
    res = Success ;

out:
    EPHYR_LOG ("leave\n") ;
    return res ;
}

int
ephyrGLXClientInfoSwap (__GLXclientState *a_cl, GLbyte *a_pc)
{
    xGLXClientInfoReq *req = (xGLXClientInfoReq *)a_pc;
    __GLX_DECLARE_SWAP_VARIABLES;

    __GLX_SWAP_SHORT (&req->length);
    __GLX_SWAP_INT (&req->major);
    __GLX_SWAP_INT (&req->minor);
    __GLX_SWAP_INT (&req->numbytes);

    return ephyrGLXClientInfo (a_cl, a_pc) ;
}

int
ephyrGLXQueryServerString(__GLXclientState *a_cl, GLbyte *a_pc)
{
    int res = BadImplementation ;
    ClientPtr client = a_cl->client;
    xGLXQueryServerStringReq *req = (xGLXQueryServerStringReq *) a_pc;
    xGLXQueryServerStringReply reply;
    char *server_string=NULL ;
    int length=0 ;

    EPHYR_LOG ("enter\n") ;
    if (!ephyrHostGLXGetStringFromServer (req->screen, req->name, &server_string)) {
        EPHYR_LOG_ERROR ("failed to query string from host\n") ;
        goto out ;
    }
    length= strlen (server_string) + 1;
    reply.type = X_Reply ;
    reply.sequenceNumber = client->sequence ;
    reply.length = __GLX_PAD (length) >> 2 ;
    reply.n = length ;

    WriteToClient(client, sz_xGLXQueryServerStringReply, (char*)&reply);
    WriteToClient(client, (int)length, server_string);

    res = Success ;

out:
    EPHYR_LOG ("leave\n") ;
    if (server_string) {
        xfree (server_string) ;
        server_string = NULL;
    }
    return res ;
}

int
ephyrGLXQueryServerStringSwap(__GLXclientState *a_cl, GLbyte *a_pc)
{
    EPHYR_LOG_ERROR ("not yet implemented\n") ;
    return BadImplementation ;
}

#endif /*XEPHYR_DRI*/

