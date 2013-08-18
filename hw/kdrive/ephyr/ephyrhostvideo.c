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
ephyrHostXVQueryAdaptors (xcb_xv_query_adaptors_reply_t **a_adaptors)
{
    Bool is_ok = FALSE;
    xcb_connection_t *conn = hostx_get_xcbconn();
    xcb_xv_query_adaptors_cookie_t cookie;
    xcb_xv_query_adaptors_reply_t *reply = NULL;
    xcb_generic_error_t *e = NULL;

    EPHYR_RETURN_VAL_IF_FAIL(a_adaptors, FALSE);

    EPHYR_LOG("enter\n");

    cookie = xcb_xv_query_adaptors(conn,
		    xcb_aux_get_screen(conn, hostx_get_screen())->root);
    reply = xcb_xv_query_adaptors_reply(hostx_get_xcbconn(), cookie, &e);
    if (e) {
        EPHYR_LOG_ERROR ("failed to query host adaptors: %d\n", e->error_code);
        goto out;
    }
    *a_adaptors = reply;
    is_ok = TRUE;

out:
    EPHYR_LOG("leave\n");
    free(e);
    return is_ok;
}

xcb_xv_adaptor_info_t *
ephyrHostXVAdaptorArrayAt(const xcb_xv_query_adaptors_reply_t *a_this,
                          int a_index)
{
    int i;
    xcb_xv_adaptor_info_iterator_t it;
    EPHYR_RETURN_VAL_IF_FAIL(a_this, NULL);

    it = xcb_xv_query_adaptors_info_iterator(a_this);
    if (a_index >= a_this->num_adaptors)
        return NULL;
    for (i = 0; i < a_index; i++)
        xcb_xv_adaptor_info_next(&it);

    return it.data;
}

char
ephyrHostXVAdaptorGetType(const xcb_xv_adaptor_info_t *a_this)
{
    EPHYR_RETURN_VAL_IF_FAIL(a_this, -1);
    return a_this->type;
}

char *
ephyrHostXVAdaptorGetName(const xcb_xv_adaptor_info_t *a_this)
{
    char *name;

    EPHYR_RETURN_VAL_IF_FAIL(a_this, NULL);

    name = malloc(a_this->name_size + 1);
    if (!name)
        return NULL;
    memcpy(name, xcb_xv_adaptor_info_name(a_this), a_this->name_size);
    name[a_this->name_size] = '\0';

    return name;
}

EphyrHostVideoFormat*
ephyrHostXVAdaptorGetVideoFormats (const xcb_xv_adaptor_info_t *a_this,
                                   int *a_nb_formats)
{
    EphyrHostVideoFormat *formats = NULL;
    int nb_formats = 0, i = 0;
    xcb_xv_format_t *format = xcb_xv_adaptor_info_formats(a_this);

    EPHYR_RETURN_VAL_IF_FAIL(a_this, NULL);

    nb_formats = a_this->num_formats;
    formats = calloc(nb_formats, sizeof(EphyrHostVideoFormat));
    for (i = 0; i < nb_formats; i++) {
        xcb_visualtype_t *visual =
            xcb_aux_find_visual_by_id(
                    xcb_aux_get_screen(hostx_get_xcbconn(), hostx_get_screen()),
                    format[i].visual);
        formats[i].depth = format[i].depth;
        formats[i].visual_class = visual->_class;
    }
    if (a_nb_formats)
        *a_nb_formats = nb_formats;
    return formats;
}

