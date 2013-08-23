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
