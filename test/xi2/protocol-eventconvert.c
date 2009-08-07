/**
 * Copyright © 2009 Red Hat, Inc.
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
#include <glib.h>

#include "inputstr.h"
#include "eventstr.h"
#include "eventconvert.h"
#include <X11/extensions/XI2proto.h>


static void test_values_XIRawEvent(RawDeviceEvent *in, xXIRawEvent *out,
                                   BOOL swap)
{
    int i;
    unsigned char *ptr;
    FP3232 *value, *raw_value;
    int nvals = 0;
    int bits_set;
    int len;

    if (swap)
    {
        char n;

        swaps(&out->sequenceNumber, n);
        swapl(&out->length, n);
        swaps(&out->evtype, n);
        swaps(&out->deviceid, n);
        swapl(&out->time, n);
        swapl(&out->detail, n);
        swaps(&out->valuators_len, n);
    }


    g_assert(out->type == GenericEvent);
    g_assert(out->extension == 0); /* IReqCode defaults to 0 */
    g_assert(out->evtype == GetXI2Type((InternalEvent*)in));
    g_assert(out->time == in->time);
    g_assert(out->detail == in->detail.button);
    g_assert(out->deviceid == in->deviceid);
    g_assert(out->valuators_len >= bytes_to_int32(bits_to_bytes(sizeof(in->valuators.mask))));
    g_assert(out->flags == 0); /* FIXME: we don't set the flags yet */

    ptr = (unsigned char*)&out[1];
    bits_set = 0;

    for (i = 0; out->valuators_len && i < sizeof(in->valuators.mask) * 8; i++)
    {
        g_assert (XIMaskIsSet(in->valuators.mask, i) == XIMaskIsSet(ptr, i));
        if (XIMaskIsSet(in->valuators.mask, i))
            bits_set++;
    }

    /* length is len of valuator mask (in 4-byte units) + the number of bits
     * set. Each bit set represents 2 8-byte values, hence the
     * 'bits_set * 4' */
    len = out->valuators_len + bits_set * 4;
    g_assert(out->length == len);

    nvals = 0;

    for (i = 0; out->valuators_len && i < MAX_VALUATORS; i++)
    {
        g_assert (XIMaskIsSet(in->valuators.mask, i) == XIMaskIsSet(ptr, i));
        if (XIMaskIsSet(in->valuators.mask, i))
        {
            FP3232 vi, vo;
            value = (FP3232*)(((unsigned char*)&out[1]) + out->valuators_len * 4);
            value += nvals;

            vi.integral = in->valuators.data[i];
            vi.frac = in->valuators.data_frac[i];

            vo.integral = value->integral;
            vo.frac = value->frac;
            if (swap)
            {
                char n;
                swapl(&vo.integral, n);
                swapl(&vo.frac, n);
            }

            g_assert(vi.integral == vo.integral);
            g_assert(vi.frac == vo.frac);

            raw_value = value + bits_set;

            vi.integral = in->valuators.data_raw[i];
            vi.frac = in->valuators.data_raw_frac[i];

            vo.integral = raw_value->integral;
            vo.frac = raw_value->frac;
            if (swap)
            {
                char n;
                swapl(&vo.integral, n);
                swapl(&vo.frac, n);
            }

            g_assert(vi.integral == vo.integral);
            g_assert(vi.frac == vo.frac);

            nvals++;
        }
    }
}

static void test_XIRawEvent(RawDeviceEvent *in)
{
    xXIRawEvent *out, *swapped;
    int rc;

    rc = EventToXI2((InternalEvent*)in, (xEvent**)&out);
    g_assert(rc == Success);

    test_values_XIRawEvent(in, out, FALSE);

    swapped = xcalloc(1, sizeof(xEvent) + out->length * 4);
    XI2EventSwap(out, swapped);
    test_values_XIRawEvent(in, swapped, TRUE);

    xfree(out);
    xfree(swapped);

}


static void test_convert_XIRawEvent(void)
{
    RawDeviceEvent in;
    int i;

    memset(&in, 0, sizeof(in));

    g_test_message("Testing all event types");
    in.header = ET_Internal;
    in.type = ET_RawMotion;
    test_XIRawEvent(&in);

    in.header = ET_Internal;
    in.type = ET_RawKeyPress;
    test_XIRawEvent(&in);

    in.header = ET_Internal;
    in.type = ET_RawKeyRelease;
    test_XIRawEvent(&in);

    in.header = ET_Internal;
    in.type = ET_RawButtonPress;
    test_XIRawEvent(&in);

    in.header = ET_Internal;
    in.type = ET_RawButtonRelease;
    test_XIRawEvent(&in);

    g_test_message("Testing details and other fields");
    in.detail.button = 1L;
    test_XIRawEvent(&in);
    in.detail.button = 1L << 8;
    test_XIRawEvent(&in);
    in.detail.button = 1L << 16;
    test_XIRawEvent(&in);
    in.detail.button = 1L << 24;
    test_XIRawEvent(&in);
    in.detail.button = ~0L;
    test_XIRawEvent(&in);

    in.detail.button = 0;

    in.time = 1L;
    test_XIRawEvent(&in);
    in.time = 1L << 8;
    test_XIRawEvent(&in);
    in.time = 1L << 16;
    test_XIRawEvent(&in);
    in.time = 1L << 24;
    test_XIRawEvent(&in);
    in.time = ~0L;
    test_XIRawEvent(&in);

    in.deviceid = 1;
    test_XIRawEvent(&in);
    in.deviceid = 1 << 8;
    test_XIRawEvent(&in);
    in.deviceid = ~0 & 0xFF;
    test_XIRawEvent(&in);

    g_test_message("Testing valuator masks");
    for (i = 0; i < sizeof(in.valuators.mask) * 8; i++)
    {
        XISetMask(in.valuators.mask, i);
        test_XIRawEvent(&in);
        XIClearMask(in.valuators.mask, i);
    }

    for (i = 0; i < sizeof(in.valuators.mask) * 8; i++)
    {
        XISetMask(in.valuators.mask, i);

        in.valuators.data[i] = i;
        in.valuators.data_raw[i] = i + 10;
        in.valuators.data_frac[i] = i + 20;
        in.valuators.data_raw_frac[i] = i + 30;
        test_XIRawEvent(&in);
        XIClearMask(in.valuators.mask, i);
    }

    for (i = 0; i < sizeof(in.valuators.mask) * 8; i++)
    {
        XISetMask(in.valuators.mask, i);
        test_XIRawEvent(&in);
    }
}

int main(int argc, char** argv)
{
    g_test_init(&argc, &argv,NULL);
    g_test_bug_base("https://bugzilla.freedesktop.org/show_bug.cgi?id=");

    g_test_add_func("/xi2/eventconvert/XIRawEvent", test_convert_XIRawEvent);

    return g_test_run();
}
