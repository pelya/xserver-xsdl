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
#include <xcb/xv.h>
#include <xcb/xcb_aux.h>
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

Bool
ephyrHostXVQueryBestSize(int a_port_id,
                         Bool a_motion,
                         unsigned int a_frame_w,
                         unsigned int a_frame_h,
                         unsigned int a_drw_w,
                         unsigned int a_drw_h,
                         unsigned int *a_actual_w, unsigned int *a_actual_h)
{
    Bool is_ok = FALSE;
    xcb_xv_query_best_size_cookie_t cookie;
    xcb_xv_query_best_size_reply_t *reply;

    EPHYR_RETURN_VAL_IF_FAIL(a_actual_w && a_actual_h, FALSE);

    EPHYR_LOG("enter: frame (%dx%d), drw (%dx%d)\n",
              a_frame_w, a_frame_h, a_drw_w, a_drw_h);

    cookie = xcb_xv_query_best_size(hostx_get_xcbconn(),
                                    a_port_id,
                                    a_frame_w, a_frame_h,
                                    a_drw_w, a_drw_h,
                                    a_motion);
    reply = xcb_xv_query_best_size_reply(hostx_get_xcbconn(), cookie, NULL);
    if (!reply) {
        EPHYR_LOG_ERROR ("XvQueryBestSize() failed\n");
        goto out;
    }
    *a_actual_w = reply->actual_width;
    *a_actual_h = reply->actual_height;
    free(reply);

    EPHYR_LOG("actual (%dx%d)\n", *a_actual_w, *a_actual_h);
    is_ok = TRUE;

out:
    free(reply);
    EPHYR_LOG("leave\n");
    return is_ok;
}

Bool
ephyrHostGetAtom(const char *a_name, Bool a_create_if_not_exists, int *a_atom)
{
    xcb_connection_t *conn = hostx_get_xcbconn();
    xcb_intern_atom_cookie_t cookie;
    xcb_intern_atom_reply_t *reply;

    EPHYR_RETURN_VAL_IF_FAIL(a_atom, FALSE);

    cookie = xcb_intern_atom(conn,
                             a_create_if_not_exists,
                             strlen(a_name),
                             a_name);
    reply = xcb_intern_atom_reply(conn, cookie, NULL);
    if (!reply || reply->atom == None) {
        free(reply);
        return FALSE;
    }
    *a_atom = reply->atom;
    free(reply);
    return TRUE;
}

char *
ephyrHostGetAtomName(int a_atom)
{
    xcb_connection_t *conn = hostx_get_xcbconn();
    xcb_get_atom_name_cookie_t cookie;
    xcb_get_atom_name_reply_t *reply;
    char *ret;

    cookie = xcb_get_atom_name(conn, a_atom);
    reply = xcb_get_atom_name_reply(conn, cookie, NULL);
    if (!reply)
        return NULL;
    ret = malloc(xcb_get_atom_name_name_length(reply) + 1);
    if (ret) {
        memcpy(ret, xcb_get_atom_name_name(reply),
               xcb_get_atom_name_name_length(reply));
        ret[xcb_get_atom_name_name_length(reply)] = '\0';
    }
    free(reply);
    return ret;
}

Bool
ephyrHostXVPutImage(int a_screen_num,
                    int a_port_id,
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
                    unsigned char *a_buf,
                    EphyrHostBox * a_clip_rects, int a_clip_rect_nums)
{
    Bool is_ok = TRUE;
    xcb_connection_t *conn = hostx_get_xcbconn ();
    xcb_gcontext_t gc;
    xcb_rectangle_t *rects = NULL;
    int data_len, width, height;
    xcb_xv_query_image_attributes_cookie_t image_attr_cookie;
    xcb_xv_query_image_attributes_reply_t *image_attr_reply;

    EPHYR_RETURN_VAL_IF_FAIL(a_buf, FALSE);

    EPHYR_LOG("enter, num_clip_rects: %d\n", a_clip_rect_nums);

    gc = xcb_generate_id(conn);
    xcb_create_gc(conn, gc, hostx_get_window(a_screen_num), 0, NULL);

    image_attr_cookie = xcb_xv_query_image_attributes(conn,
                                                      a_port_id,
                                                      a_image_id,
                                                      a_image_width,
                                                      a_image_height);
    image_attr_reply = xcb_xv_query_image_attributes_reply(conn,
                                                           image_attr_cookie,
                                                           NULL);
    if (!image_attr_reply)
        goto out;
    data_len = image_attr_reply->data_size;
    width = image_attr_reply->width;
    height = image_attr_reply->height;
    free(image_attr_reply);

    if (a_clip_rect_nums) {
        int i = 0;
        rects = calloc(a_clip_rect_nums, sizeof(xcb_rectangle_t));
        for (i=0; i < a_clip_rect_nums; i++) {
            rects[i].x = a_clip_rects[i].x1;
            rects[i].y = a_clip_rects[i].y1;
            rects[i].width = a_clip_rects[i].x2 - a_clip_rects[i].x1;
            rects[i].height = a_clip_rects[i].y2 - a_clip_rects[i].y1;
            EPHYR_LOG("(x,y,w,h): (%d,%d,%d,%d)\n",
                      rects[i].x, rects[i].y, rects[i].width, rects[i].height);
        }
        xcb_set_clip_rectangles(conn,
                                XCB_CLIP_ORDERING_YX_BANDED,
                                gc,
                                0,
                                0,
                                a_clip_rect_nums,
                                rects);
	free(rects);
    }
    xcb_xv_put_image(conn,
                     a_port_id,
                     hostx_get_window (a_screen_num),
                     gc,
                     a_image_id,
                     a_src_x, a_src_y, a_src_w, a_src_h,
                     a_drw_x, a_drw_y, a_drw_w, a_drw_h,
                     width, height,
                     data_len, a_buf);
    is_ok = TRUE;

out:
    xcb_free_gc(conn, gc);
    EPHYR_LOG("leave\n");
    return is_ok;
}

