/************************************************************
Copyright (c) 1993 by Silicon Graphics Computer Systems, Inc.

Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and without
fee is hereby granted, provided that the above copyright
notice appear in all copies and that both that copyright
notice and this permission notice appear in supporting
documentation, and that the name of Silicon Graphics not be 
used in advertising or publicity pertaining to distribution 
of the software without specific prior written permission.
Silicon Graphics makes no representation about the suitability 
of this software for any purpose. It is provided "as is"
without any express or implied warranty.

SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS 
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY 
AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL 
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, 
DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE 
OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "inputstr.h"
#include <xkbsrv.h>
#include "mi.h"

void
XkbDDXFakePointerMotion(DeviceIntPtr dev, unsigned flags,int x,int y)
{
    EventListPtr        events;
    int                 nevents, i;
    DeviceIntPtr        ptr;
    int                 gpe_flags = 0;

    if (!dev->u.master)
        ptr = dev;
    else
        ptr = GetXTestDevice(GetMaster(dev, MASTER_POINTER));

    if (flags & XkbSA_MoveAbsoluteX || flags & XkbSA_MoveAbsoluteY)
        gpe_flags = POINTER_ABSOLUTE;
    else
        gpe_flags = POINTER_RELATIVE;

    events = InitEventList(GetMaximumEventsNum());
    OsBlockSignals();
    nevents = GetPointerEvents(events, ptr,
                               MotionNotify, 0,
                               gpe_flags, 0, 2, (int[]){x, y});
    OsReleaseSignals();

    for (i = 0; i < nevents; i++)
        mieqProcessDeviceEvent(ptr, (InternalEvent*)events[i].event, NULL);

    FreeEventList(events, GetMaximumEventsNum());
}
