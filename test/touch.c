/**
 * Copyright © 2011 Red Hat, Inc.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice (including the next
 *  paragraph) shall be included in all copies or substantial portions of the
 *  Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stdint.h>
#include "inputstr.h"
#include "assert.h"

static void touch_find_ddxid(void)
{
    DeviceIntRec dev;
    DDXTouchPointInfoPtr ti;
    ValuatorClassRec val;
    TouchClassRec touch;
    int size = 5;
    int i;

    memset(&dev, 0, sizeof(dev));
    dev.id = 2;
    dev.valuator = &val;
    val.numAxes = 5;
    dev.touch = &touch;
    dev.last.num_touches = size;
    dev.last.touches = calloc(dev.last.num_touches, sizeof(*dev.last.touches));
    inputInfo.devices = &dev;
    assert(dev.last.touches);


    dev.last.touches[0].active = TRUE;
    dev.last.touches[0].ddx_id = 10;
    dev.last.touches[0].client_id = 20;


    /* existing */
    ti = TouchFindByDDXID(&dev, 10, FALSE);
    assert(ti == &dev.last.touches[0]);

    /* non-existing */
    ti = TouchFindByDDXID(&dev, 20, FALSE);
    assert(ti == NULL);

    /* Non-active */
    dev.last.touches[0].active = FALSE;
    ti = TouchFindByDDXID(&dev, 10, FALSE);
    assert(ti == NULL);

    /* create on number 2*/
    dev.last.touches[0].active = TRUE;

    ti = TouchFindByDDXID(&dev, 20, TRUE);
    assert(ti == &dev.last.touches[1]);
    assert(ti->active);
    assert(ti->ddx_id == 20);

    /* set all to active */
    for (i = 0; i < size; i++)
        dev.last.touches[i].active = TRUE;

    /* Try to create more, fail */
    ti = TouchFindByDDXID(&dev, 30, TRUE);
    assert(ti == NULL);
    ti = TouchFindByDDXID(&dev, 30, TRUE);
    assert(ti == NULL);
    /* make sure we haven't resized, we're in the signal handler */
    assert(dev.last.num_touches == size);

    /* stop one touchpoint, try to create, succeed */
    dev.last.touches[2].active = FALSE;
    ti = TouchFindByDDXID(&dev, 30, TRUE);
    assert(ti == &dev.last.touches[2]);
    /* but still grow anyway */
    ProcessWorkQueue();
    ti = TouchFindByDDXID(&dev, 40, TRUE);
    assert(ti == &dev.last.touches[size]);
}

static void touch_begin_ddxtouch(void)
{
    DeviceIntRec dev;
    DDXTouchPointInfoPtr ti;
    ValuatorClassRec val;
    TouchClassRec touch;
    int ddx_id = 123;
    unsigned int last_client_id = 0;
    int size = 5;

    memset(&dev, 0, sizeof(dev));
    dev.id = 2;
    dev.valuator = &val;
    val.numAxes = 5;
    touch.mode = XIDirectTouch;
    dev.touch = &touch;
    dev.last.num_touches = size;
    dev.last.touches = calloc(dev.last.num_touches, sizeof(*dev.last.touches));
    inputInfo.devices = &dev;
    assert(dev.last.touches);

    ti = TouchBeginDDXTouch(&dev, ddx_id);
    assert(ti);
    assert(ti->ddx_id == ddx_id);
    /* client_id == ddx_id can happen in real life, but not in this test */
    assert(ti->client_id != ddx_id);
    assert(ti->active);
    assert(ti->client_id > last_client_id);
    assert(ti->emulate_pointer);
    last_client_id = ti->client_id;

    ddx_id += 10;
    ti = TouchBeginDDXTouch(&dev, ddx_id);
    assert(ti);
    assert(ti->ddx_id == ddx_id);
    /* client_id == ddx_id can happen in real life, but not in this test */
    assert(ti->client_id != ddx_id);
    assert(ti->active);
    assert(ti->client_id > last_client_id);
    assert(!ti->emulate_pointer);
    last_client_id = ti->client_id;
}

int main(int argc, char** argv)
{
    touch_find_ddxid();
    touch_begin_ddxtouch();

    return 0;
}
