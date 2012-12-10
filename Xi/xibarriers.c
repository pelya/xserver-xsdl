/*
 * Copyright 2012 Red Hat, Inc.
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
 * Copyright © 2002 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "xibarriers.h"
#include "scrnintstr.h"
#include "cursorstr.h"
#include "dixevents.h"
#include "servermd.h"
#include "mipointer.h"
#include "inputstr.h"
#include "windowstr.h"
#include "xace.h"
#include "list.h"
#include "exglobals.h"
#include "eventstr.h"
#include "mi.h"

RESTYPE PointerBarrierType;

static DevPrivateKeyRec BarrierScreenPrivateKeyRec;

#define BarrierScreenPrivateKey (&BarrierScreenPrivateKeyRec)

typedef struct PointerBarrierClient *PointerBarrierClientPtr;

struct PointerBarrierClient {
    XID id;
    ScreenPtr screen;
    WindowPtr window;
    struct PointerBarrier barrier;
    struct xorg_list entry;
    int num_devices;
    int *device_ids; /* num_devices */
    Time last_timestamp;
    int barrier_event_id;
    int release_event_id;
    Bool hit;
    Bool seen;
};

typedef struct _BarrierScreen {
    struct xorg_list barriers;
} BarrierScreenRec, *BarrierScreenPtr;

#define GetBarrierScreen(s) ((BarrierScreenPtr)dixLookupPrivate(&(s)->devPrivates, BarrierScreenPrivateKey))
#define GetBarrierScreenIfSet(s) GetBarrierScreen(s)
#define SetBarrierScreen(s,p) dixSetPrivate(&(s)->devPrivates, BarrierScreenPrivateKey, p)

static BOOL
barrier_is_horizontal(const struct PointerBarrier *barrier)
{
    return barrier->y1 == barrier->y2;
}

static BOOL
barrier_is_vertical(const struct PointerBarrier *barrier)
{
    return barrier->x1 == barrier->x2;
}

/**
 * @return The set of barrier movement directions the movement vector
 * x1/y1 → x2/y2 represents.
 */
int
barrier_get_direction(int x1, int y1, int x2, int y2)
{
    int direction = 0;

    /* which way are we trying to go */
    if (x2 > x1)
        direction |= BarrierPositiveX;
    if (x2 < x1)
        direction |= BarrierNegativeX;
    if (y2 > y1)
        direction |= BarrierPositiveY;
    if (y2 < y1)
        direction |= BarrierNegativeY;

    return direction;
}

/**
 * Test if the barrier may block movement in the direction defined by
 * x1/y1 → x2/y2. This function only tests whether the directions could be
 * blocked, it does not test if the barrier actually blocks the movement.
 *
 * @return TRUE if the barrier blocks the direction of movement or FALSE
 * otherwise.
 */
BOOL
barrier_is_blocking_direction(const struct PointerBarrier * barrier,
                              int direction)
{
    /* Barriers define which way is ok, not which way is blocking */
    return (barrier->directions & direction) != direction;
}

/**
 * Test if the movement vector x1/y1 → x2/y2 is intersecting with the
 * barrier. A movement vector with the startpoint or endpoint adjacent to
 * the barrier itself counts as intersecting.
 *
 * @param x1 X start coordinate of movement vector
 * @param y1 Y start coordinate of movement vector
 * @param x2 X end coordinate of movement vector
 * @param y2 Y end coordinate of movement vector
 * @param[out] distance The distance between the start point and the
 * intersection with the barrier (if applicable).
 * @return TRUE if the barrier intersects with the given vector
 */
