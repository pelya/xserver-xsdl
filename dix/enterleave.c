/*
 * Copyright © 2008 Red Hat, Inc.
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
 * Authors: Peter Hutterer
 *
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include "windowstr.h"
#include "enterleave.h"

/**
 * @return The window that is the first ancestor of both a and b.
 */
WindowPtr
CommonAncestor(
    WindowPtr a,
    WindowPtr b)
{
    for (b = b->parent; b; b = b->parent)
	if (IsParent(b, a)) return b;
    return NullWindow;
}


/**
 * Send enter notifies to all parent windows up to ancestor.
 * This function recurses.
 */
static void
EnterNotifies(DeviceIntPtr pDev,
              WindowPtr ancestor,
              WindowPtr child,
              int mode,
              int detail)
{
    WindowPtr	parent = child->parent;

    if (ancestor == parent)
	return;
    EnterNotifies(pDev, ancestor, parent, mode, detail);
    EnterLeaveEvent(pDev, EnterNotify, mode, detail, parent,
                    child->drawable.id);
}


/**
 * Send leave notifies to all parent windows up to ancestor.
 * This function recurses.
 */
static void
LeaveNotifies(DeviceIntPtr pDev,
              WindowPtr child,
              WindowPtr ancestor,
              int mode,
              int detail)
{
    WindowPtr  pWin;

    if (ancestor == child)
	return;
    for (pWin = child->parent; pWin != ancestor; pWin = pWin->parent)
    {
        EnterLeaveEvent(pDev, LeaveNotify, mode, detail, pWin,
                        child->drawable.id);
        child = pWin;
    }
}

/**
 * Figure out if enter/leave events are necessary and send them to the
 * appropriate windows.
 *
 * @param fromWin Window the sprite moved out of.
 * @param toWin Window the sprite moved into.
 */
void
DoEnterLeaveEvents(DeviceIntPtr pDev,
        WindowPtr fromWin,
        WindowPtr toWin,
        int mode)
{
    if (!IsPointerDevice(pDev))
        return;

    if (fromWin == toWin)
	return;
    if (IsParent(fromWin, toWin))
    {
        EnterLeaveEvent(pDev, LeaveNotify, mode, NotifyInferior, fromWin,
                        None);
        EnterNotifies(pDev, fromWin, toWin, mode,
                      NotifyVirtual);
        EnterLeaveEvent(pDev, EnterNotify, mode, NotifyAncestor, toWin, None);
    }
    else if (IsParent(toWin, fromWin))
    {
	EnterLeaveEvent(pDev, LeaveNotify, mode, NotifyAncestor, fromWin,
                        None);
	LeaveNotifies(pDev, fromWin, toWin, mode, NotifyVirtual);
	EnterLeaveEvent(pDev, EnterNotify, mode, NotifyInferior, toWin, None);
    }
    else
    { /* neither fromWin nor toWin is descendent of the other */
	WindowPtr common = CommonAncestor(toWin, fromWin);
	/* common == NullWindow ==> different screens */
        EnterLeaveEvent(pDev, LeaveNotify, mode, NotifyNonlinear, fromWin,
                        None);
        LeaveNotifies(pDev, fromWin, common, mode, NotifyNonlinearVirtual);
	EnterNotifies(pDev, common, toWin, mode, NotifyNonlinearVirtual);
        EnterLeaveEvent(pDev, EnterNotify, mode, NotifyNonlinear, toWin,
                        None);
    }
}

