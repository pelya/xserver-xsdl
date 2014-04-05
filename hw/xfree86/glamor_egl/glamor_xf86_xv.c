/*
 * Copyright © 2013 Red Hat
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
 *
 * Authors:
 *      Dave Airlie <airlied@redhat.com>
 *
 * some code is derived from the xf86-video-ati radeon driver, mainly
 * the calculations.
 */

/** @file glamor_xf86_xv.c
 *
 * This implements the XF86 XV interface, and calls into glamor core
 * for its support of the suspiciously similar XF86 and Kdrive
 * device-dependent XV interfaces.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#define GLAMOR_FOR_XORG
#include "glamor_priv.h"

#include <X11/extensions/Xv.h>
#include "fourcc.h"

#define NUM_FORMATS 3

static XF86VideoFormatRec Formats[NUM_FORMATS] = {
    {15, TrueColor}, {16, TrueColor}, {24, TrueColor}
};

static void
glamor_xf86_xv_stop_video(ScrnInfoPtr pScrn, void *data, Bool cleanup)
{
    if (!cleanup)
        return;

    glamor_xv_stop_video(data);
}

static int
glamor_xf86_xv_set_port_attribute(ScrnInfoPtr pScrn,
                                  Atom attribute, INT32 value, void *data)
{
    return glamor_xv_set_port_attribute(data, attribute, value);
}

static int
glamor_xf86_xv_get_port_attribute(ScrnInfoPtr pScrn,
                                  Atom attribute, INT32 *value, void *data)
{
    return glamor_xv_get_port_attribute(data, attribute, value);
}

static void
glamor_xf86_xv_query_best_size(ScrnInfoPtr pScrn,
                               Bool motion,
                               short vid_w, short vid_h,
                               short drw_w, short drw_h,
                               unsigned int *p_w, unsigned int *p_h, void *data)
{
    *p_w = drw_w;
    *p_h = drw_h;
}

static int
glamor_xf86_xv_query_image_attributes(ScrnInfoPtr pScrn,
                                      int id,
                                      unsigned short *w, unsigned short *h,
                                      int *pitches, int *offsets)
{
    return glamor_xv_query_image_attributes(id, w, h, pitches, offsets);
}

static int
glamor_xf86_xv_put_image(ScrnInfoPtr pScrn,
                    short src_x, short src_y,
                    short drw_x, short drw_y,
                    short src_w, short src_h,
                    short drw_w, short drw_h,
                    int id,
                    unsigned char *buf,
                    short width,
                    short height,
                    Bool sync,
                    RegionPtr clipBoxes, void *data, DrawablePtr pDrawable)
{
    ScreenPtr screen = pDrawable->pScreen;
    glamor_port_private *port_priv = (glamor_port_private *) data;
    INT32 x1, x2, y1, y2;
    int srcPitch, srcPitch2;
    BoxRec dstBox;
    int top, nlines;
    int s2offset, s3offset, tmp;

    s2offset = s3offset = srcPitch2 = 0;

    /* Clip */
    x1 = src_x;
    x2 = src_x + src_w;
    y1 = src_y;
    y2 = src_y + src_h;

    dstBox.x1 = drw_x;
    dstBox.x2 = drw_x + drw_w;
    dstBox.y1 = drw_y;
    dstBox.y2 = drw_y + drw_h;
    if (!xf86XVClipVideoHelper
        (&dstBox, &x1, &x2, &y1, &y2, clipBoxes, width, height))
        return Success;

    if ((x1 >= x2) || (y1 >= y2))
        return Success;

    srcPitch = width;
    srcPitch2 = width >> 1;

    if (!port_priv->src_pix[0] ||
        (width != port_priv->src_pix_w || height != port_priv->src_pix_h)) {
        int i;

        for (i = 0; i < 3; i++)
            if (port_priv->src_pix[i])
                glamor_destroy_pixmap(port_priv->src_pix[i]);

        port_priv->src_pix[0] =
            glamor_create_pixmap(screen, width, height, 8, 0);
        port_priv->src_pix[1] =
            glamor_create_pixmap(screen, width >> 1, height >> 1, 8, 0);
        port_priv->src_pix[2] =
            glamor_create_pixmap(screen, width >> 1, height >> 1, 8, 0);
        port_priv->src_pix_w = width;
        port_priv->src_pix_h = height;

        if (!port_priv->src_pix[0] || !port_priv->src_pix[1] ||
            !port_priv->src_pix[2])
            return BadAlloc;
    }

    top = (y1 >> 16) & ~1;
    nlines = ((y2 + 0xffff) >> 16) - top;

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
        port_priv->pPixmap = (*screen->GetWindowPixmap) ((WindowPtr) pDrawable);
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

static XF86VideoEncodingRec DummyEncodingGLAMOR[1] = {
    {
     0,
     "XV_IMAGE",
     8192, 8192,
     {1, 1}
     }
};

XF86VideoAdaptorPtr
glamor_xv_init(ScreenPtr screen, int num_texture_ports)
{
    glamor_port_private *port_priv;
    XF86VideoAdaptorPtr adapt;
    int i;

    glamor_xv_core_init(screen);

    adapt = calloc(1, sizeof(XF86VideoAdaptorRec) + num_texture_ports *
                   (sizeof(glamor_port_private) + sizeof(DevUnion)));
    if (adapt == NULL)
        return NULL;

    adapt->type = XvWindowMask | XvInputMask | XvImageMask;
    adapt->flags = 0;
    adapt->name = "GLAMOR Textured Video";
    adapt->nEncodings = 1;
    adapt->pEncodings = DummyEncodingGLAMOR;

    adapt->nFormats = NUM_FORMATS;
    adapt->pFormats = Formats;
    adapt->nPorts = num_texture_ports;
    adapt->pPortPrivates = (DevUnion *) (&adapt[1]);

    adapt->pAttributes = glamor_xv_attributes;
    adapt->nAttributes = glamor_xv_num_attributes;

    port_priv =
        (glamor_port_private *) (&adapt->pPortPrivates[num_texture_ports]);
    adapt->pImages = glamor_xv_images;
    adapt->nImages = glamor_xv_num_images;
    adapt->PutVideo = NULL;
    adapt->PutStill = NULL;
    adapt->GetVideo = NULL;
    adapt->GetStill = NULL;
    adapt->StopVideo = glamor_xf86_xv_stop_video;
    adapt->SetPortAttribute = glamor_xf86_xv_set_port_attribute;
    adapt->GetPortAttribute = glamor_xf86_xv_get_port_attribute;
    adapt->QueryBestSize = glamor_xf86_xv_query_best_size;
    adapt->PutImage = glamor_xf86_xv_put_image;
    adapt->ReputImage = NULL;
    adapt->QueryImageAttributes = glamor_xf86_xv_query_image_attributes;

    for (i = 0; i < num_texture_ports; i++) {
        glamor_port_private *pPriv = &port_priv[i];

        pPriv->brightness = 0;
        pPriv->contrast = 0;
        pPriv->saturation = 0;
        pPriv->hue = 0;
        pPriv->gamma = 1000;
        pPriv->transform_index = 0;

        REGION_NULL(pScreen, &pPriv->clip);

        adapt->pPortPrivates[i].ptr = (void *) (pPriv);
    }
    return adapt;
}
