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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

#ifdef HAVE_CONFIG_H
#include <kdrive-config.h>
#endif

#include "ephyr.h"
#include "glamor.h"

/**
 * This function initializes EXA to use the fake acceleration implementation
 * which just falls through to software.  The purpose is to have a reliable,
 * correct driver with which to test changes to the EXA core.
 */
Bool
ephyr_glamor_init(ScreenPtr screen)
{
    KdScreenPriv(screen);
    KdScreenInfo *kd_screen = pScreenPriv->screen;

    ephyr_glamor_host_create_context(kd_screen);

    glamor_init(screen, GLAMOR_HOSTX);

    return TRUE;
}

void
ephyr_glamor_enable(ScreenPtr screen)
{
}

void
ephyr_glamor_disable(ScreenPtr screen)
{
}

void
ephyr_glamor_fini(ScreenPtr screen)
{
    glamor_fini(screen);
}
