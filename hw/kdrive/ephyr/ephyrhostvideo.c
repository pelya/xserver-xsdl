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
#include <X11/extensions/Xvlib.h>

#include "hostx.h"
#include "ephyrhostvideo.h"
#include "ephyrlog.h"

#ifndef TRUE
#define TRUE 1
#endif /*TRUE*/

#ifndef FALSE
#define FALSE 0
#endif /*FALSE*/

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

    result = Xcalloc (sizeof (EphyrHostXVAdaptorArray)) ;
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
    formats = Xcalloc (nb_formats * sizeof (EphyrHostVideoFormat)) ;
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
        encodings = Xcalloc (num_encodings * sizeof (EphyrHostEncoding)) ;
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

    *a_attributes = (EphyrHostAttribute*)XvQueryPortAttributes (hostx_get_display (),
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

    result = XvListImageFormats (hostx_get_display (), a_port_id, a_num_format) ;
    *a_formats = (EphyrHostImageFormat*) result ;
    return TRUE ;

}
