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

#include "eventstr.h"
#include "exevents.h"

/**
 * Some documentation about touch points:
 * The driver submits touch events with it's own (unique) touch point ID.
 * The driver may re-use those IDs, the DDX doesn't care. It just passes on
 * the data to the DIX. In the server, the driver's ID is referred to as the
 * DDX id anyway.
 *
 * On a TouchBegin, we create a DDXTouchPointInfo that contains the DDX id
 * and the client ID that this touchpoint will have. The client ID is the
 * one visible on the protocol.
 *
 * TouchUpdate and TouchEnd will only be processed if there is an active
 * touchpoint with the same DDX id.
 *
 * The DDXTouchPointInfo struct is stored dev->last.touches. When the event
 * being processed, it becomes a TouchPointInfo in dev->touch-touches which
 * contains amongst other things the sprite trace and delivery information.
 */

/**
 * Given the DDX-facing ID (which is _not_ DeviceEvent::detail.touch), find the
 * associated DDXTouchPointInfoRec.
 *
 * @param dev The device to create the touch point for
 * @param ddx_id Touch id assigned by the driver/ddx
 * @param create Create the touchpoint if it cannot be found
 */
DDXTouchPointInfoPtr
TouchFindByDDXID(DeviceIntPtr dev, uint32_t ddx_id, Bool create)
{
    DDXTouchPointInfoPtr ti;
    int i;

    if (!dev->touch)
        return NULL;

    for (i = 0; i < dev->last.num_touches; i++)
    {
        ti = &dev->last.touches[i];
        if (ti->active && ti->ddx_id == ddx_id)
            return ti;
    }

    return create ? TouchBeginDDXTouch(dev, ddx_id) : NULL;
}

/**
 * Given a unique DDX ID for a touchpoint, create a touchpoint record and
 * return it.
 *
 * If no other touch points are active, mark new touchpoint for pointer
 * emulation.
 *
 * Returns NULL on failure (i.e. if another touch with that ID is already active,
 * allocation failure).
 */
DDXTouchPointInfoPtr
TouchBeginDDXTouch(DeviceIntPtr dev, uint32_t ddx_id)
{
    static int next_client_id = 1;
    int i;
    TouchClassPtr t = dev->touch;
    DDXTouchPointInfoPtr ti = NULL;
    Bool emulate_pointer = (t->mode == XIDirectTouch);

    if (!t)
        return NULL;

    /* Look for another active touchpoint with the same DDX ID. DDX
     * touchpoints must be unique. */
    if (TouchFindByDDXID(dev, ddx_id, FALSE))
        return NULL;

    for (i = 0; i < dev->last.num_touches; i++)
    {
        /* Only emulate pointer events on the first touch */
        if (dev->last.touches[i].active)
            emulate_pointer = FALSE;
        else if (!ti) /* ti is now first non-active touch rec */
            ti = &dev->last.touches[i];

        if (!emulate_pointer && ti)
            break;
    }

    if (ti)
    {
        int client_id;
        ti->active = TRUE;
        ti->ddx_id = ddx_id;
        client_id = next_client_id;
        next_client_id++;
        if (next_client_id == 0)
            next_client_id = 1;
        ti->client_id = client_id;
        ti->emulate_pointer = emulate_pointer;
        return ti;
    }

    /* If we get here, then we've run out of touches, drop the event */
    return NULL;
}

void
TouchEndDDXTouch(DeviceIntPtr dev, DDXTouchPointInfoPtr ti)
{
    TouchClassPtr t = dev->touch;

    if (!t)
        return;

    ti->active = FALSE;
}

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


