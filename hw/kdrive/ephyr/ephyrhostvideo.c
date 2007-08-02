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
#include <X11/Xutil.h>
#include <X11/Xlibint.h>
#include <X11/extensions/Xvlib.h>
#include <X11/extensions/Xvproto.h>
#include <X11/extensions/Xext.h>
#include <X11/extensions/extutil.h>
#define _HAVE_XALLOC_DECLS

#include "hostx.h"
#include "ephyrhostvideo.h"
#include "ephyrlog.h"

#ifndef TRUE
#define TRUE 1
#endif /*TRUE*/

#ifndef FALSE
#define FALSE 0
#endif /*FALSE*/

static XExtensionInfo _xv_info_data;
static XExtensionInfo *xv_info = &_xv_info_data;
static char *xv_extension_name = XvName;
static char *xv_error_string(Display *dpy, int code, XExtCodes *codes,
                             char * buf, int n);
static int xv_close_display(Display *dpy, XExtCodes *codes);
static Bool xv_wire_to_event(Display *dpy, XEvent *host, xEvent *wire);

static XExtensionHooks xv_extension_hooks = {
    NULL,                               /* create_gc */
    NULL,                               /* copy_gc */
    NULL,                               /* flush_gc */
    NULL,                               /* free_gc */
    NULL,                               /* create_font */
    NULL,                               /* free_font */
    xv_close_display,                   /* close_display */
    xv_wire_to_event,                   /* wire_to_event */
    NULL,                               /* event_to_wire */
    NULL,                               /* error */
    xv_error_string                     /* error_string */
};


static char *xv_error_list[] =
{
   "BadPort",       /* XvBadPort     */
   "BadEncoding",   /* XvBadEncoding */
   "BadControl"     /* XvBadControl  */
};


#define XvCheckExtension(dpy, i, val) \
  XextCheckExtension(dpy, i, xv_extension_name, val)
