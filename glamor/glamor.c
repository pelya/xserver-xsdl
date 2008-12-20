/*
 * Copyright © 2008 Intel Corporation
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
 *    Eric Anholt <eric@anholt.net>
 *
 */

/** @file glamor.c
 * This file covers the initialization and teardown of glamor, and has various
 * functions not responsible for performing rendering.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stdlib.h>

#include "glamor_priv.h"

static int glamor_screen_private_key_index;
DevPrivateKey glamor_screen_private_key = &glamor_screen_private_key_index;

/**
 * glamor_get_drawable_pixmap() returns a backing pixmap for a given drawable.
 *
 * @param drawable the drawable being requested.
 *
 * This function returns the backing pixmap for a drawable, whether it is a
 * redirected window, unredirected window, or already a pixmap.  Note that
 * coordinate translation is needed when drawing to the backing pixmap of a
 * redirected window, and the translation coordinates are provided by calling
 * exaGetOffscreenPixmap() on the drawable.
 */
PixmapPtr
glamor_get_drawable_pixmap(DrawablePtr drawable)
{
     if (drawable->type == DRAWABLE_WINDOW)
	return drawable->pScreen->GetWindowPixmap((WindowPtr)drawable);
     else
	return (PixmapPtr)drawable;
}

static PixmapPtr
glamor_create_pixmap(ScreenPtr screen, int w, int h, int depth,
		     unsigned int usage_hint)
{
    return fbCreatePixmap(screen, w, h, depth, usage_hint);
}

static Bool
glamor_destroy_pixmap(PixmapPtr pixmap)
{
    return fbDestroyPixmap(pixmap);
}

static void
glamor_block_handler(void *data, OSTimePtr wt, void *last_select_mask)
{
    glFlush();
}

static void
glamor_wakeup_handler(void *data, OSTimePtr wt, void *last_select_mask)
{
    glFlush();
}

/** Set up glamor for an already-configured GL context. */
Bool
glamor_init(ScreenPtr screen)
{
    glamor_screen_private *glamor_priv;

    glamor_priv = xcalloc(1, sizeof(*glamor_priv));
    if (glamor_priv == NULL)
	return FALSE;

    dixSetPrivate(&screen->devPrivates, glamor_screen_private_key, glamor_priv);

    if (!RegisterBlockAndWakeupHandlers(glamor_block_handler,
					glamor_wakeup_handler,
					NULL)) {
	dixSetPrivate(&screen->devPrivates, glamor_screen_private_key, NULL);
	xfree(glamor_priv);
	return FALSE;
    }

    glamor_priv->saved_create_gc = screen->CreateGC;
    screen->CreateGC = glamor_create_gc;

    glamor_priv->saved_create_pixmap = screen->CreatePixmap;
    screen->CreatePixmap = glamor_create_pixmap;

    glamor_priv->saved_destroy_pixmap = screen->DestroyPixmap;
    screen->DestroyPixmap = glamor_destroy_pixmap;

    return TRUE;
}

void
glamor_fini(ScreenPtr screen)
{
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);

    screen->CreateGC = glamor_priv->saved_create_gc;
    screen->CreatePixmap = glamor_priv->saved_create_pixmap;
    screen->DestroyPixmap = glamor_priv->saved_destroy_pixmap;
}
