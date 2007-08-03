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
#include <string.h>
#include <X11/extensions/Xv.h>
#include "ephyrlog.h"
#include "kdrive.h"
#include "kxv.h"
#include "ephyr.h"
#include "hostx.h"
#include "ephyrhostvideo.h"

struct _EphyrXVPriv {
    EphyrHostXVAdaptorArray *host_adaptors ;
    KdVideoAdaptorPtr adaptors ;
    int num_adaptors ;
};
typedef struct _EphyrXVPriv EphyrXVPriv ;

struct _EphyrPortPriv {
    int port_number ;
    KdVideoAdaptorPtr current_adaptor ;
    EphyrXVPriv *xv_priv;
};
typedef struct _EphyrPortPriv EphyrPortPriv ;

static Bool DoSimpleClip (BoxPtr a_dst_drw,
                          BoxPtr a_clipper,
                          BoxPtr a_result) ;

static Bool ephyrLocalAtomToHost (int a_local_atom, int *a_host_atom) ;

static Bool ephyrHostAtomToLocal (int a_host_atom, int *a_local_atom) ;

static EphyrXVPriv* ephyrXVPrivNew (void) ;
static void ephyrXVPrivDelete (EphyrXVPriv *a_this) ;
static Bool ephyrXVPrivQueryHostAdaptors (EphyrXVPriv *a_this) ;
static Bool ephyrXVPrivSetAdaptorsHooks (EphyrXVPriv *a_this) ;
static Bool ephyrXVPrivRegisterAdaptors (EphyrXVPriv *a_this,
                                         ScreenPtr a_screen) ;

static Bool ephyrXVPrivIsAttrValueValid (KdAttributePtr a_attrs,
                                         int a_attrs_len,
                                         const char *a_attr_name,
                                         int a_attr_value,
                                         Bool *a_is_valid) ;

static void ephyrStopVideo (KdScreenInfo *a_info,
                            pointer a_xv_priv,
                            Bool a_exit);

static int ephyrSetPortAttribute (KdScreenInfo *a_info,
                                  Atom a_attr_name,
                                  int a_attr_value,
                                  pointer a_port_priv);

static int ephyrGetPortAttribute (KdScreenInfo *a_screen_info,
                                  Atom a_attr_name,
                                  int *a_attr_value,
                                  pointer a_port_priv);

static void ephyrQueryBestSize (KdScreenInfo *a_info,
                                Bool a_motion,
                                short a_src_w,
                                short a_src_h,
                                short a_drw_w,
                                short a_drw_h,
                                unsigned int *a_prefered_w,
                                unsigned int *a_prefered_h,
                                pointer a_port_priv);

static int ephyrPutImage (KdScreenInfo *a_info,
                          DrawablePtr a_drawable,
                          short a_src_x,
                          short a_src_y,
                          short a_drw_x,
                          short a_drw_y,
                          short a_src_w,
                          short a_src_h,
                          short a_drw_w,
                          short a_drw_h,
                          int a_id,
                          unsigned char *a_buf,
                          short a_width,
                          short a_height,
                          Bool a_sync,
                          RegionPtr a_clipping_region,
                          pointer a_port_priv);

static int ephyrQueryImageAttributes (KdScreenInfo *a_info,
                                      int a_id,
                                      unsigned short *a_w,
                                      unsigned short *a_h,
                                      int *a_pitches,
                                      int *a_offsets);
static int s_base_port_id ;

/**************
 * <helpers>
 * ************/

