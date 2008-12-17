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
 * For a full description of the enter/leave model from a window's
 * perspective, see
 * http://lists.freedesktop.org/archives/xorg/2008-August/037606.html
 *
 * Additional notes:
 * -) The core protocol spec says that "In a LeaveNotify event, if a child of the
 * event window contains the initial position of the pointer, then the child
 * component is set to that child. Otherwise, it is None.  For an EnterNotify
 * event, if a child of the event window contains the final pointer position,
 * then the child component is set to that child. Otherwise, it is None."
 *
 * By inference, this means that only NotifyVirtual or NotifyNonlinearVirtual
 * events may have a subwindow set to other than None.
 */

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
DeviceEnterNotifies(DeviceIntPtr dev,
              WindowPtr ancestor,
              WindowPtr child,
              int mode,
              int detail)
{
    WindowPtr	parent = child->parent;

    if (ancestor == parent)
	return;
    DeviceEnterNotifies(dev, ancestor, parent, mode, detail);
    DeviceEnterLeaveEvent(dev, DeviceEnterNotify, mode, detail, parent,
                          child->drawable.id);
}

/**
 * Send enter notifies to all windows between @ancestor and @child (excluding
 * both). Events are sent running down the window hierarchy. This function
 * recurses.
 */
static void
CoreEnterNotifies(DeviceIntPtr dev,
                  WindowPtr ancestor,
                  WindowPtr child,
                  int mode,
                  int detail)
{
    WindowPtr	parent = child->parent;
    if (ancestor == parent)
	return;
    CoreEnterNotifies(dev, ancestor, parent, mode, detail);


    /* Case 3:
     A is above W, B is a descendant

      Classically: The move generates an EnterNotify on W with a detail of
        Virtual or NonlinearVirtual

     MPX:
        Case 3A: There is at least one other pointer on W itself
          P(W) doesn't change, so the event should be suppressed
        Case 3B: Otherwise, if there is at least one other pointer in a
          descendant
          P(W) stays on the same descendant, or changes to a different
          descendant. The event should be suppressed.
        Case 3C: Otherwise:
          P(W) moves from a window above W to a descendant. The subwindow
          field is set to the child containing the descendant. The detail
          may need to be changed from Virtual to NonlinearVirtual depending
          on the previous P(W). */

    if (!HasPointer(parent) && !FirstPointerChild(parent, None))
            CoreEnterLeaveEvent(dev, EnterNotify, mode, detail, parent,
                                child->drawable.id);
}

static void
CoreLeaveNotifies(DeviceIntPtr dev,
                  WindowPtr child,
                  WindowPtr ancestor,
                  int mode,
                  int detail)
{
    WindowPtr  win;

    if (ancestor == child)
        return;

    for (win = child->parent; win != ancestor; win = win->parent)
    {
        /*Case 7:
        A is a descendant of W, B is above W

        Classically: A LeaveNotify is generated on W with a detail of Virtual
          or NonlinearVirtual.

        MPX:
            Case 3A: There is at least one other pointer on W itself
              P(W) doesn't change, the event should be suppressed.
            Case 3B: Otherwise, if there is at least one other pointer in a
            descendant
             P(W) stays on the same descendant, or changes to a different
              descendant. The event should be suppressed.
            Case 3C: Otherwise:
              P(W) changes from the descendant of W to a window above W.
              The detail may need to be changed from Virtual to NonlinearVirtual
              or vice-versa depending on the new P(W).*/

        /* If one window has a pointer or a child with a pointer, skip some
         * work and exit. */
        if (HasPointer(win) || FirstPointerChild(win, None))
            return;

        CoreEnterLeaveEvent(dev, LeaveNotify, mode, detail, win, child->drawable.id);

        child = win;
    }
}

/**
 * Send leave notifies to all windows between @child and @ancestor.
 * Events are sent running up the hierarchy.
 */