Bool
ephyrHostXVQueryEncodings(int a_port_id,
                          EphyrHostEncoding ** a_encodings,
                          unsigned int *a_num_encodings)
{
    EphyrHostEncoding *encodings = NULL;
    xcb_xv_encoding_info_iterator_t encoding_info;
    xcb_xv_query_encodings_cookie_t cookie;
    xcb_xv_query_encodings_reply_t *reply;
    unsigned int num_encodings = 0, i;

    EPHYR_RETURN_VAL_IF_FAIL(a_encodings && a_num_encodings, FALSE);

    cookie = xcb_xv_query_encodings(hostx_get_xcbconn(), a_port_id);
    reply = xcb_xv_query_encodings_reply(hostx_get_xcbconn(), cookie, NULL);
    if (!reply)
        return FALSE;
    num_encodings = reply->num_encodings;
    encoding_info = xcb_xv_query_encodings_info_iterator(reply);
    if (num_encodings) {
        encodings = calloc(num_encodings, sizeof (EphyrHostEncoding));
        for (i=0; i<num_encodings; i++, xcb_xv_encoding_info_next(&encoding_info)) {
            encodings[i].id = encoding_info.data->encoding;
            encodings[i].name = malloc(encoding_info.data->name_size + 1);
	    memcpy(encodings[i].name, xcb_xv_encoding_info_name(encoding_info.data), encoding_info.data->name_size);
	    encodings[i].name[encoding_info.data->name_size] = '\0';
            encodings[i].width = encoding_info.data->width;
            encodings[i].height = encoding_info.data->height;
            encodings[i].rate.numerator = encoding_info.data->rate.numerator;
            encodings[i].rate.denominator = encoding_info.data->rate.denominator;
        }
    }
    free(reply);
    *a_encodings = encodings;
    *a_num_encodings = num_encodings;

    return TRUE;
}

void
ephyrHostEncodingsDelete(EphyrHostEncoding * a_encodings, int a_num_encodings)
{
    int i = 0;

    if (!a_encodings)
        return;
    for (i = 0; i < a_num_encodings; i++) {
        free(a_encodings[i].name);
        a_encodings[i].name = NULL;
    }
    free(a_encodings);
}

void
ephyrHostAttributesDelete(xcb_xv_query_port_attributes_reply_t *a_attributes)
{
    free(a_attributes);
}

Bool
ephyrHostXVQueryPortAttributes(int a_port_id,
                               xcb_xv_query_port_attributes_reply_t **a_attributes)
{
    xcb_xv_query_port_attributes_cookie_t cookie;
    xcb_xv_query_port_attributes_reply_t *reply;
    xcb_connection_t *conn = hostx_get_xcbconn();

    EPHYR_RETURN_VAL_IF_FAIL(a_attributes, FALSE);

    cookie = xcb_xv_query_port_attributes(conn, a_port_id);
    reply = xcb_xv_query_port_attributes_reply(conn, cookie, NULL);
    *a_attributes = reply;

    return (reply != NULL);
}

Bool
ephyrHostXVQueryImageFormats(int a_port_id,
                             EphyrHostImageFormat ** a_formats,
                             int *a_num_format)
{
    xcb_connection_t *conn = hostx_get_xcbconn();
    xcb_xv_list_image_formats_cookie_t cookie;
    xcb_xv_list_image_formats_reply_t *reply;
    xcb_xv_image_format_info_t *format;
    EphyrHostImageFormat *ephyrFormats;
    int i;

    EPHYR_RETURN_VAL_IF_FAIL(a_formats && a_num_format, FALSE);

    cookie = xcb_xv_list_image_formats(conn, a_port_id);
    reply = xcb_xv_list_image_formats_reply(conn, cookie, NULL);
    if (!reply)
        return FALSE;
    *a_num_format = reply->num_formats;
    ephyrFormats = calloc(reply->num_formats, sizeof(EphyrHostImageFormat));
    if (!ephyrFormats) {
        free(reply);
        return FALSE;
    }
    format = xcb_xv_list_image_formats_format(reply);
    for (i = 0; i < reply->num_formats; i++) {
        ephyrFormats[i].id = format[i].id;
        ephyrFormats[i].type = format[i].type;
        ephyrFormats[i].byte_order = format[i].byte_order;
        memcpy(ephyrFormats[i].guid, format[i].guid, 16);
        ephyrFormats[i].bits_per_pixel = format[i].bpp;
        ephyrFormats[i].format = format[i].format;
        ephyrFormats[i].num_planes = format[i].num_planes;
        ephyrFormats[i].depth = format[i].depth;
        ephyrFormats[i].red_mask = format[i].red_mask;
        ephyrFormats[i].green_mask = format[i].green_mask;
        ephyrFormats[i].blue_mask = format[i].blue_mask;
        ephyrFormats[i].y_sample_bits = format[i].y_sample_bits;
        ephyrFormats[i].u_sample_bits = format[i].u_sample_bits;
        ephyrFormats[i].v_sample_bits = format[i].v_sample_bits;
        ephyrFormats[i].horz_y_period = format[i].vhorz_y_period;
        ephyrFormats[i].horz_u_period = format[i].vhorz_u_period;
        ephyrFormats[i].horz_v_period = format[i].vhorz_v_period;
        ephyrFormats[i].vert_y_period = format[i].vvert_y_period;
        ephyrFormats[i].vert_u_period = format[i].vvert_u_period;
        ephyrFormats[i].vert_v_period = format[i].vvert_v_period;
        memcpy(ephyrFormats[i].component_order, format[i].vcomp_order, 32);
        ephyrFormats[i].scanline_order = format[i].vscanline_order;
    }
    *a_formats = ephyrFormats;

    free(reply);
    return TRUE;
}