static Bool
DoSimpleClip (BoxPtr a_dst_box,
              BoxPtr a_clipper,
              BoxPtr a_result)
{
    BoxRec dstClippedBox ;

    EPHYR_RETURN_VAL_IF_FAIL (a_dst_box && a_clipper && a_result, FALSE) ;

    /*
     * setup the clipbox inside the destination.
     */
    dstClippedBox.x1 = a_dst_box->x1 ;
    dstClippedBox.x2 = a_dst_box->x2 ;
    dstClippedBox.y1 = a_dst_box->y1 ;
    dstClippedBox.y2 = a_dst_box->y2 ;

    /*
     * if the cliper leftmost edge is inside
     * the destination area then the leftmost edge of the resulting
     * clipped box is the leftmost edge of the cliper.
     */
    if (a_clipper->x1 > dstClippedBox.x1)
        dstClippedBox.x1 = a_clipper->x1 ;

    /*
     * if the cliper top edge is inside the destination area
     * then the bottom horizontal edge of the resulting clipped box
     * is the bottom edge of the cliper
     */
    if (a_clipper->y1 > dstClippedBox.y1)
        dstClippedBox.y1 = a_clipper->y1 ;

    /*ditto for right edge*/
    if (a_clipper->x2 < dstClippedBox.x2)
        dstClippedBox.x2 = a_clipper->x2 ;

    /*ditto for bottom edge*/
    if (a_clipper->y2 < dstClippedBox.y2)
        dstClippedBox.y2 = a_clipper->y2 ;

    memcpy (a_result, &dstClippedBox, sizeof (dstClippedBox)) ;
    return TRUE ;
}

static Bool
ephyrLocalAtomToHost (int a_local_atom, int *a_host_atom)
{
    char *atom_name=NULL;
    int host_atom=None ;

    EPHYR_RETURN_VAL_IF_FAIL (a_host_atom, FALSE) ;

    if (!ValidAtom (a_local_atom))
        return FALSE ;

    atom_name = NameForAtom (a_local_atom) ;

    if (!atom_name)
        return FALSE ;

    if (!ephyrHostGetAtom (atom_name, FALSE, &host_atom) || host_atom == None) {
        EPHYR_LOG_ERROR ("no atom for string %s defined in host X\n",
                         atom_name) ;
        return FALSE ;
    }
    *a_host_atom = host_atom ;
    return TRUE ;
}

static Bool
ephyrHostAtomToLocal (int a_host_atom, int *a_local_atom)
{
    Bool is_ok=FALSE ;
    char *atom_name=NULL ;
    int atom=None ;

    EPHYR_RETURN_VAL_IF_FAIL (a_local_atom, FALSE) ;

    atom_name = ephyrHostGetAtomName (a_host_atom) ;
    if (!atom_name)
        goto out ;

    atom = MakeAtom (atom_name, strlen (atom_name), TRUE) ;
    if (atom == None)
        goto out ;

    *a_local_atom = atom ;
    is_ok = TRUE ;

out:
    if (atom_name) {
        ephyrHostFree (atom_name) ;
    }
    return is_ok ;
}

/**************
 *</helpers>
 * ************/

Bool
ephyrInitVideo (ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    EphyrXVPriv *xv_priv = NULL ;

    EPHYR_LOG ("enter\n") ;

    if (screen->fb[0].bitsPerPixel == 8) {
        EPHYR_LOG_ERROR ("8 bits depth not supported\n") ;
        return FALSE ;
    }

    xv_priv = ephyrXVPrivNew () ;
    if (!xv_priv) {
        EPHYR_LOG_ERROR ("failed to create xv_priv\n") ;
        return FALSE ;
    }

    if (!ephyrXVPrivRegisterAdaptors (xv_priv, pScreen)) {
        EPHYR_LOG_ERROR ("failed to register adaptors\n") ;
        return FALSE ;
    }
    return TRUE ;
}

static EphyrXVPriv*
ephyrXVPrivNew (void)
{
    EphyrXVPriv *xv_priv=NULL ;

    EPHYR_LOG ("enter\n") ;

    xv_priv = xcalloc (1, sizeof (EphyrXVPriv)) ;
    if (!xv_priv) {
        EPHYR_LOG_ERROR ("failed to create EphyrXVPriv\n") ;
        goto error ;
    }

    ephyrHostXVInit () ;

    if (!ephyrXVPrivQueryHostAdaptors (xv_priv)) {
        EPHYR_LOG_ERROR ("failed to query the host x for xv properties\n") ;
        goto error ;
    }
    if (!ephyrXVPrivSetAdaptorsHooks (xv_priv)) {
        EPHYR_LOG_ERROR ("failed to set xv_priv hooks\n") ;
        goto error ;
    }

    EPHYR_LOG ("leave\n") ;
    return xv_priv ;

error:
    if (xv_priv) {
        ephyrXVPrivDelete (xv_priv) ;
        xv_priv = NULL ;
    }
    return NULL ;
}