static void
DeviceLeaveNotifies(DeviceIntPtr dev,
              WindowPtr child,
              WindowPtr ancestor,
              int mode,
              int detail)
{
    WindowPtr  win;

    if (ancestor == child)
	return;
    for (win = child->parent; win != ancestor; win = win->parent)
    {
        DeviceEnterLeaveEvent(dev, DeviceLeaveNotify, mode, detail, win,
                                  child->drawable.id);
        child = win;
    }
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
    WindowPtr X = CommonAncestor(A, B);
    /* Case 4:
     A is W, B is above W

    Classically: The move generates a LeaveNotify on W with a detail of
       Ancestor or Nonlinear

     MPX:
        Case 3A: There is at least one other pointer on W itself
          P(W) doesn't change, the event should be suppressed
        Case 3B: Otherwise, if there is at least one other pointer in a
        descendant of W
          P(W) changes from W to a descendant of W. The subwindow field
          is set to the child containing the new P(W), the detail field
          is set to Inferior
        Case 3C: Otherwise:
          The pointer window moves from W to a window above W.
          The detail may need to be changed from Ancestor to Nonlinear or
          vice versa depending on the the new P(W)
     */

    if (!HasPointer(A))
    {
        WindowPtr child = FirstPointerChild(A, None);
        if (child)
            CoreEnterLeaveEvent(dev, LeaveNotify, mode, NotifyInferior, A, None);
        else
            CoreEnterLeaveEvent(dev, LeaveNotify, mode, NotifyNonlinear, A, None);
    }


    CoreLeaveNotifies(dev, A, X, mode, NotifyNonlinearVirtual);

    /*
      Case 9:
        A is a descendant of W, B is a descendant of W

        Classically: No events are generated on W
        MPX: The pointer window stays the same or moves to a different
          descendant of W. No events should be generated on W.


       Therefore, no event to X.
    */

    CoreEnterNotifies(dev, X, B, mode, NotifyNonlinearVirtual);

    /* Case 2:
      A is above W, B=W

      Classically: The move generates an EnterNotify on W with a detail of
        Ancestor or Nonlinear

      MPX:
        Case 2A: There is at least one other pointer on W itself
          P(W) doesn't change, so the event should be suppressed
        Case 2B: Otherwise, if there is at least one other pointer in a
          descendant
          P(W) moves from a descendant to W. detail is changed to Inferior,
          subwindow is set to the child containing the previous P(W)
        Case 2C: Otherwise:
          P(W) changes from a window above W to W itself.
          The detail may need to be changed from Ancestor to Nonlinear
          or vice-versa depending on the previous P(W). */

     if (!HasPointer(B))
     {
         WindowPtr child = FirstPointerChild(B, None);
         if (child)
             CoreEnterLeaveEvent(dev, EnterNotify, mode, NotifyInferior, B, None);
         else
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
    /* Case 4:
     A is W, B is above W

    Classically: The move generates a LeaveNotify on W with a detail of
       Ancestor or Nonlinear

     MPX:
        Case 3A: There is at least one other pointer on W itself
          P(W) doesn't change, the event should be suppressed
        Case 3B: Otherwise, if there is at least one other pointer in a
        descendant of W
          P(W) changes from W to a descendant of W. The subwindow field
          is set to the child containing the new P(W), the detail field
          is set to Inferior
        Case 3C: Otherwise:
          The pointer window moves from W to a window above W.
          The detail may need to be changed from Ancestor to Nonlinear or
          vice versa depending on the the new P(W)
     */
    if (!HasPointer(A))
    {
        WindowPtr child = FirstPointerChild(A, None);
        if (child)
            CoreEnterLeaveEvent(dev, LeaveNotify, mode, NotifyInferior, A, None);
        else
            CoreEnterLeaveEvent(dev, LeaveNotify, mode, NotifyAncestor, A, None);
    }

    CoreLeaveNotifies(dev, A, B, mode, NotifyVirtual);

    /* Case 8:
    A is a descendant of W, B is W

    Classically: A EnterNotify is generated on W with a detail of
        NotifyInferior

    MPX:
        Case 3A: There is at least one other pointer on W itself
          P(W) doesn't change, the event should be suppressed
        Case 3B: Otherwise:
          P(W) changes from a descendant to W itself. The subwindow
          field should be set to the child containing the old P(W) <<< WRONG */

    if (!HasPointer(B))
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
    /* Case 6:
    A is W, B is a descendant of W

    Classically: A LeaveNotify is generated on W with a detail of
       NotifyInferior

    MPX:
        Case 3A: There is at least one other pointer on W itself
          P(W) doesn't change, the event should be suppressed
        Case 3B: Otherwise:
          P(W) changes from W to a descendant of W. The subwindow field
          is set to the child containing the new P(W) <<< THIS IS WRONG */

    if (!HasPointer(A))
        CoreEnterLeaveEvent(dev, LeaveNotify, mode, NotifyInferior, A, None);


    CoreEnterNotifies(dev, A, B, mode, NotifyVirtual);

    /* Case 2:
      A is above W, B=W

      Classically: The move generates an EnterNotify on W with a detail of
        Ancestor or Nonlinear

      MPX:
        Case 2A: There is at least one other pointer on W itself
          P(W) doesn't change, so the event should be suppressed
        Case 2B: Otherwise, if there is at least one other pointer in a
          descendant
          P(W) moves from a descendant to W. detail is changed to Inferior,
          subwindow is set to the child containing the previous P(W)
        Case 2C: Otherwise:
          P(W) changes from a window above W to W itself.
          The detail may need to be changed from Ancestor to Nonlinear
          or vice-versa depending on the previous P(W). */

     if (!HasPointer(B))
     {
         WindowPtr child = FirstPointerChild(B, None);
         if (child)
             CoreEnterLeaveEvent(dev, EnterNotify, mode, NotifyInferior, B, None);
         else
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
        DeviceEnterNotifies(dev, from, to, mode, NotifyVirtual);
        DeviceEnterLeaveEvent(dev, DeviceEnterNotify, mode, NotifyAncestor, to, None);
    }
    else if (IsParent(to, from))
    {
	DeviceEnterLeaveEvent(dev, DeviceLeaveNotify, mode, NotifyAncestor, from, None);
	DeviceLeaveNotifies(dev, from, to, mode, NotifyVirtual);
	DeviceEnterLeaveEvent(dev, DeviceEnterNotify, mode, NotifyInferior, to, None);
    }
    else
    { /* neither from nor to is descendent of the other */
	WindowPtr common = CommonAncestor(to, from);
	/* common == NullWindow ==> different screens */
        DeviceEnterLeaveEvent(dev, DeviceLeaveNotify, mode, NotifyNonlinear, from, None);
        DeviceLeaveNotifies(dev, from, common, mode, NotifyNonlinearVirtual);
        DeviceEnterNotifies(dev, common, to, mode, NotifyNonlinearVirtual);
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