Bool
ephyrHostXVSetPortAttribute(int a_port_id, int a_atom, int a_attr_value)
{
    EPHYR_LOG("atom,value: (%d,%d)\n", a_atom, a_attr_value);

    xcb_xv_set_port_attribute(hostx_get_xcbconn(),
                              a_port_id,
                              a_atom,
                              a_attr_value);
    xcb_flush(hostx_get_xcbconn());
    EPHYR_LOG("leave\n");

    return TRUE;
}

Bool
ephyrHostXVGetPortAttribute(int a_port_id, int a_atom, int *a_attr_value)
{
    Bool ret = FALSE;
    xcb_connection_t *conn = hostx_get_xcbconn();
    xcb_xv_get_port_attribute_cookie_t cookie;
    xcb_xv_get_port_attribute_reply_t *reply;
    xcb_generic_error_t *e;

    EPHYR_RETURN_VAL_IF_FAIL(a_attr_value, FALSE);

    EPHYR_LOG("enter, a_port_id: %d, a_atomid: %d\n", a_port_id, a_atom);

    cookie = xcb_xv_get_port_attribute(conn, a_port_id, a_atom);
    reply = xcb_xv_get_port_attribute_reply(conn, cookie, &e);
    if (e) {
        EPHYR_LOG_ERROR ("XvGetPortAttribute() failed: %d \n", e->error_code);
        free(e);
        goto out;
    }
    *a_attr_value = reply->value;
    EPHYR_LOG("atom,value: (%d, %d)\n", a_atom, *a_attr_value);

    ret = TRUE;

 out:
    EPHYR_LOG("leave\n");
    return ret;
}

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
ephyrHostXVQueryImageAttributes(int a_port_id,
                                int a_image_id /*image fourcc code */ ,
                                unsigned short *a_width,
                                unsigned short *a_height,
                                int *a_image_size,
                                int *a_pitches, int *a_offsets)
{
    xcb_connection_t *conn = hostx_get_xcbconn ();
    xcb_xv_query_image_attributes_cookie_t cookie;
    xcb_xv_query_image_attributes_reply_t *reply;

    EPHYR_RETURN_VAL_IF_FAIL(a_width, FALSE);
    EPHYR_RETURN_VAL_IF_FAIL(a_height, FALSE);
    EPHYR_RETURN_VAL_IF_FAIL(a_image_size, FALSE);

    cookie = xcb_xv_query_image_attributes(conn,
                                           a_port_id, a_image_id,
                                           *a_width, *a_height);
    reply = xcb_xv_query_image_attributes_reply(conn, cookie, NULL);
    if (!reply)
        return FALSE;
    if (a_pitches && a_offsets) {
        memcpy(a_pitches,
               xcb_xv_query_image_attributes_pitches(reply),
               reply->num_planes << 2);
        memcpy(a_offsets,
               xcb_xv_query_image_attributes_offsets(reply),
               reply->num_planes << 2);
    }
    *a_width = reply->width;
    *a_height = reply->height;
    *a_image_size = reply->data_size;
    free(reply);

    return TRUE;
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