static void
ephyrXVPrivDelete (EphyrXVPriv *a_this)
{
    EPHYR_LOG ("enter\n") ;

    if (!a_this)
        return ;
    if (a_this->host_adaptors) {
        ephyrHostXVAdaptorArrayDelete (a_this->host_adaptors) ;
        a_this->host_adaptors = NULL ;
    }
    if (a_this->adaptors) {
        xfree (a_this->adaptors) ;
        a_this->adaptors = NULL ;
    }
    xfree (a_this) ;
    EPHYR_LOG ("leave\n") ;
}

static KdVideoEncodingPtr
videoEncodingDup (EphyrHostEncoding *a_encodings,
                   int a_num_encodings)
{
    KdVideoEncodingPtr result = NULL ;
    int i=0 ;

    EPHYR_RETURN_VAL_IF_FAIL (a_encodings && a_num_encodings, NULL) ;

    result = xcalloc (a_num_encodings, sizeof (KdVideoEncodingRec)) ;
    for (i=0 ; i < a_num_encodings; i++) {
        result[i].id = a_encodings[i].id ;
        result[i].name = strdup (a_encodings[i].name) ;
        result[i].width = a_encodings[i].width ;
        result[i].height = a_encodings[i].height ;
        result[i].rate.numerator = a_encodings[i].rate.numerator ;
        result[i].rate.denominator = a_encodings[i].rate.denominator ;
    }
    return result ;
}

static KdAttributePtr
portAttributesDup (EphyrHostAttribute *a_encodings,
                   int a_num_encodings)
{
    int i=0 ;
    KdAttributePtr result=NULL ;

    EPHYR_RETURN_VAL_IF_FAIL (a_encodings && a_num_encodings, NULL) ;

    result = xcalloc (a_num_encodings, sizeof (KdAttributeRec)) ;
    if (!result) {
        EPHYR_LOG_ERROR ("failed to allocate attributes\n") ;
        return NULL ;
    }
    for (i=0; i < a_num_encodings; i++) {
        result[i].flags = a_encodings[i].flags ;
        result[i].min_value = a_encodings[i].min_value ;
        result[i].max_value = a_encodings[i].max_value ;
        result[i].name = strdup (a_encodings[i].name) ;
    }
    return result ;
}

