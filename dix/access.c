/*

Copyright 2007 Peter Hutterer <peter@cs.unisa.edu.au>

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the author shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from the author.

*/

/* This file controls the access control lists for each window. 
 * Each device can be explicitely allowed or denied access to a given window.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/Xlib.h>
#include <X11/extensions/XI.h>
#include "exglobals.h"

#include "input.h"
#include "inputstr.h"
#include "windowstr.h"


/* Only one single client can be responsible for window access control. */
static ClientPtr ACClient = NULL;


/* Forward declarations */
static void acReplaceList(DeviceIntPtr** list, 
                          int* count, 
                          DeviceIntPtr* devices, 
                          int ndevices);

/* Register global window access control client
 * Return True on success or False otherwise.
 */ 

Bool
ACRegisterClient(ClientPtr client)
{
    if (ACClient && ACClient != client)
        return False;

    ACClient = client;
    return True;
}


/* Unregister global client. If client is not the registered client, nothing
 * happens and False is returned. If no client is registered, return True.
 * Returns True if client was registred and is now unregistered.
 */ 

Bool
ACUnregisterClient(ClientPtr client)
{
    if (ACClient && ACClient != client)
        return False;

    ACClient = NULL;
    return True;
}

/* Clears all access control for the window and remove the default rule,
 * depending on what is set. */
int ACClearWindowAccess(ClientPtr client,
                        WindowPtr win,
                        int what)
{
    if (client != ACClient && client != wClient(win))
        return BadAccess;

    if (!win->optional)
    {
        /* we shouldn't get here if programmers know what they're doing. 
         * A client should not request to clear a window's access controls 
         * if they've never been set before anyway. If they do, do nothing and
         * let the client figure out what to do next.
         */
        return Success;
    }

    if (what & WindowAccessClearPerm)
    {
        xfree(win->optional->access.perm);
        win->optional->access.perm = NULL;
        win->optional->access.nperm = 0;
    }

    if (what & WindowAccessClearDeny)
    {
        xfree(win->optional->access.deny);
        win->optional->access.deny = NULL;
        win->optional->access.ndeny = 0;
    }

    if (what & WindowAccessClearRule)
        win->optional->access.defaultRule = WindowAccessNoRule;

    return Success;
}

/*
 * Changes window access control.
 *
 * Returns Success or BadAccess if the client is not allowed to change
 * anything.
 */

int
ACChangeWindowAccess(ClientPtr client, 
                     WindowPtr win, 
                     int defaultRule,
                     DeviceIntPtr* perm_devices,
                     int nperm,
                     DeviceIntPtr* deny_devices,
                     int ndeny)
{
    if (client != ACClient && client != wClient(win))
        return BadAccess;

    if (!win->optional && !MakeWindowOptional(win))
    {
        ErrorF("[dix] ACChangeWindowAcccess: Failed to make window optional.\n");
        return BadImplementation;
    }

    if (defaultRule != WindowAccessKeepRule)
        win->optional->access.defaultRule = defaultRule;

    if (nperm)
    {
        acReplaceList(&win->optional->access.perm, 
                      &win->optional->access.nperm,
                      perm_devices, nperm);
    }

    if (ndeny)
    {
        acReplaceList(&win->optional->access.deny, 
                &win->optional->access.ndeny,
                deny_devices, ndeny);
    }

    return Success;
}

static void
acReplaceList(DeviceIntPtr** list, 
              int* count, 
              DeviceIntPtr* devices, 
              int ndevices)
{
    xfree(*list);
    *list = NULL;
    *count = 0;

    if (ndevices)
    {
        *list = 
            xalloc(ndevices * sizeof(DeviceIntPtr*));
        if (!*list)
        {
            ErrorF("[dix] ACChangeWindowAccess: out of memory\n");
            return;
        }
        memcpy(*list,
                devices, 
                ndevices * sizeof(DeviceIntPtr));
        *count = ndevices;
    }
    return;
}

/*
 * Query the given window for the devices allowed to access a window.
 * The caller is responsible for freeing perm and deny.
 */
void
ACQueryWindowAccess(WindowPtr win, 
                    int* defaultRule,
                    DeviceIntPtr** perm,
                    int* nperm,
                    DeviceIntPtr** deny,
                    int* ndeny)
{
    *defaultRule = WindowAccessNoRule;
    *perm = NULL;
    *nperm = 0;
    *deny = NULL;
    *ndeny = 0;

    if (!win->optional)
        return;

    *defaultRule = win->optional->access.defaultRule;

    if (win->optional->access.nperm)
    {
        *nperm = win->optional->access.nperm;
        *perm = (DeviceIntPtr*)xalloc(*nperm * sizeof(DeviceIntPtr));
        if (!*perm)
        {
            ErrorF("[dix] ACQuerywinAccess: xalloc failure\n");
            return;
        }
        memcpy(*perm, 
               win->optional->access.perm, 
               *nperm * sizeof(DeviceIntPtr));
    }

    if (win->optional->access.ndeny)
    {
        *ndeny = win->optional->access.ndeny;
        *deny = (DeviceIntPtr*)xalloc(*ndeny * sizeof(DeviceIntPtr));
        if (!*deny)
        {
            ErrorF("[dix] ACQuerywinAccess: xalloc failure\n");
            return;
        }
        memcpy(*deny, 
               win->optional->access.deny, 
               *ndeny * sizeof(DeviceIntPtr));
    }
}

/*
 * Check if the given device is allowed to send events to the window. Returns
 * true if device is allowed or false otherwise.
 *
 * Checks are done in the following order until a result is found:
 * If the device is explicitely permitted, allow.
 * If the window has a default of DenyAll, do not allow.
 * If the device is explicitely denied, do not allow.
 * Check parent window. Rinse, wash, repeat.
 * If no rule could be found, allow.
 */
Bool
ACDeviceAllowed(WindowPtr win, DeviceIntPtr dev, xEvent* xE)
{
    int i;

    if (!win) /* happens for parent of RootWindow */
        return True;

    /* there's a number of events we don't care about */
    switch (xE->u.u.type)
    {
        case ButtonPress:
        case ButtonRelease:
        case MotionNotify:
        case EnterNotify:
        case LeaveNotify:
        case KeyPress:
        case KeyRelease:
            break;
        default:
            if (xE->u.u.type == DeviceMotionNotify ||
                    xE->u.u.type == DeviceButtonPress ||
                    xE->u.u.type == DeviceButtonRelease ||
                    xE->u.u.type == DeviceKeyPress ||
                    xE->u.u.type == DeviceKeyRelease ||
                    xE->u.u.type == DeviceEnterNotify ||
                    xE->u.u.type == DeviceLeaveNotify)
            {
                break;
            }
            return True;
    }


    if (!win->optional) /* no list, check parent */
        return ACDeviceAllowed(win->parent, dev, xE);

    for (i = 0; i < win->optional->access.nperm; i++)
    {
        if (win->optional->access.perm[i]->id == dev->id)
            return True;
    }

    if (win->optional->access.defaultRule == WindowAccessDenyAll)
        return False;

    for (i = 0; i < win->optional->access.ndeny; i++)
    {
        if (win->optional->access.deny[i]->id == dev->id)
            return False;
    }

    return ACDeviceAllowed(win->parent, dev, xE);
}

