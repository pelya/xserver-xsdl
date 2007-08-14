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
#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <X11/Xutil.h>
#include <X11/Xlibint.h>
/*#define _XF86DRI_SERVER_*/
#include <GL/glx.h>
#include <X11/dri/xf86dri.h>
#include "hostx.h"
#include "ephyrdri.h"
#define _HAVE_XALLOC_DECLS
#include "ephyrlog.h"

#ifndef TRUE
#define TRUE 1
#endif /*TRUE*/

#ifndef FALSE
#define FALSE 0
#endif /*FALSE*/

Bool
ephyrDRIQueryDirectRenderingCapable (int a_screen, Bool *a_is_capable)
{
    Display *dpy=hostx_get_display () ;
    Bool is_ok=FALSE ;

    EPHYR_LOG ("enter\n") ;
    is_ok = XF86DRIQueryDirectRenderingCapable (dpy, a_screen, a_is_capable) ;
    EPHYR_LOG ("leave\n") ;

    return is_ok ;
}

Bool
ephyrDRIOpenConnection (int a_screen, drm_handle_t *a_sarea, char **a_bus_id_string)
{
    Display *dpy = hostx_get_display () ;
    Bool is_ok=FALSE ;

    EPHYR_LOG ("enter\n") ;
    is_ok = XF86DRIOpenConnection (dpy, a_screen, a_sarea, a_bus_id_string) ;
    EPHYR_LOG ("leave\n") ;
    return is_ok ;
}

Bool
ephyrDRIAuthConnection (int a_screen, drm_magic_t a_magic)
{
    Display *dpy = hostx_get_display () ;
    Bool is_ok=FALSE ;

    EPHYR_LOG ("enter\n") ;
    is_ok = XF86DRIAuthConnection (dpy, a_screen, a_magic) ;
    EPHYR_LOG ("leave\n") ;
    return is_ok ;
}

Bool
ephyrDRICloseConnection (int a_screen)
{
    Display *dpy = hostx_get_display () ;
    Bool is_ok=FALSE ;

    EPHYR_LOG ("enter\n") ;
    is_ok = XF86DRICloseConnection (dpy, a_screen) ;
    EPHYR_LOG ("leave\n") ;
    return is_ok ;
}

Bool
ephyrDRIGetClientDriverName (int a_screen,
                             int *a_ddx_driver_major_version,
                             int *a_ddx_driver_minor_version,
                             int *a_ddx_driver_patch_version,
                             char ** a_client_driver_name)
{
    Display *dpy = hostx_get_display () ;
    Bool is_ok=FALSE ;

    EPHYR_LOG ("enter\n") ;
    is_ok = XF86DRIGetClientDriverName (dpy, a_screen,
                                        a_ddx_driver_major_version,
                                        a_ddx_driver_minor_version,
                                        a_ddx_driver_patch_version,
                                        a_client_driver_name) ;
    EPHYR_LOG ("leave\n") ;
    return is_ok ;
}

Bool
ephyrDRICreateContext (int a_screen,
                       int a_visual_id,
                       unsigned long int *a_returned_ctxt_id,
                       drm_context_t *a_hw_ctxt)
{
    Display *dpy = hostx_get_display () ;
    Bool is_ok=FALSE ;
    Visual v;

    EPHYR_LOG ("enter\n") ;
    memset (&v, 0, sizeof (v)) ;
    v.visualid = a_visual_id ;
    is_ok = XF86DRICreateContext (dpy,
                                  a_screen,
                                  &v,
                                  a_returned_ctxt_id,
                                  a_hw_ctxt) ;
    EPHYR_LOG ("leave\n") ;
    return is_ok ;
}

Bool
ephyrDRIDestroyContext (int a_screen,
                        int a_context_id)
{
    Display *dpy = hostx_get_display () ;
    Bool is_ok=FALSE ;

    EPHYR_LOG ("enter\n") ;
    is_ok = XF86DRIDestroyContext (dpy, a_screen, a_context_id) ;
    EPHYR_LOG ("leave\n") ;
    return is_ok ;
}

Bool
ephyrDRICreateDrawable (int a_screen,
                        int a_drawable,
                        drm_drawable_t *a_hw_drawable)
{
    EPHYR_LOG ("enter\n") ;
    return FALSE ;
    EPHYR_LOG ("leave\n") ;
}

Bool
ephyrDRIDestroyDrawable (int a_screen, int a_drawable)
{
    EPHYR_LOG ("enter\n") ;
    return FALSE ;
    EPHYR_LOG ("leave\n") ;
}

Bool
ephyrDRIGetDrawableInfo (int a_screen,
                         int a_drawable,
                         unsigned int *a_index,
                         unsigned int *a_stamp,
                         int *a_x,
                         int *a_y,
                         int *a_w,
                         int *a_h,
                         int *a_num_clip_rects,
                         drm_clip_rect_t **a_clip_rects,
                         int *a_back_x,
                         int *a_back_y,
                         int *num_back_clip_rects,
                         drm_clip_rect_t **a_back_clip_rects)
{
    EPHYR_LOG ("enter\n") ;
    return FALSE ;
    EPHYR_LOG ("leave\n") ;
}

Bool
ephyrDRIGetDeviceInfo (int a_screen,
                       drm_handle_t *a_frame_buffer,
                       int *a_fb_origin,
                       int *a_fb_size,
                       int *a_fb_stride,
                       int *a_dev_private_size,
                       void **a_dev_private)
{
    EPHYR_LOG ("enter\n") ;
    return FALSE ;
    EPHYR_LOG ("leave\n") ;
}