static Bool
ephyrXVPrivQueryHostAdaptors (EphyrXVPriv *a_this)
{
    EphyrHostXVAdaptor *cur_host_adaptor=NULL ;
    EphyrHostVideoFormat *video_formats=NULL ;
    EphyrHostEncoding *encodings=NULL ;
    EphyrHostAttribute *attributes=NULL ;
    EphyrHostImageFormat *image_formats=NULL ;
    int num_video_formats=0, base_port_id=0,
        num_attributes=0, num_formats=0, i=0;
    unsigned num_encodings=0 ;
    Bool is_ok = FALSE ;

    EPHYR_RETURN_VAL_IF_FAIL (a_this, FALSE) ;

    EPHYR_LOG ("enter\n") ;

    if (!ephyrHostXVQueryAdaptors (&a_this->host_adaptors)) {
        EPHYR_LOG_ERROR ("failed to query host adaptors\n") ;
        goto out ;
    }
    if (a_this->host_adaptors)
        a_this->num_adaptors =
                    ephyrHostXVAdaptorArrayGetSize (a_this->host_adaptors) ;
    if (a_this->num_adaptors < 0) {
        EPHYR_LOG_ERROR ("failed to get number of host adaptors\n") ;
        goto out ;
    }
    EPHYR_LOG ("host has %d adaptors\n", a_this->num_adaptors) ;
    /*
     * copy what we can from adaptors into a_this->adaptors
     */
    if (a_this->num_adaptors) {
        a_this->adaptors = xcalloc (a_this->num_adaptors,
                                    sizeof (KdVideoAdaptorRec)) ;
        if (!a_this->adaptors) {
            EPHYR_LOG_ERROR ("failed to create internal adaptors\n") ;
            goto out ;
        }
    }
    for (i=0; i < a_this->num_adaptors; i++) {
        int j=0 ;
        cur_host_adaptor =
                   ephyrHostXVAdaptorArrayAt (a_this->host_adaptors, i) ;
        if (!cur_host_adaptor)
            continue ;
        a_this->adaptors[i].type =
                        ephyrHostXVAdaptorGetType (cur_host_adaptor) ;
        a_this->adaptors[i].type |= XvWindowMask ;
        a_this->adaptors[i].flags =
                        VIDEO_OVERLAID_IMAGES | VIDEO_CLIP_TO_VIEWPORT;
        if (ephyrHostXVAdaptorGetName (cur_host_adaptor))
            a_this->adaptors[i].name =
                    strdup (ephyrHostXVAdaptorGetName (cur_host_adaptor)) ;
        else
            a_this->adaptors[i].name = strdup ("Xephyr Video Overlay");
        base_port_id = ephyrHostXVAdaptorGetFirstPortID (cur_host_adaptor) ;
        if (base_port_id < 0) {
            EPHYR_LOG_ERROR ("failed to get port id for adaptor %d\n", i) ;
            continue ;
        }
        if (!s_base_port_id)
            s_base_port_id = base_port_id ;

        if (!ephyrHostXVQueryEncodings (base_port_id,
                                        &encodings,
                                        &num_encodings)) {
            EPHYR_LOG_ERROR ("failed to get encodings for port port id %d,"
                             " adaptors %d\n",
                             base_port_id, i) ;
            continue ;
        }
        a_this->adaptors[i].nEncodings = num_encodings ;
        a_this->adaptors[i].pEncodings =
                            videoEncodingDup (encodings, num_encodings) ;
        video_formats = (EphyrHostVideoFormat*)
            ephyrHostXVAdaptorGetVideoFormats (cur_host_adaptor,
                                               &num_video_formats);
        a_this->adaptors[i].pFormats = (KdVideoFormatPtr) video_formats ;
        a_this->adaptors[i].nFormats = num_video_formats ;
        a_this->adaptors[i].nPorts =
                            ephyrHostXVAdaptorGetNbPorts (cur_host_adaptor) ;
        a_this->adaptors[i].pPortPrivates =
                xcalloc (a_this->adaptors[i].nPorts,
                         sizeof (DevUnion) + sizeof (EphyrPortPriv)) ;
        for (j=0; j < a_this->adaptors[i].nPorts; j++) {
            int port_priv_offset = a_this->adaptors[i].nPorts ;
            EphyrPortPriv *port_priv =
                (EphyrPortPriv*)
                    &a_this->adaptors[i].pPortPrivates[port_priv_offset + j];
            port_priv->port_number = base_port_id + j;
            port_priv->current_adaptor = &a_this->adaptors[i] ;
            port_priv->xv_priv = a_this ;
            a_this->adaptors[i].pPortPrivates[j].ptr = port_priv;
        }
        if (!ephyrHostXVQueryPortAttributes (base_port_id,
                                             &attributes,
                                             &num_attributes)) {
            EPHYR_LOG_ERROR ("failed to get port attribute "
                             "for adaptor %d\n", i) ;
            continue ;
        }
        a_this->adaptors[i].pAttributes =
                    portAttributesDup (attributes, num_attributes);
        a_this->adaptors[i].nAttributes = num_attributes ;
        if (!ephyrHostXVQueryImageFormats (base_port_id,
                                           &image_formats,
                                           &num_formats)) {
            EPHYR_LOG_ERROR ("failed to get image formats "
                             "for adaptor %d\n", i) ;
            continue ;
        }
        a_this->adaptors[i].pImages = (KdImagePtr) image_formats ;
        a_this->adaptors[i].nImages = num_formats ;
    }
    is_ok = TRUE ;

out:
    if (encodings) {
        ephyrHostEncodingsDelete (encodings, num_encodings) ;
        encodings = NULL ;
    }
    if (attributes) {
        ephyrHostAttributesDelete (attributes) ;
        attributes = NULL ;
    }
    EPHYR_LOG ("leave\n") ;
    return is_ok ;
}