BOOL
barrier_is_blocking(const struct PointerBarrier * barrier,
                    int x1, int y1, int x2, int y2, double *distance)
{
    BOOL rc = FALSE;
    float ua, ub, ud;
    int dir = barrier_get_direction(x1, y1, x2, y2);

    /* Algorithm below doesn't handle edge cases well, hence the extra
     * checks. */
    if (barrier_is_vertical(barrier)) {
        /* handle immediate barrier adjacency, moving away */
        if (dir & BarrierPositiveX && x1 == barrier->x1)
            return FALSE;
        if (dir & BarrierNegativeX && x1 == (barrier->x1 - 1))
            return FALSE;
        /* startpoint adjacent to barrier, moving towards -> block */
        if (dir & BarrierPositiveX && x1 == (barrier->x1 - 1) && y1 >= barrier->y1 && y1 <= barrier->y2) {
            *distance = 0;
            return TRUE;
        }
        if (dir & BarrierNegativeX && x1 == barrier->x1 && y1 >= barrier->y1 && y1 <= barrier->y2) {
            *distance = 0;
            return TRUE;
        }
    }
    else {
        /* handle immediate barrier adjacency, moving away */
        if (dir & BarrierPositiveY && y1 == barrier->y1)
            return FALSE;
        if (dir & BarrierNegativeY && y1 == (barrier->y1 - 1))
            return FALSE;
        /* startpoint adjacent to barrier, moving towards -> block */
        if (dir & BarrierPositiveY && y1 == (barrier->y1 - 1) && x1 >= barrier->x1 && x1 <= barrier->x2) {
            *distance = 0;
            return TRUE;
        }
        if (dir & BarrierNegativeY && y1 == barrier->y1 && x1 >= barrier->x1 && x1 <= barrier->x2) {
            *distance = 0;
            return TRUE;
        }
    }

    /* not an edge case, compute distance */
    ua = 0;
    ud = (barrier->y2 - barrier->y1) * (x2 - x1) - (barrier->x2 -
                                                    barrier->x1) * (y2 - y1);
    if (ud != 0) {
        ua = ((barrier->x2 - barrier->x1) * (y1 - barrier->y1) -
              (barrier->y2 - barrier->y1) * (x1 - barrier->x1)) / ud;
        ub = ((x2 - x1) * (y1 - barrier->y1) -
              (y2 - y1) * (x1 - barrier->x1)) / ud;
        if (ua < 0 || ua > 1 || ub < 0 || ub > 1)
            ua = 0;
    }

    if (ua > 0 && ua <= 1) {
        double ix = barrier->x1 + ua * (barrier->x2 - barrier->x1);
        double iy = barrier->y1 + ua * (barrier->y2 - barrier->y1);

        *distance = sqrt(pow(x1 - ix, 2) + pow(y1 - iy, 2));
        rc = TRUE;
    }

    return rc;
}

#define HIT_EDGE_EXTENTS 2
static BOOL
barrier_inside_hit_box(struct PointerBarrier *barrier, int x, int y)
{
    int x1, x2, y1, y2;
    int dir;

    x1 = barrier->x1;
    x2 = barrier->x2;
    y1 = barrier->y1;
    y2 = barrier->y2;
    dir = ~(barrier->directions);

    if (barrier_is_vertical(barrier)) {
        if (dir & BarrierPositiveX)
            x1 -= HIT_EDGE_EXTENTS;
        if (dir & BarrierNegativeX)
            x2 += HIT_EDGE_EXTENTS;
    }
    if (barrier_is_horizontal(barrier)) {
        if (dir & BarrierPositiveY)
            y1 -= HIT_EDGE_EXTENTS;
        if (dir & BarrierNegativeY)
            y2 += HIT_EDGE_EXTENTS;
    }

    return x >= x1 && x <= x2 && y >= y1 && y <= y2;
}

static BOOL
barrier_blocks_device(struct PointerBarrierClient *client,
                      DeviceIntPtr dev)
{
    int i;
    int master_id;

    /* Clients with no devices are treated as
     * if they specified XIAllDevices. */
    if (client->num_devices == 0)
        return TRUE;

    master_id = GetMaster(dev, POINTER_OR_FLOAT)->id;

    for (i = 0; i < client->num_devices; i++) {
        int device_id = client->device_ids[i];
        if (device_id == XIAllDevices ||
            device_id == XIAllMasterDevices ||
            device_id == master_id)
            return TRUE;
    }

    return FALSE;
}