Bool
ephyrHostXVPutVideo(int a_screen_num, int a_port_id,
                    int a_vid_x, int a_vid_y, int a_vid_w, int a_vid_h,
                    int a_drw_x, int a_drw_y, int a_drw_w, int a_drw_h)
{
    Bool is_ok = FALSE;
    xcb_gcontext_t gc;
    xcb_connection_t *conn = hostx_get_xcbconn();

    EPHYR_RETURN_VAL_IF_FAIL(conn, FALSE);

    gc = xcb_generate_id(conn);
    xcb_create_gc(conn, gc, hostx_get_window (a_screen_num), 0, NULL);
    xcb_xv_put_video(conn,
                     a_port_id,
                     hostx_get_window (a_screen_num), gc,
                     a_vid_x, a_vid_y, a_vid_w, a_vid_h,
                     a_drw_x, a_drw_y, a_drw_w, a_drw_h);
    is_ok = TRUE;

    xcb_free_gc(conn, gc);

    return is_ok;
}

Bool
ephyrHostXVGetVideo(int a_screen_num, int a_port_id,
                    int a_vid_x, int a_vid_y, int a_vid_w, int a_vid_h,
                    int a_drw_x, int a_drw_y, int a_drw_w, int a_drw_h)
{
    xcb_gcontext_t gc;
    xcb_connection_t *conn = hostx_get_xcbconn();

    EPHYR_RETURN_VAL_IF_FAIL(conn, FALSE);

    gc = xcb_generate_id(conn);
    xcb_create_gc(conn,
                  gc,
                  hostx_get_window (a_screen_num),
                  0, NULL);
    xcb_xv_get_video(conn,
                     a_port_id,
                     hostx_get_window (a_screen_num),
                     gc,
                     a_vid_x, a_vid_y, a_vid_w, a_vid_h,
                     a_drw_x, a_drw_y, a_drw_w, a_drw_h);

    xcb_free_gc(conn, gc);

    return TRUE;
}

Bool
ephyrHostXVPutStill(int a_screen_num, int a_port_id,
                    int a_vid_x, int a_vid_y, int a_vid_w, int a_vid_h,
                    int a_drw_x, int a_drw_y, int a_drw_w, int a_drw_h)
{
    xcb_connection_t *conn = hostx_get_xcbconn();
    xcb_gcontext_t gc;

    EPHYR_RETURN_VAL_IF_FAIL(conn, FALSE);

    gc = xcb_generate_id(conn);
    xcb_create_gc(conn,
                  gc,
                  hostx_get_window (a_screen_num),
                  0, NULL);
    xcb_xv_put_still(conn,
                     a_port_id,
                     hostx_get_window (a_screen_num),
                     gc,
                     a_vid_x, a_vid_y, a_vid_w, a_vid_h,
                     a_drw_x, a_drw_y, a_drw_w, a_drw_h);

    xcb_free_gc(conn, gc);

    return TRUE;
}

Bool
ephyrHostXVGetStill(int a_screen_num, int a_port_id,
                    int a_vid_x, int a_vid_y, int a_vid_w, int a_vid_h,
                    int a_drw_x, int a_drw_y, int a_drw_w, int a_drw_h)
{
    xcb_gcontext_t gc;
    xcb_connection_t *conn = hostx_get_xcbconn();

    EPHYR_RETURN_VAL_IF_FAIL(conn, FALSE);

    gc = xcb_generate_id(conn);
    xcb_create_gc(conn,
                  gc,
                  hostx_get_window (a_screen_num),
                  0, NULL);
    xcb_xv_get_still(conn,
                     a_port_id,
                     hostx_get_window (a_screen_num),
                     gc,
                     a_vid_x, a_vid_y, a_vid_w, a_vid_h,
                     a_drw_x, a_drw_y, a_drw_w, a_drw_h);
    xcb_free_gc(conn, gc);

    return TRUE;
}
