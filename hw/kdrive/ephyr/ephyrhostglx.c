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
 * a lots of the content of this file has been adapted from the mesa source
 * code.
 * Authors:
 *    Dodji Seketeli <dodji@openedhand.com>
 */
#ifdef HAVE_CONFIG_H
#include <kdrive-config.h>
#endif

#include <X11/Xlibint.h>
#include <GL/glx.h>
#include <GL/internal/glcore.h>
#include <GL/glxproto.h>
#include <GL/glxint.h>
#include "hostx.h"
#include "ephyrhostglx.h"
#define _HAVE_XALLOC_DECLS
#include "ephyrlog.h"

#ifdef XEPHYR_DRI
enum VisualConfRequestType {
    EPHYR_GET_FB_CONFIG,
    EPHYR_VENDOR_PRIV_GET_FB_CONFIG_SGIX,
    EPHYR_GET_VISUAL_CONFIGS

};

static Bool ephyrHostGLXGetVisualConfigsInternal
                                        (enum VisualConfRequestType a_type,
                                         int32_t a_screen,
                                         int32_t *a_num_visuals,
                                         int32_t *a_num_props,
                                         int32_t *a_props_buf_size,
                                         int32_t **a_props_buf);
Bool
ephyrHostGLXGetMajorOpcode (int *a_opcode)
{
    Bool is_ok=FALSE ;
    Display *dpy=hostx_get_display () ;
    static int opcode ;
    int first_event_return=0, first_error_return=0;

    EPHYR_RETURN_VAL_IF_FAIL (dpy, FALSE) ;
    EPHYR_LOG ("enter\n") ;
    if (!opcode) {
        if (!XQueryExtension (dpy, GLX_EXTENSION_NAME, &opcode,
                              &first_event_return, &first_error_return)) {
            EPHYR_LOG_ERROR ("XQueryExtension() failed\n") ;
            goto out ;
        }
    }
    *a_opcode = opcode ;
    is_ok = TRUE ;
out:
    EPHYR_LOG ("release\n") ;
    return is_ok ;
}

Bool
ephyrHostGLXQueryVersion (int *a_major, int *a_minor)
{
    Bool is_ok = FALSE ;
    Display *dpy = hostx_get_display () ;

    EPHYR_RETURN_VAL_IF_FAIL (a_major && a_minor, FALSE) ;
    EPHYR_LOG ("enter\n") ;

    if (!glXQueryVersion (dpy, a_major, a_minor)) {
        EPHYR_LOG_ERROR ("glxQueryVersion() failed\n") ;
        goto out ;
    }

    is_ok = TRUE ;
out:
    EPHYR_LOG ("leave\n") ;
    return is_ok ;
}

/**
 * GLX protocol structure for the ficticious "GXLGenericGetString" request.
 * 
 * This is a non-existant protocol packet.  It just so happens that all of
 * the real protocol packets used to request a string from the server have
 * an identical binary layout.  The only difference between them is the
 * meaning of the \c for_whom field and the value of the \c glxCode.
 */
typedef struct GLXGenericGetString {
    CARD8 reqType;
    CARD8 glxCode;
    CARD16 length B16;
    CARD32 for_whom B32;
    CARD32 name B32;
} xGLXGenericGetStringReq;

/* These defines are only needed to make the GetReq macro happy.
 */
#define sz_xGLXGenericGetStringReq 12
#define X_GLXGenericGetString 0

