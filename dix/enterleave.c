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

/* @file This file describes the model for sending core enter/leave events in
 * the case of multiple pointers.
 * Since we can't send more than one Enter or Leave event per window
 * to a core client without confusing it, this is a rather complicated
 * approach.
 *
 * For a full description of the model from a window's perspective, see
 * http://lists.freedesktop.org/archives/xorg/2008-August/037606.html
 *
 *
 * EnterNotify(Virtual, B) means EnterNotify Event, detail Virtual, child = B.
 *
 * Pointer moves from A to B, nonlinear (CoreEnterLeaveNonLinear):
 * 1. a. if A has another pointer, goto 2.
 *    b. otherwise, if A has a child with a pointer in it,
 *       LeaveNotify(Inferior) to A
 *       LeaveNotify(Virtual) between A and child(A)
 *
 * 2. Find common ancestor X between A and B.
 * 3. Find closest pointer window P between A and X.
 *    a. if P exists
 *       LeaveNotify(Ancestor) to A
 *       LeaveNotify(Virtual) between A and P
 *    b. otherwise, if P does not exist,
 *       LeaveNotify(NonLinear) to A
 *       LeaveNotify(NonLinearVirtual) between A and X.
 *
 * 4. If X does not have a pointer, EnterNotify(NonLinearVirtual, B) to X.
 * 5. Find closest pointer window P between X and B.
 *    a. if P exists, EnterNotify(NonLinearVirtual) between X and P
 *    b. otherwise, EnterNotify(NonLinearVirtual) between X and B
 *
 * 5. a. if B has another pointer in it, finish.
 *    b. otherwise, if B has a child with a pointer in it
 *       LeaveNotify(Virtual) between child(B) and B.
 *       EnterNotify(Inferior) to B.
 *    c. otherwise, EnterNotify(NonLinear) to B.
 *
 * --------------------------------------------------------------------------
 *
 * Pointer moves from A to B, A is a parent of B (CoreEnterLeaveToDescendant):
 * 1. a. If A has another pointer, goto 2.
 *    b. Otherwise, LeaveNotify(Inferior) to A.
 *
 * 2. Find highest window X that has a pointer child that is not a child of B.
 *    a. if X exists, EnterNotify(Virtual, B) between A and X,
 *       EnterNotify(Virtual, B) to X (if X has no pointer).
 *    b. otherwise, EnterNotify(Virtual, B) between A and B.
 *
 * 3. a. if B has another pointer, finish
 *    b. otherwise, if B has a child with a pointer in it,
 *       LeaveNotify(Virtual, child(B)) between child(B) and B.
 *       EnterNotify(Inferior, child(B)) to B.
 *    c. otherwise, EnterNotify(Ancestor) to B.
 *
 * --------------------------------------------------------------------------
 *
 * Pointer moves from A to B, A is a child of B (CoreEnterLeaveToAncestor):
 * 1. a. If A has another pointer, goto 2.
 *    b. Otherwise, if A has a child with a pointer in it.
 *       LeaveNotify(Inferior, child(A)) to A.
 *       EnterNotify(Virtual, child(A)) between A and child(A).
 *       Skip to 3.
 *
 * 2. Find closest pointer window P between A and B.
 *    If P does not exist, P is B.
 *           LeaveNotify(Ancestor) to A.
 *           LeaveNotify(Virtual, A) between A and P.
 * 3. a. If B has another pointer, finish.
 *    b. otherwise, EnterNotify(Inferior) to B.
 */

#define WID(w) ((w) ? ((w)->drawable.id) : 0)

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
void
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
 * Pointer @dev moves from @A to @B and @A neither a descendant of @B nor is
 * @B a descendant of @A.
 */
static void
CoreEnterLeaveNonLinear(DeviceIntPtr dev,
                        WindowPtr A,
                        WindowPtr B,
                        int mode)
{
    WindowPtr childA, childB, X, P;
    BOOL hasPointerA = HasPointer(A);

    /* 2 */
    X = CommonAncestor(A, B);

    /* 1.a */             /* 1.b */
    if (!hasPointerA && (childA = FirstPointerChild(A, None)))
    {
        CoreEnterLeaveEvent(dev, LeaveNotify, mode, NotifyInferior, A, WID(childA));
        EnterNotifies(dev, A, childA, mode, NotifyVirtual, TRUE);
    } else {
        /* 3 */
        P = FirstPointerAncestor(A, X);

        if (P)
        {
            if (!hasPointerA)
                CoreEnterLeaveEvent(dev, LeaveNotify, mode, NotifyAncestor, A, None);
            LeaveNotifies(dev, A, P, mode, NotifyVirtual, TRUE);
        /* 3.b */
        } else
        {
            if (!hasPointerA)
                CoreEnterLeaveEvent(dev, LeaveNotify, mode, NotifyNonlinear, A, None);
            LeaveNotifies(dev, A, X, mode, NotifyNonlinearVirtual, TRUE);
        }
    }

    /* 4. */
    if (!HasPointer(X))
        CoreEnterLeaveEvent(dev, EnterNotify, mode, NotifyNonlinearVirtual, X, WID(B));

    /* 5. */
    P = FirstPointerChild(X, B);
    if (!P)
        P = B; /* 4.b */
    EnterNotifies(dev, X, P, mode, NotifyNonlinearVirtual, TRUE);

    /* 5.a */
    if (!HasOtherPointer(B, dev))
    {
       /* 5.b */
       if ((childB = FirstPointerChild(B, None)))
       {
           LeaveNotifies(dev, childB, B, mode, NotifyVirtual, TRUE);
           CoreEnterLeaveEvent(dev, EnterNotify, mode, NotifyInferior, B, WID(childB));
       } else
           /* 5.c */
           CoreEnterLeaveEvent(dev, EnterNotify, mode, NotifyNonlinear, B, None);
    }
}