static Bool
ephyrXVPrivSetAdaptorsHooks (EphyrXVPriv *a_this)
{
    int i=0 ;

    EPHYR_RETURN_VAL_IF_FAIL (a_this, FALSE) ;

    EPHYR_LOG ("enter\n") ;

    for (i=0; i < a_this->num_adaptors; i++) {
        a_this->adaptors[i].StopVideo = ephyrStopVideo ;
        a_this->adaptors[i].SetPortAttribute = ephyrSetPortAttribute ;
        a_this->adaptors[i].GetPortAttribute = ephyrGetPortAttribute ;
        a_this->adaptors[i].QueryBestSize = ephyrQueryBestSize ;
        a_this->adaptors[i].PutImage = ephyrPutImage;
        a_this->adaptors[i].QueryImageAttributes = ephyrQueryImageAttributes ;
    }
    EPHYR_LOG ("leave\n") ;
    return TRUE ;
}

static Bool
ephyrXVPrivRegisterAdaptors (EphyrXVPriv *a_this,
                             ScreenPtr a_screen)
{
    KdScreenPriv(a_screen);
    KdScreenInfo *screen = pScreenPriv->screen;
    Bool is_ok = FALSE ;
    KdVideoAdaptorPtr *adaptors=NULL, *registered_adaptors=NULL ;
    int num_registered_adaptors=0, i=0, num_adaptors=0 ;

    EPHYR_RETURN_VAL_IF_FAIL (a_this && a_screen, FALSE) ;

    EPHYR_LOG ("enter\n") ;

    if (!a_this->num_adaptors)
        goto out ;
    num_registered_adaptors =
                KdXVListGenericAdaptors (screen, &registered_adaptors);
    num_adaptors = num_registered_adaptors + a_this->num_adaptors ;
    adaptors = xcalloc (num_adaptors, sizeof (KdVideoAdaptorPtr)) ;
    if (!adaptors) {
        EPHYR_LOG_ERROR ("failed to allocate adaptors tab\n") ;
        goto out ;
    }
    memmove (adaptors, registered_adaptors, num_registered_adaptors) ;
    for (i=0 ; i < a_this->num_adaptors; i++) {
        *(adaptors + num_registered_adaptors + i) = &a_this->adaptors[i] ;
    }
    if (!KdXVScreenInit (a_screen, adaptors, num_adaptors)) {
        EPHYR_LOG_ERROR ("failed to register adaptors\n");
        goto out ;
    }
    EPHYR_LOG ("registered %d adaptors\n", num_adaptors) ;
    is_ok = TRUE ;

out:
    if (registered_adaptors) {
        xfree (registered_adaptors) ;
        registered_adaptors = NULL ;
    }
    if (adaptors) {
        xfree (adaptors) ;
        adaptors=NULL ;
    }
    EPHYR_LOG ("leave\n") ;
    return is_ok ;
}

static Bool
ephyrXVPrivIsAttrValueValid (KdAttributePtr a_attrs,
                             int a_attrs_len,
                             const char *a_attr_name,
                             int a_attr_value,
                             Bool *a_is_valid)
{
    int i=0 ;

    EPHYR_RETURN_VAL_IF_FAIL (a_attrs && a_attr_name && a_is_valid,
                              FALSE) ;

    for (i=0; i < a_attrs_len; i++) {
        if (a_attrs[i].name && strcmp (a_attrs[i].name, a_attr_name))
            continue ;
        if (a_attrs[i].min_value > a_attr_value ||
            a_attrs[i].max_value < a_attr_value) {
            *a_is_valid = FALSE ;
        } else {
            *a_is_valid = TRUE ;
        }
        return TRUE ;
    }
    return FALSE ;
}