Bool
ephyrHostGLXGetStringFromServer (int a_screen_number,
                                 int a_string_name,
                                 char **a_string)
{
    Display *dpy = hostx_get_display () ;
    xGLXGenericGetStringReq *req=NULL;
    xGLXSingleReply reply;
    int length=0, numbytes=0, major_opcode=0;

    EPHYR_RETURN_VAL_IF_FAIL (dpy && a_string, FALSE) ;

    EPHYR_LOG ("enter\n") ;

    if (!ephyrHostGLXGetMajorOpcode (&major_opcode)) {
        EPHYR_LOG_ERROR ("failed to get major opcode\n") ;
        return FALSE ;
    }
    EPHYR_LOG ("major opcode: %d\n", major_opcode) ;

    LockDisplay (dpy);

    /* All of the GLX protocol requests for getting a string from the server
     * look the same.  The exact meaning of the a_for_whom field is usually
     * either the screen number (for glXQueryServerString) or the context tag
     * (for GLXSingle).
     */
    GetReq (GLXGenericGetString, req);
    req->reqType = major_opcode;
    req->glxCode = X_GLXQueryServerString;
    req->for_whom = a_screen_number;
    req->name = a_string_name;

    _XReply (dpy, (xReply *)&reply, 0, False);

    length = reply.length * 4;
    numbytes = reply.size;

    *a_string = (char *) Xmalloc( numbytes );
    if (*a_string != NULL) {
        if (_XRead (dpy, *a_string, numbytes)) {
            EPHYR_LOG_ERROR ("read failed\n") ;
        }
        length -= numbytes;
    }

    _XEatData (dpy, length) ;

    UnlockDisplay (dpy);
    SyncHandle ();

    EPHYR_LOG ("leave\n") ;
    return TRUE ;
}

static Bool
ephyrHostGLXGetVisualConfigsInternal (enum VisualConfRequestType a_type,
                                      int32_t a_screen,
                                      int32_t *a_num_visuals,
                                      int32_t *a_num_props,
                                      int32_t *a_props_buf_size,
                                      int32_t **a_props_buf)
{
    Bool is_ok = FALSE ;
    Display *dpy = hostx_get_display () ;
    xGLXGetVisualConfigsReq *req;
    xGLXGetFBConfigsReq *fb_req;
    xGLXVendorPrivateWithReplyReq *vpreq;
    xGLXGetFBConfigsSGIXReq *sgi_req;
    xGLXGetVisualConfigsReply reply;
    char *server_glx_version=NULL,
         *server_glx_extensions=NULL ;
    int j=0,
        screens=0,
        major_opcode=0,
        num_props=0,
        num_visuals=0,
        props_buf_size=0,
        props_per_visual_size=0;
    int32_t *props_buf=NULL;

    EPHYR_RETURN_VAL_IF_FAIL (dpy, FALSE) ;

    screens = ScreenCount (dpy);
    if (!ephyrHostGLXGetMajorOpcode (&major_opcode)) {
        EPHYR_LOG_ERROR ("failed to get opcode\n") ;
        goto out ;
    }

    LockDisplay(dpy);
    switch (a_type) {
        case EPHYR_GET_FB_CONFIG:
        GetReq(GLXGetFBConfigs,fb_req);
        fb_req->reqType = major_opcode;
        fb_req->glxCode = X_GLXGetFBConfigs;
        fb_req->screen = a_screen;
        break;

        case EPHYR_VENDOR_PRIV_GET_FB_CONFIG_SGIX:
        GetReqExtra(GLXVendorPrivateWithReply,
                    sz_xGLXGetFBConfigsSGIXReq
                         -
                    sz_xGLXVendorPrivateWithReplyReq,
                    vpreq);
        sgi_req = (xGLXGetFBConfigsSGIXReq *) vpreq;
        sgi_req->reqType = major_opcode;
        sgi_req->glxCode = X_GLXVendorPrivateWithReply;
        sgi_req->vendorCode = X_GLXvop_GetFBConfigsSGIX;
        sgi_req->screen = a_screen;
        break;

        case EPHYR_GET_VISUAL_CONFIGS:
        GetReq(GLXGetVisualConfigs,req);
        req->reqType = major_opcode;
        req->glxCode = X_GLXGetVisualConfigs;
        req->screen = a_screen;
        break;
    }

    if (!_XReply(dpy, (xReply*) &reply, 0, False)) {
        EPHYR_LOG_ERROR ("unknown error\n") ;
        UnlockDisplay(dpy);
        goto out ;
    }
   if (!reply.numVisuals) {
        EPHYR_LOG_ERROR ("screen does not support GL rendering\n") ;
        UnlockDisplay(dpy);
        goto out ;
    }
   num_visuals = reply.numVisuals ;

    /* FIXME: Is the __GLX_MIN_CONFIG_PROPS test correct for
     * FIXME: FBconfigs? 
     */
    /* Check number of properties */
    num_props = reply.numProps;
    if ((num_props < __GLX_MIN_CONFIG_PROPS) ||
        (num_props > __GLX_MAX_CONFIG_PROPS)) {
        /* Huh?  Not in protocol defined limits.  Punt */
        EPHYR_LOG_ERROR ("got a bad reply to request\n") ;
        UnlockDisplay(dpy);
        goto out ;
    }

    if (a_type != EPHYR_GET_VISUAL_CONFIGS) {
        num_props *= 2;
    }
    props_per_visual_size = num_props * __GLX_SIZE_INT32;
    props_buf_size = props_per_visual_size * reply.numVisuals;
    props_buf = malloc (props_buf_size) ;
    for (j = 0; j < reply.numVisuals; j++) {
        if (_XRead (dpy,
                    &((char*)props_buf)[j*props_per_visual_size],
                    props_per_visual_size) != Success) {
            EPHYR_LOG_ERROR ("read failed\n") ;
        }
    }
    UnlockDisplay(dpy);

    *a_num_visuals = num_visuals ;
    *a_num_props = reply.numProps ;
    *a_props_buf_size = props_buf_size ;
    *a_props_buf = props_buf ;
    is_ok = TRUE ;

out:
    if (server_glx_version) {
        XFree (server_glx_version) ;
        server_glx_version = NULL ;
    }
    if (server_glx_extensions) {
        XFree (server_glx_extensions) ;
        server_glx_extensions = NULL ;
    }
    SyncHandle () ;
    return is_ok;
}