/**
 * Pointer @dev moves from @A to @B and @A is a descendant of @B.
 */
static void
CoreEnterLeaveToAncestor(DeviceIntPtr dev,
                         WindowPtr A,
                         WindowPtr B,
                         int mode)
{
    WindowPtr childA = NULL, P;
    BOOL hasPointerA = HasPointer(A);

    /* 1.a */             /* 1.b */
    if (!hasPointerA && (childA = FirstPointerChild(A, None)))
    {
        CoreEnterLeaveEvent(dev, LeaveNotify, mode, NotifyInferior, A, WID(childA));
        EnterNotifies(dev, A, childA, mode, NotifyVirtual, TRUE);
    } else {
        /* 2 */
        P = FirstPointerAncestor(A, B);
        if (!P)
            P = B;

        if (!hasPointerA)
            CoreEnterLeaveEvent(dev, LeaveNotify, mode, NotifyAncestor, A, None);
        LeaveNotifies(dev, A, P, mode, NotifyVirtual, TRUE);
    }

    /* 3 */
    if (!HasOtherPointer(B, dev))
        CoreEnterLeaveEvent(dev, EnterNotify, mode, NotifyInferior, B, None);

}

/**
 * Pointer @dev moves from @A to @B and @B is a descendant of @A.
 */
static void
CoreEnterLeaveToDescendant(DeviceIntPtr dev,
                           WindowPtr A,
                           WindowPtr B,
                           int mode)
{
    WindowPtr X, childB, tmp;

    /* 1 */
    if (!HasPointer(A))
        CoreEnterLeaveEvent(dev, LeaveNotify, mode, NotifyInferior, A, WID(B));

    /* 2 */
    X = FirstPointerAncestor(B, A);
    if (X)
    {
        /* 2.a */
        tmp = X;
        while((tmp = FirstPointerAncestor(tmp, A)))
            X = tmp;
    } else /* 2.b */
        X = B;

    EnterNotifies(dev, A, X, mode, NotifyVirtual, TRUE);

    if (X != B && !HasPointer(X))
        CoreEnterLeaveEvent(dev, LeaveNotify, mode, NotifyVirtual, X, None);

    /* 3 */
    if (!HasOtherPointer(B, dev))
    {
        childB = FirstPointerChild(B, None);
        /* 3.a */
        if (childB)
        {
            LeaveNotifies(dev, childB, B, mode, NotifyVirtual, TRUE);
            CoreEnterLeaveEvent(dev, EnterNotify, mode, NotifyInferior, B, WID(childB));
        } else /* 3.c */
            CoreEnterLeaveEvent(dev, EnterNotify, mode, NotifyAncestor, B, None);
    }
}

static void
CoreEnterLeaveEvents(DeviceIntPtr dev,
                     WindowPtr from,
                     WindowPtr to,
                     int mode)
{
    if (!dev->isMaster)
        return;

    LeaveWindow(dev, from, mode);

    if (IsParent(from, to))
        CoreEnterLeaveToDescendant(dev, from, to, mode);
    else if (IsParent(to, from))
        CoreEnterLeaveToAncestor(dev, from, to, mode);
    else
        CoreEnterLeaveNonLinear(dev, from, to, mode);

    EnterWindow(dev, to, mode);
}

static void
DeviceEnterLeaveEvents(DeviceIntPtr dev,
                       WindowPtr    from,
                       WindowPtr    to,
                       int          mode)
{
    if (IsParent(from, to))
    {
        DeviceEnterLeaveEvent(dev, DeviceLeaveNotify, mode, NotifyInferior, from, None);
        EnterNotifies(dev, from, to, mode, NotifyVirtual, FALSE);
        DeviceEnterLeaveEvent(dev, DeviceEnterNotify, mode, NotifyAncestor, to, None);
    }
    else if (IsParent(to, from))
    {
	DeviceEnterLeaveEvent(dev, DeviceLeaveNotify, mode, NotifyAncestor, from, None);
	LeaveNotifies(dev, from, to, mode, NotifyVirtual, FALSE);
	DeviceEnterLeaveEvent(dev, DeviceEnterNotify, mode, NotifyInferior, to, None);
    }
    else
    { /* neither from nor to is descendent of the other */
	WindowPtr common = CommonAncestor(to, from);
	/* common == NullWindow ==> different screens */
        DeviceEnterLeaveEvent(dev, DeviceLeaveNotify, mode, NotifyNonlinear, from, None);
        LeaveNotifies(dev, from, common, mode, NotifyNonlinearVirtual, FALSE);
	EnterNotifies(dev, common, to, mode, NotifyNonlinearVirtual, FALSE);
        DeviceEnterLeaveEvent(dev, DeviceEnterNotify, mode, NotifyNonlinear, to, None);
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

    CoreEnterLeaveEvents(pDev, fromWin, toWin, mode);
    DeviceEnterLeaveEvents(pDev, fromWin, toWin, mode);
}