/**
 * Find the nearest barrier client that is blocking movement from x1/y1 to x2/y2.
 *
 * @param dir Only barriers blocking movement in direction dir are checked
 * @param x1 X start coordinate of movement vector
 * @param y1 Y start coordinate of movement vector
 * @param x2 X end coordinate of movement vector
 * @param y2 Y end coordinate of movement vector
 * @return The barrier nearest to the movement origin that blocks this movement.
 */
static struct PointerBarrierClient *
barrier_find_nearest(BarrierScreenPtr cs, DeviceIntPtr dev,
                     int dir,
                     int x1, int y1, int x2, int y2)
{
    struct PointerBarrierClient *c, *nearest = NULL;
    double min_distance = INT_MAX;      /* can't get higher than that in X anyway */

    xorg_list_for_each_entry(c, &cs->barriers, entry) {
        struct PointerBarrier *b = &c->barrier;
        double distance;

        if (c->seen)
            continue;

        if (!barrier_is_blocking_direction(b, dir))
            continue;

        if (!barrier_blocks_device(c, dev))
            continue;

        if (barrier_is_blocking(b, x1, y1, x2, y2, &distance)) {
            if (min_distance > distance) {
                min_distance = distance;
                nearest = c;
            }
        }
    }

    return nearest;
}

/**
 * Clamp to the given barrier given the movement direction specified in dir.
 *
 * @param barrier The barrier to clamp to
 * @param dir The movement direction
 * @param[out] x The clamped x coordinate.
 * @param[out] y The clamped x coordinate.
 */
void
barrier_clamp_to_barrier(struct PointerBarrier *barrier, int dir, int *x,
                         int *y)
{
    if (barrier_is_vertical(barrier)) {
        if ((dir & BarrierNegativeX) & ~barrier->directions)
            *x = barrier->x1;
        if ((dir & BarrierPositiveX) & ~barrier->directions)
            *x = barrier->x1 - 1;
    }
    if (barrier_is_horizontal(barrier)) {
        if ((dir & BarrierNegativeY) & ~barrier->directions)
            *y = barrier->y1;
        if ((dir & BarrierPositiveY) & ~barrier->directions)
            *y = barrier->y1 - 1;
    }
}

void
input_constrain_cursor(DeviceIntPtr dev, ScreenPtr screen,
                       int current_x, int current_y,
                       int dest_x, int dest_y,
                       int *out_x, int *out_y)
{
    /* Clamped coordinates here refer to screen edge clamping. */
    BarrierScreenPtr cs = GetBarrierScreen(screen);
    int x = dest_x,
        y = dest_y;
    int dir;
    struct PointerBarrier *nearest = NULL;
    PointerBarrierClientPtr c;
    Time ms = GetTimeInMillis();
    BarrierEvent ev = {
        .header = ET_Internal,
        .type = 0,
        .length = sizeof (BarrierEvent),
        .time = ms,
        .deviceid = dev->id,
        .sourceid = dev->id,
        .dx = dest_x - current_x,
        .dy = dest_y - current_y,
        .root = screen->root->drawable.id,
    };

    if (xorg_list_is_empty(&cs->barriers) || IsFloating(dev))
        goto out;

    /* How this works:
     * Given the origin and the movement vector, get the nearest barrier
     * to the origin that is blocking the movement.
     * Clamp to that barrier.
     * Then, check from the clamped intersection to the original
     * destination, again finding the nearest barrier and clamping.
     */
    dir = barrier_get_direction(current_x, current_y, x, y);

    while (dir != 0) {
        c = barrier_find_nearest(cs, dev, dir, current_x, current_y, x, y);
        if (!c)
            break;

        nearest = &c->barrier;

        c->seen = TRUE;
        c->hit = TRUE;

        if (c->barrier_event_id == c->release_event_id)
            continue;

        ev.type = ET_BarrierHit;
        barrier_clamp_to_barrier(nearest, dir, &x, &y);

        if (barrier_is_vertical(nearest)) {
            dir &= ~(BarrierNegativeX | BarrierPositiveX);
            current_x = x;
        }
        else if (barrier_is_horizontal(nearest)) {
            dir &= ~(BarrierNegativeY | BarrierPositiveY);
            current_y = y;
        }

        ev.flags = 0;
        ev.event_id = c->barrier_event_id;
        ev.barrierid = c->id;

        ev.dt = ms - c->last_timestamp;
        ev.window = c->window->drawable.id;
        c->last_timestamp = ms;

        mieqEnqueue(dev, (InternalEvent *) &ev);
    }

    xorg_list_for_each_entry(c, &cs->barriers, entry) {
        int flags = 0;
        c->seen = FALSE;
        if (!c->hit)
            continue;

        if (barrier_inside_hit_box(&c->barrier, x, y))
            continue;

        c->hit = FALSE;

        ev.type = ET_BarrierLeave;

        if (c->barrier_event_id == c->release_event_id)
            flags |= XIBarrierPointerReleased;

        ev.flags = flags;
        ev.event_id = c->barrier_event_id;
        ev.barrierid = c->id;

        ev.dt = ms - c->last_timestamp;
        ev.window = c->window->drawable.id;
        c->last_timestamp = ms;

        mieqEnqueue(dev, (InternalEvent *) &ev);

        /* If we've left the hit box, this is the
         * start of a new event ID. */
        c->barrier_event_id++;
    }

 out:
    *out_x = x;
    *out_y = y;
}

