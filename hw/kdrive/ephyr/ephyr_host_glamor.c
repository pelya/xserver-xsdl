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

#ifdef HAVE_CONFIG_H
#include <kdrive-config.h>
#endif

#include <err.h>
#include "GL/glx.h"

#include "hostx.h"

void
ephyr_glamor_host_create_context(EphyrScreenInfo ephyr_screen)
{
    Display *dpy = hostx_get_display();
    int attribs[] = {GLX_RGBA,
		     GLX_RED_SIZE, 1,
		     GLX_GREEN_SIZE, 1,
		     GLX_BLUE_SIZE, 1,
		     None};
    XVisualInfo *visual_info;
    GLXContext ctx;

    visual_info = glXChooseVisual(dpy, DefaultScreen(dpy), attribs);
    if (visual_info == NULL)
	errx(1, "Couldn't get RGB visual\n");

    ctx = glXCreateContext(dpy, visual_info, NULL, True);
    if (ctx == NULL)
	errx(1, "glXCreateContext failed\n");

    glXMakeCurrent(dpy, hostx_get_window(DefaultScreen(dpy)), ctx);

    XFree(visual_info);
}
