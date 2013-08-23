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

typedef struct _EphyrHostRational {
    int numerator;
    int denominator;
} EphyrHostRational;

typedef struct _EphyrHostEncoding {
    int id;
    char *name;
    unsigned short width, height;
    EphyrHostRational rate;
} EphyrHostEncoding;

typedef struct _EphyrHostImageFormat {
    int id;                     /* Unique descriptor for the format */
    int type;                   /* XvRGB, XvYUV */
    int byte_order;             /* LSBFirst, MSBFirst */
    char guid[16];              /* Globally Unique IDentifier */
    int bits_per_pixel;
    int format;                 /* XvPacked, XvPlanar */
    int num_planes;

    /* for RGB formats only */
    int depth;
    unsigned int red_mask;
    unsigned int green_mask;
    unsigned int blue_mask;

    /* for YUV formats only */
    unsigned int y_sample_bits;
    unsigned int u_sample_bits;
    unsigned int v_sample_bits;
    unsigned int horz_y_period;
    unsigned int horz_u_period;
    unsigned int horz_v_period;
    unsigned int vert_y_period;
    unsigned int vert_u_period;
    unsigned int vert_v_period;
    char component_order[32];   /* eg. UYVY */
    int scanline_order;         /* XvTopToBottom, XvBottomToTop */
} EphyrHostImageFormat;

typedef struct {
    unsigned short x1, y1, x2, y2;
} EphyrHostBox;

/*
 * encoding
 */
Bool ephyrHostXVQueryEncodings(int a_port_id,
                               EphyrHostEncoding ** a_encodings,
                               unsigned int *a_num_encodings);

void ephyrHostEncodingsDelete(EphyrHostEncoding * a_encodings,
                              int a_num_encodings);

/*
 * image format
 */

Bool ephyrHostXVQueryImageFormats(int a_port_id,
                                  EphyrHostImageFormat ** a_formats,
                                  int *a_num_format);

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

/*
 *PutImage
 * (ignore clipping for now)
 */
Bool ephyrHostXVPutImage(int a_screen_num,
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
                         EphyrHostBox * a_clip_rects, int a_clip_rect_nums);

/*
 * Putvideo/PutStill/GetVideo
 */
Bool ephyrHostXVPutVideo(int a_screen_num,
                         int a_port_id,
                         int a_vid_x, int a_vid_y, int a_vid_w, int a_vid_h,
                         int a_drw_x, int a_drw_y, int a_drw_w, int a_drw_h);

Bool ephyrHostXVGetVideo(int a_screen_num,
                         int a_port_id,
                         int a_vid_x, int a_vid_y, int a_vid_w, int a_vid_h,
                         int a_drw_x, int a_drw_y, int a_drw_w, int a_drw_h);

Bool ephyrHostXVPutStill(int a_screen_num,
                         int a_port_id,
                         int a_vid_x, int a_vid_y, int a_vid_w, int a_vid_h,
                         int a_drw_x, int a_drw_y, int a_drw_w, int a_drw_h);

Bool ephyrHostXVGetStill(int a_screen_num,
                         int a_port_id,
                         int a_vid_x, int a_vid_y, int a_vid_w, int a_vid_h,
                         int a_drw_x, int a_drw_y, int a_drw_w, int a_drw_h);

#endif /*__EPHYRHOSTVIDEO_H__*/
