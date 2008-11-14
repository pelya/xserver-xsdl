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
#include "exglobals.h"
#include "enterleave.h"

/**
 * Return TRUE if @win has a pointer within its boundaries, excluding child
 * window.
 */
static BOOL
HasPointer(WindowPtr win)
{
    int i;

    for (i = 0; i < sizeof(win->enterleave); i++)
        if (win->enterleave[i])
            return TRUE;

    return FALSE;
}

static BOOL
HasOtherPointer(WindowPtr win, DeviceIntPtr dev)
{
    int i;

    for (i = 0; i < sizeof(win->enterleave); i++)
        if (win->enterleave[i] &&
            !(i == dev->id/8 && win->enterleave[i] == (1 << (dev->id % 8))))
        {
            return TRUE;
        }

    return FALSE;
}

/**
 * Set the presence flag for @dev to mark that it is now in @win.
 */
static void
EnterWindow(DeviceIntPtr dev, WindowPtr win, int mode)
{
    win->enterleave[dev->id/8] |= (1 << (dev->id % 8));
}

/**
 * Unset the presence flag for @dev to mark that it is not in @win anymore.
 */
static void
LeaveWindow(DeviceIntPtr dev, WindowPtr win, int mode)
{
    win->enterleave[dev->id/8] &= ~(1 << (dev->id % 8));
}


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
 * Send enter notifies to all windows between @ancestor and @child (excluding
 * both). Events are sent running up the window hierarchy. This function
 * recurses.
 * If @core is TRUE, core events are sent, otherwise XI events will be sent.
 */
static void
EnterNotifies(DeviceIntPtr dev,
              WindowPtr ancestor,
              WindowPtr child,
              int mode,
              int detail,
              BOOL core)
{
    WindowPtr	parent = child->parent;

    if (ancestor == parent)
	return;
    EnterNotifies(dev, ancestor, parent, mode, detail, core);
    if (core)
        CoreEnterLeaveEvent(dev, EnterNotify, mode, detail, parent,
                            child->drawable.id);
    else
        DeviceEnterLeaveEvent(dev, DeviceEnterNotify, mode, detail, parent,
                              child->drawable.id);
}

/**
 * Send leave notifies to all windows between @child and @ancestor.
 * Events are sent running up the hierarchy.
 */
static void
LeaveNotifies(DeviceIntPtr dev,
              WindowPtr child,
              WindowPtr ancestor,
              int mode,
              int detail,
              BOOL core)
{
    WindowPtr  win;

    if (ancestor == child)
	return;
    for (win = child->parent; win != ancestor; win = win->parent)
    {
        if (core)
            CoreEnterLeaveEvent(dev, LeaveNotify, mode, detail, win,
                            child->drawable.id);
        else
            DeviceEnterLeaveEvent(dev, DeviceLeaveNotify, mode, detail, win,
                                  child->drawable.id);
        child = win;
    }
}

/**
 * Search for the first window below @win that has a pointer directly within
 * it's boundaries (excluding boundaries of its own descendants).
 * Windows including @exclude and its descendants are ignored.
 *
 * @return The child window that has the pointer within its boundaries or
 *         NULL.
 */
static WindowPtr
FirstPointerChild(WindowPtr win, WindowPtr exclude)
{
    static WindowPtr *queue = NULL;
    static int queue_size  = 256; /* allocated size of queue */

    WindowPtr child = NULL;
    int queue_len   = 0;          /* no of elements in queue */
    int queue_head  = 0;          /* pos of current element  */

    if (!win || win == exclude || !win->firstChild)
        return NULL;

    if (!queue && !(queue = xcalloc(queue_size, sizeof(WindowPtr))))
        FatalError("[dix] FirstPointerChild: OOM.\n");

    queue[0] = win;
    queue_head = 0;
    queue_len  = 1;

    while (queue_len--)
    {
        if (queue[queue_head] == exclude)
        {
            queue_head = (queue_head + 1) % queue_size;
            continue;
        }

        if (queue[queue_head] != win && HasPointer(queue[queue_head]))
            return queue[queue_head];

        child = queue[queue_head]->firstChild;
        /* pop children onto queue */
        while(child)
        {
            queue_len++;
            if (queue_len >= queue_size)
            {
                const int inc = 256;

                queue = xrealloc(queue, (queue_size + inc) * sizeof(WindowPtr));
                if (!queue)
                    FatalError("[dix] FirstPointerChild: OOM.\n");

                /* Are we wrapped around? */
                if (queue_head + queue_len > queue_size)
                {
                    memmove(&queue[queue_head + inc], &queue[queue_head],
                            (queue_size - queue_head) * sizeof(WindowPtr));
                    queue_head += inc;
                }

                queue_size += inc;
            }

            queue[(queue_head + queue_len) % queue_size] = child;
            child = child->nextSib;
        }

        queue_head = (queue_head + 1) % queue_size;
    }

    return NULL;
}

/**
 * Find the first parent of @win that has a pointer or has a child window with
 * a pointer. Traverses up to (and including) the root window if @stopBefore
 * is NULL, otherwise it stops at @stopBefore.
 * Neither @win nor @win's descendants nor @stopBefore are tested for having a
 * pointer.
 *
 * @return the window or NULL if @stopBefore was reached.
 */
static WindowPtr
FirstPointerAncestor(WindowPtr win, WindowPtr stopBefore)
{
    WindowPtr parent;

    parent = win->parent;

    while(parent && parent != stopBefore)
    {
        if (HasPointer(parent) || FirstPointerChild(parent, win))
            return parent;

        parent = parent->parent;
    }

    return NULL;
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

    CoreEnterLeaveEvents(pDev, fromWin, toWin, mode);
    DeviceEnterLeaveEvents(pDev, fromWin, toWin, mode);
}