static void
ephyrStopVideo (KdScreenInfo *a_info, pointer a_port_priv, Bool a_exit)
{
    EPHYR_LOG ("enter\n") ;
    EPHYR_LOG ("leave\n") ;
}

static int
ephyrSetPortAttribute (KdScreenInfo *a_info,
                       Atom a_attr_name,
                       int a_attr_value,
                       pointer a_port_priv)
{
    int res=Success, host_atom=0 ;
    EphyrPortPriv *port_priv = a_port_priv ;
    Bool is_attr_valid=FALSE ;

    EPHYR_RETURN_VAL_IF_FAIL (port_priv, BadMatch) ;
    EPHYR_RETURN_VAL_IF_FAIL (port_priv->current_adaptor, BadMatch) ;
    EPHYR_RETURN_VAL_IF_FAIL (port_priv->current_adaptor->pAttributes,
                              BadMatch) ;
    EPHYR_RETURN_VAL_IF_FAIL (port_priv->current_adaptor->nAttributes,
                              BadMatch) ;
    EPHYR_RETURN_VAL_IF_FAIL (ValidAtom (a_attr_name), BadMatch) ;

    EPHYR_LOG ("enter, portnum:%d, atomid:%d, attr_name:%s, attr_val:%d\n",
               port_priv->port_number,
               (int)a_attr_name,
               NameForAtom (a_attr_name),
               a_attr_value) ;

    if (!ephyrLocalAtomToHost (a_attr_name, &host_atom)) {
        EPHYR_LOG_ERROR ("failed to convert local atom to host atom\n") ;
        res = BadMatch ;
        goto out ;
    }

    if (!ephyrXVPrivIsAttrValueValid (port_priv->current_adaptor->pAttributes,
                                      port_priv->current_adaptor->nAttributes,
                                      NameForAtom (a_attr_name),
                                      a_attr_value,
                                      &is_attr_valid)) {
        EPHYR_LOG_ERROR ("failed to validate attribute %s\n",
                         NameForAtom (a_attr_name)) ;
        res = BadMatch ;
        goto out ;
    }
    if (!is_attr_valid) {
        EPHYR_LOG_ERROR ("attribute %s is not valid\n",
                         NameForAtom (a_attr_name)) ;
        res = BadMatch ;
        goto out ;
    }

    if (!ephyrHostXVSetPortAttribute (port_priv->port_number,
                                      host_atom,
                                      a_attr_value)) {
        EPHYR_LOG_ERROR ("failed to set port attribute\n") ;
        res = BadMatch ;
        goto out ;
    }

    res = Success ;
out:
    EPHYR_LOG ("leave\n") ;
    return res ;
}

static int
ephyrGetPortAttribute (KdScreenInfo *a_screen_info,
                       Atom a_attr_name,
                       int *a_attr_value,
                       pointer a_port_priv)
{
    int res=Success, host_atom=0 ;
    EphyrPortPriv *port_priv = a_port_priv ;

    EPHYR_RETURN_VAL_IF_FAIL (port_priv, BadMatch) ;
    EPHYR_RETURN_VAL_IF_FAIL (ValidAtom (a_attr_name), BadMatch) ;

    EPHYR_LOG ("enter, portnum:%d, atomid:%d, attr_name:%s\n",
               port_priv->port_number,
               (int)a_attr_name,
               NameForAtom (a_attr_name)) ;

    if (!ephyrLocalAtomToHost (a_attr_name, &host_atom)) {
        EPHYR_LOG_ERROR ("failed to convert local atom to host atom\n") ;
        res = BadMatch ;
        goto out ;
    }

    if (!ephyrHostXVGetPortAttribute (port_priv->port_number,
                                      host_atom,
                                      a_attr_value)) {
        EPHYR_LOG_ERROR ("failed to get port attribute\n") ;
        res = BadMatch ;
        goto out ;
    }

    res = Success ;
out:
    EPHYR_LOG ("leave\n") ;
    return res ;
}