static int
CreatePointerBarrierClient(ClientPtr client,
                           xXFixesCreatePointerBarrierReq * stuff,
                           PointerBarrierClientPtr *client_out)
{
    WindowPtr pWin;
    ScreenPtr screen;
    BarrierScreenPtr cs;
    int err;
    int size;
    int i;
    struct PointerBarrierClient *ret;
    CARD16 *in_devices;

    size = sizeof(*ret) + sizeof(DeviceIntPtr) * stuff->num_devices;
    ret = malloc(size);

    if (!ret) {
        return BadAlloc;
    }

    err = dixLookupWindow(&pWin, stuff->window, client, DixReadAccess);
    if (err != Success) {
        client->errorValue = stuff->window;
        goto error;
    }

    screen = pWin->drawable.pScreen;
    cs = GetBarrierScreen(screen);

    ret->screen = screen;
    ret->window = pWin;
    ret->num_devices = stuff->num_devices;
    if (ret->num_devices > 0)
        ret->device_ids = (int*)&ret[1];
    else
        ret->device_ids = NULL;

    in_devices = (CARD16 *) &stuff[1];
    for (i = 0; i < stuff->num_devices; i++) {
        int device_id = in_devices[i];
        DeviceIntPtr device;

        if ((err = dixLookupDevice (&device, device_id,
                                    client, DixReadAccess))) {
            client->errorValue = device_id;
            goto error;
        }

        if (!IsMaster (device)) {
            client->errorValue = device_id;
            err = BadDevice;
            goto error;
        }

        ret->device_ids[i] = device_id;
    }

    ret->id = stuff->barrier;
    ret->barrier_event_id = 1;
    ret->release_event_id = 0;
    ret->hit = FALSE;
    ret->seen = FALSE;
    ret->barrier.x1 = min(stuff->x1, stuff->x2);
    ret->barrier.x2 = max(stuff->x1, stuff->x2);
    ret->barrier.y1 = min(stuff->y1, stuff->y2);
    ret->barrier.y2 = max(stuff->y1, stuff->y2);
    ret->barrier.directions = stuff->directions & 0x0f;
    if (barrier_is_horizontal(&ret->barrier))
        ret->barrier.directions &= ~(BarrierPositiveX | BarrierNegativeX);
    if (barrier_is_vertical(&ret->barrier))
        ret->barrier.directions &= ~(BarrierPositiveY | BarrierNegativeY);
    xorg_list_add(&ret->entry, &cs->barriers);

    *client_out = ret;
    return Success;

 error:
    *client_out = NULL;
    free(ret);
    return err;
}

static int
BarrierFreeBarrier(void *data, XID id)
{
    struct PointerBarrierClient *c;

    c = container_of(data, struct PointerBarrierClient, barrier);
    xorg_list_del(&c->entry);

    free(c);
    return Success;
}

