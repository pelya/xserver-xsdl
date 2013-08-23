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
#ifndef __EPHYRHOSTVIDEO_H__
#define __EPHYRHOSTVIDEO_H__

#include <xcb/xv.h>
#include <X11/Xdefs.h>

/*
 *size query
 */
Bool ephyrHostXVQueryBestSize(int a_port_id,
                              Bool a_motion,
                              unsigned int a_frame_w,
                              unsigned int a_frame_h,
                              unsigned int a_drw_w,
                              unsigned int a_drw_h,
                              unsigned int *a_actual_w,
                              unsigned int *a_actual_h);

/*
 * atom
 */
Bool ephyrHostGetAtom(const char *a_name,
                      Bool a_create_if_not_exists, int *a_atom);
char *ephyrHostGetAtomName(int a_atom);

#endif /*__EPHYRHOSTVIDEO_H__*/
