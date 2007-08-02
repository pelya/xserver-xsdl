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
    EphyrXVPriv *xv_priv;
};
typedef struct _EphyrPortPriv EphyrPortPriv ;

static Bool EphyrLocalAtomToHost (int a_local_atom, int *a_host_atom) ;

static Bool EphyrHostAtomToLocal (int a_host_atom, int *a_local_atom) ;

static EphyrXVPriv* EphyrXVPrivNew (void) ;
static void EphyrXVPrivDelete (EphyrXVPriv *a_this) ;
static Bool EphyrXVPrivQueryHostAdaptors (EphyrXVPriv *a_this) ;
static Bool EphyrXVPrivSetAdaptorsHooks (EphyrXVPriv *a_this) ;
static Bool EphyrXVPrivRegisterAdaptors (EphyrXVPriv *a_this,
                                         ScreenPtr a_screen) ;
static void EphyrStopVideo (KdScreenInfo *a_info,
                            pointer a_xv_priv,
                            Bool a_exit);

static int EphyrSetPortAttribute (KdScreenInfo *a_info,
                                  Atom a_attr_name,
                                  int a_attr_value,
                                  pointer a_port_priv);

static int EphyrGetPortAttribute (KdScreenInfo *a_screen_info,
                                  Atom a_attr_name,
                                  int *a_attr_value,
                                  pointer a_port_priv);

static void EphyrQueryBestSize (KdScreenInfo *a_info,
                                Bool a_motion,
                                short a_src_w,
                                short a_src_h,
                                short a_drw_w,
                                short a_drw_h,
                                unsigned int *a_prefered_w,
                                unsigned int *a_prefered_h,
                                pointer a_port_priv);