int
XICreatePointerBarrier(ClientPtr client,
                       xXFixesCreatePointerBarrierReq * stuff)
{
    int err;
    struct PointerBarrierClient *barrier;
    struct PointerBarrier b;

    b.x1 = stuff->x1;
    b.x2 = stuff->x2;
    b.y1 = stuff->y1;
    b.y2 = stuff->y2;

    if (!barrier_is_horizontal(&b) && !barrier_is_vertical(&b))
        return BadValue;

    /* no 0-sized barriers */
    if (barrier_is_horizontal(&b) && barrier_is_vertical(&b))
        return BadValue;

    if ((err = CreatePointerBarrierClient(client, stuff, &barrier)))
        return err;

    if (!AddResource(stuff->barrier, PointerBarrierType, &barrier->barrier))
        return BadAlloc;

    return Success;
}

int
XIDestroyPointerBarrier(ClientPtr client,
                        xXFixesDestroyPointerBarrierReq * stuff)
{
    int err;
    void *barrier;

    err = dixLookupResourceByType((void **) &barrier, stuff->barrier,
                                  PointerBarrierType, client, DixDestroyAccess);
    if (err != Success) {
        client->errorValue = stuff->barrier;
        return err;
    }

    if (CLIENT_ID(stuff->barrier) != client->index)
        return BadAccess;

    FreeResource(stuff->barrier, RT_NONE);
    return Success;
}

int
SProcXIBarrierReleasePointer(ClientPtr client)
{
    xXIBarrierReleasePointerInfo *info;
    REQUEST(xXIBarrierReleasePointerReq);
    int i;

    info = (xXIBarrierReleasePointerInfo*) &stuff[1];

    swaps(&stuff->length);
    swapl(&stuff->num_barriers);
    for (i = 0; i < stuff->num_barriers; i++, info++) {
        swaps(&info->deviceid);
        swapl(&info->barrier);
        swapl(&info->eventid);
    }

    return (ProcXIBarrierReleasePointer(client));
}

int
ProcXIBarrierReleasePointer(ClientPtr client)
{
    int i;
    int err;
    struct PointerBarrierClient *barrier;
    struct PointerBarrier *b;
    xXIBarrierReleasePointerInfo *info;

    REQUEST(xXIBarrierReleasePointerReq);
    REQUEST_AT_LEAST_SIZE(xXIBarrierReleasePointerReq);

    info = (xXIBarrierReleasePointerInfo*) &stuff[1];
    for (i = 0; i < stuff->num_barriers; i++, info++) {
        CARD32 barrier_id, event_id;
        _X_UNUSED CARD32 device_id;

        barrier_id = info->barrier;
        event_id = info->eventid;

        /* FIXME: per-device releases */
        device_id = info->deviceid;

        err = dixLookupResourceByType((void **) &b, barrier_id,
                                      PointerBarrierType, client, DixReadAccess);
        if (err != Success) {
            client->errorValue = barrier_id;
            return err;
        }

        if (CLIENT_ID(barrier_id) != client->index)
            return BadAccess;

        barrier = container_of(b, struct PointerBarrierClient, barrier);
        if (barrier->barrier_event_id == event_id)
            barrier->release_event_id = event_id;
    }

    return Success;
}

Bool
XIBarrierInit(void)
{
    int i;

    if (!dixRegisterPrivateKey(&BarrierScreenPrivateKeyRec, PRIVATE_SCREEN, 0))
        return FALSE;

    for (i = 0; i < screenInfo.numScreens; i++) {
        ScreenPtr pScreen = screenInfo.screens[i];
        BarrierScreenPtr cs;

        cs = (BarrierScreenPtr) calloc(1, sizeof(BarrierScreenRec));
        if (!cs)
            return FALSE;
        xorg_list_init(&cs->barriers);
        SetBarrierScreen(pScreen, cs);
    }

    PointerBarrierType = CreateNewResourceType(BarrierFreeBarrier,
                                               "XIPointerBarrier");

    return PointerBarrierType;
}