#define XvGetReq(name, req) \
        WORD64ALIGN\
        if ((dpy->bufptr + SIZEOF(xv##name##Req)) > dpy->bufmax)\
                _XFlush(dpy);\
        req = (xv##name##Req *)(dpy->last_req = dpy->bufptr);\
        req->reqType = info->codes->major_opcode;\
        req->xvReqType = xv_##name; \
        req->length = (SIZEOF(xv##name##Req))>>2;\
        dpy->bufptr += SIZEOF(xv##name##Req);\
        dpy->request++

static XEXT_GENERATE_CLOSE_DISPLAY (xv_close_display, xv_info)


static XEXT_GENERATE_FIND_DISPLAY (xv_find_display, xv_info,
                                   xv_extension_name,
                                   &xv_extension_hooks,
                                   XvNumEvents, NULL)

static XEXT_GENERATE_ERROR_STRING (xv_error_string, xv_extension_name,
                                   XvNumErrors, xv_error_list)

struct _EphyrHostXVAdaptorArray {
    XvAdaptorInfo *adaptors ;
    unsigned int nb_adaptors ;
};

Bool
EphyrHostXVQueryAdaptors (EphyrHostXVAdaptorArray **a_adaptors)
{
    EphyrHostXVAdaptorArray *result=NULL ;
    int ret=0 ;
    Bool is_ok=FALSE ;

    EPHYR_RETURN_VAL_IF_FAIL (a_adaptors, FALSE) ;

    EPHYR_LOG ("enter\n") ;

    result = Xcalloc (1, sizeof (EphyrHostXVAdaptorArray)) ;
    if (!result)
        goto out ;

    ret = XvQueryAdaptors (hostx_get_display (),
                           DefaultRootWindow (hostx_get_display ()),
                           &result->nb_adaptors,
                           &result->adaptors) ;
    if (ret != Success) {
        EPHYR_LOG_ERROR ("failed to query host adaptors: %d\n", ret) ;
        goto out ;
    }
    *a_adaptors = result ;
    is_ok = TRUE ;

out:
    EPHYR_LOG ("leave\n") ;
    return is_ok ;
}

void
EphyrHostXVAdaptorArrayDelete (EphyrHostXVAdaptorArray *a_adaptors)
{
    if (!a_adaptors)
        return ;
    if (a_adaptors->adaptors) {
        XvFreeAdaptorInfo (a_adaptors->adaptors) ;
        a_adaptors->adaptors = NULL ;
        a_adaptors->nb_adaptors = 0 ;
    }
    XFree (a_adaptors) ;
}

int
EphyrHostXVAdaptorArrayGetSize (const EphyrHostXVAdaptorArray *a_this)
{
    EPHYR_RETURN_VAL_IF_FAIL (a_this, -1) ;
    return a_this->nb_adaptors ;
}

EphyrHostXVAdaptor*
EphyrHostXVAdaptorArrayAt (const EphyrHostXVAdaptorArray *a_this,
                           int a_index)
{
    EPHYR_RETURN_VAL_IF_FAIL (a_this, NULL) ;

    if (a_index >= a_this->nb_adaptors)
        return NULL ;
    return (EphyrHostXVAdaptor*)&a_this->adaptors[a_index] ;
}

char
EphyrHostXVAdaptorGetType (const EphyrHostXVAdaptor *a_this)
{
    EPHYR_RETURN_VAL_IF_FAIL (a_this, -1) ;
    return ((XvAdaptorInfo*)a_this)->type ;
}

const char*
EphyrHostXVAdaptorGetName (const EphyrHostXVAdaptor *a_this)
{
    EPHYR_RETURN_VAL_IF_FAIL (a_this, NULL) ;

    return ((XvAdaptorInfo*)a_this)->name ;
}

EphyrHostVideoFormat*
EphyrHostXVAdaptorGetVideoFormats (const EphyrHostXVAdaptor *a_this,
                                   int *a_nb_formats)
{
    EphyrHostVideoFormat *formats=NULL ;
    int nb_formats=0, i=0 ;
    XVisualInfo *visual_info, visual_info_template ;
    int nb_visual_info ;

    EPHYR_RETURN_VAL_IF_FAIL (a_this, NULL) ;

    nb_formats = ((XvAdaptorInfo*)a_this)->num_formats ;
    formats = Xcalloc (nb_formats, sizeof (EphyrHostVideoFormat)) ;
    for (i=0; i < nb_formats; i++) {
        memset (&visual_info_template, 0, sizeof (visual_info_template)) ;
        visual_info_template.visualid =
                            ((XvAdaptorInfo*)a_this)->formats[i].visual_id;
        visual_info = XGetVisualInfo (hostx_get_display (),
                                      VisualIDMask,
                                      &visual_info_template,
                                      &nb_visual_info) ;
        formats[i].depth = ((XvAdaptorInfo*)a_this)->formats[i].depth ;
        formats[i].visual_class = visual_info->class ;
        XFree (visual_info) ;
    }
    if (a_nb_formats)
        *a_nb_formats = nb_formats ;
    return formats ;
}

int
EphyrHostXVAdaptorGetNbPorts (const EphyrHostXVAdaptor *a_this)
{
    EPHYR_RETURN_VAL_IF_FAIL (a_this, -1) ;

    return ((XvAdaptorInfo*)a_this)->num_ports ;
}

int
EphyrHostXVAdaptorGetFirstPortID (const EphyrHostXVAdaptor *a_this)
{
    EPHYR_RETURN_VAL_IF_FAIL (a_this, -1) ;

    return ((XvAdaptorInfo*)a_this)->base_id ;
}

Bool
EphyrHostXVQueryEncodings (int a_port_id,
                           EphyrHostEncoding **a_encodings,
                           unsigned int *a_num_encodings)
{
    EphyrHostEncoding *encodings=NULL ;
    XvEncodingInfo *encoding_info=NULL ;
    unsigned int num_encodings=0, i;
    int ret=0 ;

    EPHYR_RETURN_VAL_IF_FAIL (a_encodings && a_num_encodings, FALSE) ;

    ret = XvQueryEncodings (hostx_get_display (),
                            a_port_id,
                            &num_encodings,
                            &encoding_info) ;
    if (num_encodings && encoding_info) {
        encodings = Xcalloc (num_encodings, sizeof (EphyrHostEncoding)) ;
        for (i=0; i<num_encodings; i++) {
            encodings[i].id = encoding_info[i].encoding_id ;
            encodings[i].name = strdup (encoding_info[i].name) ;
            encodings[i].width = encoding_info[i].width ;
            encodings[i].height = encoding_info[i].height ;
            encodings[i].rate.numerator = encoding_info[i].rate.numerator ;
            encodings[i].rate.denominator = encoding_info[i].rate.denominator ;
        }
    }
    if (encoding_info) {
        XvFreeEncodingInfo (encoding_info) ;
        encoding_info = NULL ;
    }
    *a_encodings = encodings ;
    *a_num_encodings = num_encodings ;

    if (ret != Success)
        return FALSE ;
    return TRUE ;
}

void
EphyrHostEncodingsDelete (EphyrHostEncoding *a_encodings,
                          int a_num_encodings)
{
    int i=0 ;

    if (!a_encodings)
        return ;
    for (i=0; i < a_num_encodings; i++) {
        if (a_encodings[i].name) {
            xfree (a_encodings[i].name) ;
            a_encodings[i].name = NULL ;
        }
    }
    xfree (a_encodings) ;
}

void
EphyrHostAttributesDelete (EphyrHostAttribute *a_attributes)
{
    if (!a_attributes)
        return ;
    XFree (a_attributes) ;
}

Bool
EphyrHostXVQueryPortAttributes (int a_port_id,
                                EphyrHostAttribute **a_attributes,
                                int *a_num_attributes)
{
    EPHYR_RETURN_VAL_IF_FAIL (a_attributes && a_num_attributes, FALSE) ;

    *a_attributes =
        (EphyrHostAttribute*)XvQueryPortAttributes (hostx_get_display (),
                                                    a_port_id,
                                                    a_num_attributes);

    return TRUE ;
}

Bool
EphyrHostXVQueryImageFormats (int a_port_id,
                              EphyrHostImageFormat **a_formats,
                              int *a_num_format)
{
    XvImageFormatValues *result=NULL ;

    EPHYR_RETURN_VAL_IF_FAIL (a_formats && a_num_format, FALSE) ;

    result = XvListImageFormats (hostx_get_display (),
                                 a_port_id,
                                 a_num_format) ;
    *a_formats = (EphyrHostImageFormat*) result ;
    return TRUE ;

}

Bool
EphyrHostXVSetPortAttribute (int a_port_id,
                             int a_atom,
                             int a_attr_value)
{
    int res=Success ;

    EPHYR_LOG ("atom,value: (%d, %d)\n", a_atom, a_attr_value) ;

    res = XvSetPortAttribute (hostx_get_display (),
                              a_port_id,
                              a_atom,
                              a_attr_value) ;
    if (res != Success) {
        EPHYR_LOG_ERROR ("XvSetPortAttribute() failed: %d\n", res) ;
        return FALSE ;
    }
    EPHYR_LOG ("leave\n") ;

    return TRUE ;
}

Bool
EphyrHostXVGetPortAttribute (int a_port_id,
                             int a_atom,
                             int *a_attr_value)
{
    int res=Success ;
    Bool ret=FALSE ;

    EPHYR_RETURN_VAL_IF_FAIL (a_attr_value, FALSE) ;

    EPHYR_LOG ("enter, a_port_id: %d, a_atomid: %d, attr_name: %s\n",
               a_port_id, a_atom, XGetAtomName (hostx_get_display (), a_atom)) ;

    res = XvGetPortAttribute (hostx_get_display (),
                              a_port_id,
                              a_atom,
                              a_attr_value) ;
    if (res != Success) {
        EPHYR_LOG_ERROR ("XvGetPortAttribute() failed: %d \n", res) ;
        goto out ;
    }
    EPHYR_LOG ("atom,value: (%d, %d)\n", a_atom, *a_attr_value) ;

    ret = TRUE ;

out:
    EPHYR_LOG ("leave\n") ;
    return ret ;
}

Bool
EphyrHostXVQueryBestSize (int a_port_id,
                          Bool a_motion,
                          unsigned int a_frame_w,
                          unsigned int a_frame_h,
                          unsigned int a_drw_w,
                          unsigned int a_drw_h,
                          unsigned int *a_actual_w,
                          unsigned int *a_actual_h)
{
    int res=0 ;
    Bool is_ok=FALSE ;

    EPHYR_RETURN_VAL_IF_FAIL (a_actual_w && a_actual_h, FALSE) ;

    EPHYR_LOG ("enter: frame (%dx%d), drw (%dx%d)\n",
               a_frame_w, a_frame_h,
               a_drw_w, a_drw_h) ;

    res = XvQueryBestSize (hostx_get_display (),
                           a_port_id,
                           a_motion,
                           a_frame_w, a_frame_h,
                           a_drw_w, a_drw_h,
                           a_actual_w, a_actual_h) ;
    if (res != Success) {
        EPHYR_LOG_ERROR ("XvQueryBestSize() failed: %d\n", res) ;
        goto out ;
    }

    EPHYR_LOG ("actual (%dx%d)\n", *a_actual_w, *a_actual_h) ;

    is_ok = TRUE ;

out:
    EPHYR_LOG ("leave\n") ;
    return is_ok ;
}

static Bool
xv_wire_to_event(Display *dpy, XEvent *host, xEvent *wire)
{
    XExtDisplayInfo *info = xv_find_display (dpy);
    XvEvent *re    = (XvEvent *)host;
    xvEvent *event = (xvEvent *)wire;

    XvCheckExtension(dpy, info, False);

    switch ((event->u.u.type & 0x7F) - info->codes->first_event) {
        case XvVideoNotify:
            re->xvvideo.type = event->u.u.type & 0x7f;
            re->xvvideo.serial =
            _XSetLastRequestRead(dpy, (xGenericReply *)event);
            re->xvvideo.send_event = ((event->u.u.type & 0x80) != 0);
            re->xvvideo.display = dpy;
            re->xvvideo.time = event->u.videoNotify.time;
            re->xvvideo.reason = event->u.videoNotify.reason;
            re->xvvideo.drawable = event->u.videoNotify.drawable;
            re->xvvideo.port_id = event->u.videoNotify.port;
            break;
        case XvPortNotify:
            re->xvport.type = event->u.u.type & 0x7f;
            re->xvport.serial =
            _XSetLastRequestRead(dpy, (xGenericReply *)event);
            re->xvport.send_event = ((event->u.u.type & 0x80) != 0);
            re->xvport.display = dpy;
            re->xvport.time = event->u.portNotify.time;
            re->xvport.port_id = event->u.portNotify.port;
            re->xvport.attribute = event->u.portNotify.attribute;
            re->xvport.value = event->u.portNotify.value;
            break;
        default:
            return False;
    }

    return True ;
}

Bool
EphyrHostXVQueryImageAttributes (int a_port_id,
                                 int a_image_id /*image fourcc code*/,
                                 unsigned short *a_width,
                                 unsigned short *a_height,
                                 int *a_image_size,
                                 int *a_pitches,
                                 int *a_offsets)
{
    Display *dpy = hostx_get_display () ;
    Bool ret=FALSE ;
    XExtDisplayInfo *info = xv_find_display (dpy);
    xvQueryImageAttributesReq *req=NULL;
    xvQueryImageAttributesReply rep;

    EPHYR_RETURN_VAL_IF_FAIL (a_width, FALSE) ;
    EPHYR_RETURN_VAL_IF_FAIL (a_height, FALSE) ;
    EPHYR_RETURN_VAL_IF_FAIL (a_image_size, FALSE) ;

    XvCheckExtension (dpy, info, FALSE);

    LockDisplay (dpy);

    XvGetReq (QueryImageAttributes, req);
    req->id = a_image_id;
    req->port = a_port_id;
    req->width = *a_width;
    req->height = *a_height;
    /*
     * read the reply
     */
    if (!_XReply (dpy, (xReply *)&rep, 0, xFalse)) {
        EPHYR_LOG_ERROR ("QeryImageAttribute req failed\n") ;
        goto out ;
    }
    if (a_pitches && a_offsets) {
        _XRead (dpy,
                (char*)a_pitches,
                rep.num_planes << 2);
        _XRead (dpy,
                (char*)a_offsets,
                rep.num_planes << 2);
    } else {
        _XEatData(dpy, rep.length << 2);
    }
    *a_width = rep.width ;
    *a_height = rep.height ;
    *a_image_size = rep.data_size ;

    ret = TRUE ;

out:
    UnlockDisplay (dpy) ;
    SyncHandle ();
    return ret ;
}

Bool
EphyrHostGetAtom (const char* a_name,
                  Bool a_create_if_not_exists,
                  int *a_atom)
{
    int atom=None ;

    EPHYR_RETURN_VAL_IF_FAIL (a_atom, FALSE) ;

    atom = XInternAtom (hostx_get_display (), a_name, a_create_if_not_exists);
    if (atom == None) {
        return FALSE ;
    }
    *a_atom = atom ;
    return TRUE ;
}

char*
EphyrHostGetAtomName (int a_atom)
{
    return XGetAtomName (hostx_get_display (), a_atom) ;
}

void
EphyrHostFree (void *a_pointer)
{
    if (a_pointer)
        XFree (a_pointer) ;
}

Bool
EphyrHostXVPutImage (int a_port_id,
                     int a_image_id,
                     int a_drw_x,
                     int a_drw_y,
                     int a_drw_w,
                     int a_drw_h,
                     int a_src_x,
                     int a_src_y,
                     int a_src_w,
                     int a_src_h,
                     int a_image_width,
                     int a_image_height,
                     unsigned char *a_buf)
{
    Bool is_ok=TRUE ;
    XvImage *xv_image=NULL ;
    GC gc=0 ;
    XGCValues gc_values;

    EPHYR_RETURN_VAL_IF_FAIL (a_buf, FALSE) ;

    EPHYR_LOG ("enter\n") ;

    gc = XCreateGC (hostx_get_display (), hostx_get_window (), 0L, &gc_values);
    if (!gc) {
        EPHYR_LOG_ERROR ("failed to create gc \n") ;
        goto out ;
    }
    xv_image = (XvImage*) XvCreateImage (hostx_get_display (),
                                         a_port_id, a_image_id,
                                         NULL, a_image_width, a_image_height) ;
    if (!xv_image) {
        EPHYR_LOG_ERROR ("failed to create image\n") ;
        goto out ;
    }
    xv_image->data = (char*)a_buf ;
    XvPutImage (hostx_get_display (), a_port_id, hostx_get_window (),
                gc, xv_image,
                a_src_x, a_src_y, a_src_w, a_src_h,
                a_drw_x, a_drw_y, a_drw_w, a_drw_h) ;
    XFlush (hostx_get_display ()) ;
    is_ok = TRUE ;

out:

    EPHYR_LOG ("leave\n") ;
    if (xv_image) {
        XFree (xv_image) ;
        xv_image = NULL ;
    }
    return is_ok ;
}