static int EphyrPutImage (KdScreenInfo *a_info,
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

static int EphyrQueryImageAttributes (KdScreenInfo *a_info,
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
EphyrLocalAtomToHost (int a_local_atom, int *a_host_atom)
{
    char *atom_name=NULL;
    int host_atom=None ;

    EPHYR_RETURN_VAL_IF_FAIL (a_host_atom, FALSE) ;

    if (!ValidAtom (a_local_atom))
        return FALSE ;

    atom_name = NameForAtom (a_local_atom) ;

    if (!atom_name)
        return FALSE ;

    if (!EphyrHostGetAtom (atom_name, FALSE, &host_atom) || host_atom == None) {
        EPHYR_LOG_ERROR ("no atom for string %s defined in host X\n",
                         atom_name) ;
        return FALSE ;
    }
    *a_host_atom = host_atom ;
    return TRUE ;
}

static Bool
EphyrHostAtomToLocal (int a_host_atom, int *a_local_atom)
{
    Bool is_ok=FALSE ;
    char *atom_name=NULL ;
    int atom=None ;

    EPHYR_RETURN_VAL_IF_FAIL (a_local_atom, FALSE) ;

    atom_name = EphyrHostGetAtomName (a_host_atom) ;
    if (!atom_name)
        goto out ;

    atom = MakeAtom (atom_name, strlen (atom_name), TRUE) ;
    if (atom == None)
        goto out ;

    *a_local_atom = atom ;
    is_ok = TRUE ;

out:
    if (atom_name) {
        EphyrHostFree (atom_name) ;
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

    xv_priv = EphyrXVPrivNew () ;
    if (!xv_priv) {
        EPHYR_LOG_ERROR ("failed to create xv_priv\n") ;
        return FALSE ;
    }

    if (!EphyrXVPrivRegisterAdaptors (xv_priv, pScreen)) {
        EPHYR_LOG_ERROR ("failed to register adaptors\n") ;
        return FALSE ;
    }
    return TRUE ;
}

static EphyrXVPriv*
EphyrXVPrivNew (void)
{
    EphyrXVPriv *xv_priv=NULL ;

    EPHYR_LOG ("enter\n") ;

    xv_priv = xcalloc (1, sizeof (EphyrXVPriv)) ;
    if (!xv_priv) {
        EPHYR_LOG_ERROR ("failed to create EphyrXVPriv\n") ;
        goto error ;
    }
    if (!EphyrXVPrivQueryHostAdaptors (xv_priv)) {
        EPHYR_LOG_ERROR ("failed to query the host x for xv properties\n") ;
        goto error ;
    }
    if (!EphyrXVPrivSetAdaptorsHooks (xv_priv)) {
        EPHYR_LOG_ERROR ("failed to set xv_priv hooks\n") ;
        goto error ;
    }

    EPHYR_LOG ("leave\n") ;
    return xv_priv ;

error:
    if (xv_priv) {
        EphyrXVPrivDelete (xv_priv) ;
        xv_priv = NULL ;
    }
    return NULL ;
}

static void
EphyrXVPrivDelete (EphyrXVPriv *a_this)
{
    EPHYR_LOG ("enter\n") ;

    if (!a_this)
        return ;
    if (a_this->host_adaptors) {
        EphyrHostXVAdaptorArrayDelete (a_this->host_adaptors) ;
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
EphyrXVPrivQueryHostAdaptors (EphyrXVPriv *a_this)
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

    if (!EphyrHostXVQueryAdaptors (&a_this->host_adaptors)) {
        EPHYR_LOG_ERROR ("failed to query host adaptors\n") ;
        goto out ;
    }
    if (a_this->host_adaptors)
        a_this->num_adaptors =
                    EphyrHostXVAdaptorArrayGetSize (a_this->host_adaptors) ;
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
                   EphyrHostXVAdaptorArrayAt (a_this->host_adaptors, i) ;
        if (!cur_host_adaptor)
            continue ;
        a_this->adaptors[i].type =
                        EphyrHostXVAdaptorGetType (cur_host_adaptor) ;
        a_this->adaptors[i].type |= XvWindowMask ;
        a_this->adaptors[i].flags =
                        VIDEO_OVERLAID_IMAGES | VIDEO_CLIP_TO_VIEWPORT;
        if (EphyrHostXVAdaptorGetName (cur_host_adaptor))
            a_this->adaptors[i].name =
                    strdup (EphyrHostXVAdaptorGetName (cur_host_adaptor)) ;
        else
            a_this->adaptors[i].name = strdup ("Xephyr Video Overlay");
        base_port_id = EphyrHostXVAdaptorGetFirstPortID (cur_host_adaptor) ;
        if (base_port_id < 0) {
            EPHYR_LOG_ERROR ("failed to get port id for adaptor %d\n", i) ;
            continue ;
        }
        if (!s_base_port_id)
            s_base_port_id = base_port_id ;

        if (!EphyrHostXVQueryEncodings (base_port_id,
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
            EphyrHostXVAdaptorGetVideoFormats (cur_host_adaptor,
                                               &num_video_formats);
        a_this->adaptors[i].pFormats = (KdVideoFormatPtr) video_formats ;
        a_this->adaptors[i].nFormats = num_video_formats ;
        a_this->adaptors[i].nPorts =
                            EphyrHostXVAdaptorGetNbPorts (cur_host_adaptor) ;
        a_this->adaptors[i].pPortPrivates =
                xcalloc (a_this->adaptors[i].nPorts,
                         sizeof (DevUnion) + sizeof (EphyrPortPriv)) ;
        for (j=0; j < a_this->adaptors[i].nPorts; j++) {
            int port_priv_offset = a_this->adaptors[i].nPorts ;
            EphyrPortPriv *port_priv =
                (EphyrPortPriv*)
                    &a_this->adaptors[i].pPortPrivates[port_priv_offset + j];
            port_priv->port_number = base_port_id + j;
            port_priv->xv_priv = a_this ;
            a_this->adaptors[i].pPortPrivates[j].ptr = port_priv;
        }
        if (!EphyrHostXVQueryPortAttributes (base_port_id,
                                             &attributes,
                                             &num_attributes)) {
            EPHYR_LOG_ERROR ("failed to get port attribute "
                             "for adaptor %d\n", i) ;
            continue ;
        }
        a_this->adaptors[i].pAttributes =
                    portAttributesDup (attributes, num_attributes);
        a_this->adaptors[i].nAttributes = num_attributes ;
        if (!EphyrHostXVQueryImageFormats (base_port_id,
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
        EphyrHostEncodingsDelete (encodings, num_encodings) ;
        encodings = NULL ;
    }
    if (attributes) {
        EphyrHostAttributesDelete (attributes) ;
        attributes = NULL ;
    }
    EPHYR_LOG ("leave\n") ;
    return is_ok ;
}

static Bool
EphyrXVPrivSetAdaptorsHooks (EphyrXVPriv *a_this)
{
    int i=0 ;

    EPHYR_RETURN_VAL_IF_FAIL (a_this, FALSE) ;

    EPHYR_LOG ("enter\n") ;

    for (i=0; i < a_this->num_adaptors; i++) {
        a_this->adaptors[i].StopVideo = EphyrStopVideo ;
        a_this->adaptors[i].SetPortAttribute = EphyrSetPortAttribute ;
        a_this->adaptors[i].GetPortAttribute = EphyrGetPortAttribute ;
        a_this->adaptors[i].QueryBestSize = EphyrQueryBestSize ;
        a_this->adaptors[i].PutImage = EphyrPutImage;
        a_this->adaptors[i].QueryImageAttributes = EphyrQueryImageAttributes ;
    }
    EPHYR_LOG ("leave\n") ;
    return TRUE ;
}

static Bool
EphyrXVPrivRegisterAdaptors (EphyrXVPriv *a_this,
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

static void
EphyrStopVideo (KdScreenInfo *a_info, pointer a_port_priv, Bool a_exit)
{
    EPHYR_LOG ("enter\n") ;
    EPHYR_LOG ("leave\n") ;
}

static int
EphyrSetPortAttribute (KdScreenInfo *a_info,
                       Atom a_attr_name,
                       int a_attr_value,
                       pointer a_port_priv)
{
    int res=Success, host_atom=0 ;
    EphyrPortPriv *port_priv = a_port_priv ;

    EPHYR_RETURN_VAL_IF_FAIL (port_priv, BadMatch) ;
    EPHYR_RETURN_VAL_IF_FAIL (ValidAtom (a_attr_name), BadMatch) ;

    EPHYR_LOG ("enter, portnum:%d, atomid:%d, attr_name:%s, attr_val:%d\n",
               port_priv->port_number,
               (int)a_attr_name,
               NameForAtom (a_attr_name),
               a_attr_value) ;

    if (!EphyrLocalAtomToHost (a_attr_name, &host_atom)) {
        EPHYR_LOG_ERROR ("failed to convert local atom to host atom\n") ;
        res = BadMatch ;
        goto out ;
    }

    if (!EphyrHostXVSetPortAttribute (port_priv->port_number,
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
EphyrGetPortAttribute (KdScreenInfo *a_screen_info,
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

    if (!EphyrLocalAtomToHost (a_attr_name, &host_atom)) {
        EPHYR_LOG_ERROR ("failed to convert local atom to host atom\n") ;
        res = BadMatch ;
        goto out ;
    }

    if (!EphyrHostXVGetPortAttribute (port_priv->port_number,
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
EphyrQueryBestSize (KdScreenInfo *a_info,
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
    res = EphyrHostXVQueryBestSize (port_priv->port_number,
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
EphyrPutImage (KdScreenInfo *a_info,
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
    EPHYR_LOG ("enter\n") ;
    return 0 ;
    EPHYR_LOG ("leave\n") ;
}

static int
EphyrQueryImageAttributes (KdScreenInfo *a_info,
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

   if (!EphyrHostXVQueryImageAttributes (s_base_port_id,
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