Bool
ephyrHostGLXGetVisualConfigs (int32_t a_screen,
                              int32_t *a_num_visuals,
                              int32_t *a_num_props,
                              int32_t *a_props_buf_size,
                              int32_t **a_props_buf)
{
    Bool is_ok = FALSE;

    EPHYR_LOG ("enter\n") ;
    is_ok = ephyrHostGLXGetVisualConfigsInternal (EPHYR_GET_VISUAL_CONFIGS,
                                                  a_screen,
                                                  a_num_visuals,
                                                  a_num_props,
                                                  a_props_buf_size,
                                                  a_props_buf) ;

    EPHYR_LOG ("leave:%d\n", is_ok) ;
    return is_ok;
}

Bool
ephyrHostGLXVendorPrivGetFBConfigsSGIX (int a_screen,
                                        int32_t *a_num_visuals,
                                        int32_t *a_num_props,
                                        int32_t *a_props_buf_size,
                                        int32_t **a_props_buf)
{
    Bool is_ok=FALSE ;
    EPHYR_LOG ("enter\n") ;
    is_ok = ephyrHostGLXGetVisualConfigsInternal
                                        (EPHYR_VENDOR_PRIV_GET_FB_CONFIG_SGIX,
                                         a_screen,
                                         a_num_visuals,
                                         a_num_props,
                                         a_props_buf_size,
                                         a_props_buf) ;
    EPHYR_LOG ("leave\n") ;
    return is_ok ;
}

Bool
ephyrHostGLXSendClientInfo (int32_t a_major, int32_t a_minor,
                            const char* a_extension_list)
{
    Bool is_ok = FALSE ;
    Display *dpy = hostx_get_display () ;
    xGLXClientInfoReq *req;
    int size;
    int32_t major_opcode=0 ;

    EPHYR_RETURN_VAL_IF_FAIL (dpy && a_extension_list, FALSE) ;

    if (!ephyrHostGLXGetMajorOpcode (&major_opcode)) {
        EPHYR_LOG_ERROR ("failed to get major opcode\n") ;
        goto out ;
    }

    LockDisplay (dpy);

    GetReq (GLXClientInfo,req);
    req->reqType = major_opcode;
    req->glxCode = X_GLXClientInfo;
    req->major = a_major;
    req->minor = a_minor;

    size = strlen (a_extension_list) + 1;
    req->length += (size + 3) >> 2;
    req->numbytes = size;
    Data (dpy, a_extension_list, size);

    is_ok=TRUE ;

out:
    UnlockDisplay(dpy);
    SyncHandle();
    return is_ok ;
}

#endif /*XEPHYR_DRI*/

