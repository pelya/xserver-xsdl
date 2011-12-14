/*
 * Copyright © 2011 Collabra Ltd.
 * Copyright © 2011 Red Hat, Inc.
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author: Daniel Stone <daniel@fooishbar.org>
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "inputstr.h"
#include "scrnintstr.h"


void
TouchInitDDXTouchPoint(DeviceIntPtr dev, DDXTouchPointInfoPtr ddxtouch)
{
    memset(ddxtouch, 0, sizeof(*ddxtouch));
    ddxtouch->valuators = valuator_mask_new(dev->valuator->numAxes);
}


Bool
TouchInitTouchPoint(TouchClassPtr t, ValuatorClassPtr v, int index)
{
    TouchPointInfoPtr ti;

    if (index >= t->num_touches)
        return FALSE;
    ti = &t->touches[index];

    memset(ti, 0, sizeof(*ti));

    ti->valuators = valuator_mask_new(v->numAxes);
    if (!ti->valuators)
        return FALSE;

    ti->sprite.spriteTrace = calloc(32, sizeof(*ti->sprite.spriteTrace));
    if (!ti->sprite.spriteTrace)
    {
        valuator_mask_free(&ti->valuators);
        return FALSE;
    }
    ti->sprite.spriteTraceSize = 32;
    ti->sprite.spriteTrace[0] = screenInfo.screens[0]->root;
    ti->sprite.hot.pScreen = screenInfo.screens[0];
    ti->sprite.hotPhys.pScreen = screenInfo.screens[0];

    ti->client_id = -1;

    return TRUE;
}

void
TouchFreeTouchPoint(DeviceIntPtr device, int index)
{
    TouchPointInfoPtr ti;

    if (!device->touch || index >= device->touch->num_touches)
        return;
    ti = &device->touch->touches[index];

    valuator_mask_free(&ti->valuators);
    free(ti->sprite.spriteTrace);
    ti->sprite.spriteTrace = NULL;
    free(ti->listeners);
    ti->listeners = NULL;
    free(ti->history);
    ti->history = NULL;
    ti->history_size = 0;
    ti->history_elements = 0;
}


