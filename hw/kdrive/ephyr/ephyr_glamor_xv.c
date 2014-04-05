/*
 * Copyright © 2014 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include <kdrive-config.h>
#endif

#include "kdrive.h"
#include "kxv.h"
#include "ephyr.h"
#include "glamor_priv.h"

#include <X11/extensions/Xv.h>
#include "fourcc.h"

#define NUM_FORMATS 3

static KdVideoFormatRec Formats[NUM_FORMATS] = {
    {15, TrueColor}, {16, TrueColor}, {24, TrueColor}
};

static void
ephyr_glamor_xv_stop_video(KdScreenInfo *screen, void *data, Bool cleanup)
{
    if (!cleanup)
        return;

    glamor_xv_stop_video(data);
}

static int
ephyr_glamor_xv_set_port_attribute(KdScreenInfo *screen,
                                   Atom attribute, INT32 value, void *data)
{
    return glamor_xv_set_port_attribute(data, attribute, value);
}

static int
ephyr_glamor_xv_get_port_attribute(KdScreenInfo *screen,
                                   Atom attribute, INT32 *value, void *data)
{
    return glamor_xv_get_port_attribute(data, attribute, value);
}

static void
ephyr_glamor_xv_query_best_size(KdScreenInfo *screen,
                                Bool motion,
                                short vid_w, short vid_h,
                                short drw_w, short drw_h,
                                unsigned int *p_w, unsigned int *p_h,
                                void *data)
{
    *p_w = drw_w;
    *p_h = drw_h;
}

static int
ephyr_glamor_xv_query_image_attributes(KdScreenInfo *screen,
                                       int id,
                                       unsigned short *w, unsigned short *h,
                                       int *pitches, int *offsets)
{
    return glamor_xv_query_image_attributes(id, w, h, pitches, offsets);
}

static int
ephyr_glamor_xv_put_image(KdScreenInfo *screen,
                          DrawablePtr pDrawable,
                          short src_x, short src_y,
                          short drw_x, short drw_y,
                          short src_w, short src_h,
                          short drw_w, short drw_h,
                          int id,
                          unsigned char *buf,
                          short width,
                          short height,
                          Bool sync,
                          RegionPtr clipBoxes, void *data)
{
    ScreenPtr pScreen = pDrawable->pScreen;
    glamor_port_private *port_priv = (glamor_port_private *) data;
    int srcPitch, srcPitch2;
    int top, nlines;
    int s2offset, s3offset, tmp;

    s2offset = s3offset = srcPitch2 = 0;

    srcPitch = width;
    srcPitch2 = width >> 1;

    if (!port_priv->src_pix[0] ||
        (width != port_priv->src_pix_w || height != port_priv->src_pix_h)) {
        int i;

        for (i = 0; i < 3; i++)
            if (port_priv->src_pix[i])
                glamor_destroy_pixmap(port_priv->src_pix[i]);

        port_priv->src_pix[0] =
            glamor_create_pixmap(pScreen, width, height, 8, 0);
        port_priv->src_pix[1] =
            glamor_create_pixmap(pScreen, width >> 1, height >> 1, 8, 0);
        port_priv->src_pix[2] =
            glamor_create_pixmap(pScreen, width >> 1, height >> 1, 8, 0);
        port_priv->src_pix_w = width;
        port_priv->src_pix_h = height;

        if (!port_priv->src_pix[0] || !port_priv->src_pix[1] ||
            !port_priv->src_pix[2])
            return BadAlloc;
    }

    top = (src_y) & ~1;
    nlines = (src_y + height) - top;

    switch (id) {
    case FOURCC_YV12:
    case FOURCC_I420:
        s2offset = srcPitch * height;
        s3offset = s2offset + (srcPitch2 * ((height + 1) >> 1));
        s2offset += ((top >> 1) * srcPitch2);
        s3offset += ((top >> 1) * srcPitch2);
        if (id == FOURCC_YV12) {
            tmp = s2offset;
            s2offset = s3offset;
            s3offset = tmp;
        }
        glamor_upload_sub_pixmap_to_texture(port_priv->src_pix[0],
                                            0, 0, srcPitch, nlines,
                                            port_priv->src_pix[0]->devKind,
                                            buf + (top * srcPitch), 0);

        glamor_upload_sub_pixmap_to_texture(port_priv->src_pix[1],
                                            0, 0, srcPitch2, (nlines + 1) >> 1,
                                            port_priv->src_pix[1]->devKind,
                                            buf + s2offset, 0);

        glamor_upload_sub_pixmap_to_texture(port_priv->src_pix[2],
                                            0, 0, srcPitch2, (nlines + 1) >> 1,
                                            port_priv->src_pix[2]->devKind,
                                            buf + s3offset, 0);
        break;
    default:
        return BadMatch;
    }

    if (pDrawable->type == DRAWABLE_WINDOW)
        port_priv->pPixmap = pScreen->GetWindowPixmap((WindowPtr) pDrawable);
    else
        port_priv->pPixmap = (PixmapPtr) pDrawable;

    if (!RegionEqual(&port_priv->clip, clipBoxes)) {
        RegionCopy(&port_priv->clip, clipBoxes);
    }

    port_priv->src_x = src_x;
    port_priv->src_y = src_y;
    port_priv->src_w = src_w;
    port_priv->src_h = src_h;
    port_priv->dst_w = drw_w;
    port_priv->dst_h = drw_h;
    port_priv->drw_x = drw_x;
    port_priv->drw_y = drw_y;
    port_priv->w = width;
    port_priv->h = height;
    port_priv->pDraw = pDrawable;
    glamor_xv_render(port_priv);
    return Success;
}

void
ephyr_glamor_xv_init(ScreenPtr screen)
{
    KdVideoAdaptorRec *adaptor;
    glamor_port_private *port_privates;
    KdVideoEncodingRec encoding = {
        0,
        "XV_IMAGE",
        /* These sizes should probably be GL_MAX_TEXTURE_SIZE instead
         * of 2048, but our context isn't set up yet.
         */
        2048, 2048,
        {1, 1}
    };
    int i;

    glamor_xv_core_init(screen);

    adaptor = xnfcalloc(1, sizeof(*adaptor));

    adaptor->name = "glamor textured video";
    adaptor->type = XvWindowMask | XvInputMask | XvImageMask;
    adaptor->flags = 0;
    adaptor->nEncodings = 1;
    adaptor->pEncodings = &encoding;

    adaptor->pFormats = Formats;
    adaptor->nFormats = NUM_FORMATS;

    adaptor->nPorts = 16; /* Some absurd number */
    port_privates = xnfcalloc(adaptor->nPorts,
                              sizeof(glamor_port_private));
    adaptor->pPortPrivates = xnfcalloc(adaptor->nPorts,
                                       sizeof(glamor_port_private *));
    for (i = 0; i < adaptor->nPorts; i++) {
        adaptor->pPortPrivates[i].ptr = &port_privates[i];
        glamor_xv_init_port(&port_privates[i]);
    }

    adaptor->pAttributes = glamor_xv_attributes;
    adaptor->nAttributes = glamor_xv_num_attributes;

    adaptor->pImages = glamor_xv_images;
    adaptor->nImages = glamor_xv_num_images;

    adaptor->StopVideo = ephyr_glamor_xv_stop_video;
    adaptor->SetPortAttribute = ephyr_glamor_xv_set_port_attribute;
    adaptor->GetPortAttribute = ephyr_glamor_xv_get_port_attribute;
    adaptor->QueryBestSize = ephyr_glamor_xv_query_best_size;
    adaptor->PutImage = ephyr_glamor_xv_put_image;
    adaptor->QueryImageAttributes = ephyr_glamor_xv_query_image_attributes;

    KdXVScreenInit(screen, adaptor, 1);
}