static void
ephyrQueryBestSize (KdScreenInfo *a_info,
                    Bool a_motion,
                    short a_src_w,
                    short a_src_h,
                    short a_drw_w,
                    short a_drw_h,
                    unsigned int *a_prefered_w,
                    unsigned int *a_prefered_h,
                    pointer a_port_priv)
{
    int res=0 ;
    EphyrPortPriv *port_priv = a_port_priv ;

    EPHYR_RETURN_IF_FAIL (port_priv) ;

    EPHYR_LOG ("enter\n") ;
    res = ephyrHostXVQueryBestSize (port_priv->port_number,
                                    a_motion,
                                    a_src_w, a_src_h,
                                    a_drw_w, a_drw_h,
                                    a_prefered_w, a_prefered_h) ;
    if (!res) {
        EPHYR_LOG_ERROR ("Failed to query best size\n") ;
    }
    EPHYR_LOG ("leave\n") ;
}


static int
ephyrPutImage (KdScreenInfo *a_info,
               DrawablePtr a_drawable,
               short a_src_x,
               short a_src_y,
               short a_drw_x,
               short a_drw_y,
               short a_src_w,
               short a_src_h,
               short a_drw_w,
               short a_drw_h,
               int a_id,
               unsigned char *a_buf,
               short a_width,
               short a_height,
               Bool a_sync,
               RegionPtr a_clipping_region,
               pointer a_port_priv)
{
    EphyrPortPriv *port_priv = a_port_priv ;
    BoxRec clipped_area, dst_box ;
    int result=BadImplementation ;
    int drw_x=0, drw_y=0, drw_w=0, drw_h=0 ;

    EPHYR_RETURN_VAL_IF_FAIL (a_drawable, BadValue) ;

    EPHYR_LOG ("enter\n") ;

    dst_box.x1 = a_drw_x ;
    dst_box.x2 = a_drw_x + a_drw_w;
    dst_box.y1 = a_drw_y ;
    dst_box.y2 = a_drw_y + a_drw_h;

    if (!DoSimpleClip (&dst_box,
                       REGION_EXTENTS (pScreen->pScreen, a_clipping_region),
                       &clipped_area)) {
        EPHYR_LOG_ERROR ("failed to simple clip\n") ;
        goto out ;
    }

    drw_x = clipped_area.x1 ;
    drw_y = clipped_area.y1 ;
    drw_w = clipped_area.x2 - clipped_area.x1 ;
    drw_h = clipped_area.y2 - clipped_area.y1 ;

    if (!ephyrHostXVPutImage (port_priv->port_number,
                              a_id,
                              drw_x, drw_y, drw_w, drw_h,
                              a_src_x, a_src_y, a_src_w, a_src_h,
                              a_width, a_height, a_buf)) {
        EPHYR_LOG_ERROR ("EphyrHostXVPutImage() failed\n") ;
        goto out ;
    }

    result = Success ;

out:
    EPHYR_LOG ("leave\n") ;
    return result ;
}

static int
ephyrQueryImageAttributes (KdScreenInfo *a_info,
                           int a_id,
                           unsigned short *a_w,
                           unsigned short *a_h,
                           int *a_pitches,
                           int *a_offsets)
{
    int image_size=0 ;

    EPHYR_RETURN_VAL_IF_FAIL (a_w && a_h, FALSE) ;

    EPHYR_LOG ("enter: dim (%dx%d), pitches: %#x, offsets: %#x\n",
               *a_w, *a_h, (unsigned int)a_pitches, (unsigned int)a_offsets) ;

   if (!ephyrHostXVQueryImageAttributes (s_base_port_id,
                                         a_id,
                                         a_w, a_h,
                                         &image_size,
                                         a_pitches, a_offsets)) {
       EPHYR_LOG_ERROR ("EphyrHostXVQueryImageAttributes() failed\n") ;
       goto out ;
   }
   EPHYR_LOG ("image size: %d, dim (%dx%d)", image_size, *a_w, *a_h) ;

out:
    EPHYR_LOG ("leave\n") ;
    return image_size ;
}
