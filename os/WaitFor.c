/***********************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

/*****************************************************************
 * OS Dependent input routines:
 *
 *  WaitForSomething
 *  TimerForce, TimerSet, TimerCheck, TimerFree
 *
 *****************************************************************/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifdef WIN32
#include <X11/Xwinsock.h>
#endif
#include <X11/Xos.h>            /* for strings, fcntl, time */
#include <errno.h>
#include <stdio.h>
#include <X11/X.h>
#include "misc.h"

#include "osdep.h"
#include "dixstruct.h"
#include "opaque.h"
#ifdef DPMSExtension
#include "dpmsproc.h"
#endif
#include "busfault.h"

#ifdef WIN32
/* Error codes from windows sockets differ from fileio error codes  */
#undef EINTR
#define EINTR WSAEINTR
#undef EINVAL
#define EINVAL WSAEINVAL
#undef EBADF
#define EBADF WSAENOTSOCK
/* Windows select does not set errno. Use GetErrno as wrapper for
   WSAGetLastError */
#define GetErrno WSAGetLastError
#else
/* This is just a fallback to errno to hide the differences between unix and
   Windows in the code */
#define GetErrno() errno
#endif

/* like ffs, but uses fd_mask instead of int as argument, so it works
   when fd_mask is longer than an int, such as common 64-bit platforms */
/* modifications by raphael */
int
mffs(fd_mask mask)
{
    int i;

    if (!mask)
        return 0;
    i = 1;
    while (!(mask & 1)) {
        i++;
        mask >>= 1;
    }
    return i;
}

#ifdef DPMSExtension
#include <X11/extensions/dpmsconst.h>
#endif

struct _OsTimerRec {
    OsTimerPtr next;
    CARD32 expires;
    CARD32 delta;
    OsTimerCallback callback;
    void *arg;
};

static void DoTimer(OsTimerPtr timer, CARD32 now, volatile OsTimerPtr *prev);
static void CheckAllTimers(void);
static volatile OsTimerPtr timers = NULL;

/*****************
 * WaitForSomething:
 *     Make the server suspend until there is
 *	1. data from clients or
 *	2. input events available or
 *	3. ddx notices something of interest (graphics
 *	   queue ready, etc.) or
 *	4. clients that have buffered replies/events are ready
 *
 *     If the time between INPUT events is
 *     greater than ScreenSaverTime, the display is turned off (or
 *     saved, depending on the hardware).  So, WaitForSomething()
 *     has to handle this also (that's why the select() has a timeout.
 *     For more info on ClientsWithInput, see ReadRequestFromClient().
 *     pClientsReady is an array to store ready client->index values into.
 *****************/

Bool
WaitForSomething(Bool are_ready)
{
    int i;
    int timeout;
    int pollerr;
    static Bool were_ready;
    Bool timer_is_running;
    CARD32 now = 0;

    timer_is_running = were_ready;

    if (were_ready && !are_ready) {
        timer_is_running = FALSE;
        SmartScheduleStopTimer();
    }

    were_ready = FALSE;

#ifdef BUSFAULT
    busfault_check();
#endif

    /* We need a while loop here to handle
       crashed connections and the screen saver timeout */
    while (1) {
        /* deal with any blocked jobs */
        if (workQueue)
            ProcessWorkQueue();

        if (are_ready) {
            timeout = 0;
        }
        else {
            timeout = -1;
            if (timers) {
                now = GetTimeInMillis();
                timeout = timers->expires - now;
                if (timeout > 0 && timeout > timers->delta + 250) {
                    /* time has rewound.  reset the timers. */
                    CheckAllTimers();
                }

                if (timers) {
                    timeout = timers->expires - now;
                    if (timeout < 0)
                        timeout = 0;
                }
            }
        }

        BlockHandler(&timeout);
        if (NewOutputPending)
            FlushAllOutput();
        /* keep this check close to select() call to minimize race */
        if (dispatchException)
            i = -1;
        else
            i = ospoll_wait(server_poll, timeout);
        pollerr = GetErrno();
        WakeupHandler(i);
        if (i <= 0) {           /* An error or timeout occurred */
            if (dispatchException)
                return FALSE;
            if (i < 0) {
                if (pollerr == EBADF) {       /* Some client disconnected */
                    CheckConnections();
                    return FALSE;
                }
                else if (pollerr == EINVAL) {
                    FatalError("WaitForSomething(): poll: %s\n",
                               strerror(pollerr));
                }
                else if (pollerr != EINTR && pollerr != EAGAIN) {
                    ErrorF("WaitForSomething(): poll: %s\n",
                           strerror(pollerr));
                }
            }
            else if (are_ready) {
                /*
                 * If no-one else is home, bail quickly
                 */
                break;
            }
            if (*checkForInput[0] != *checkForInput[1])
                return FALSE;

            if (timers) {
                int expired = 0;

                now = GetTimeInMillis();
                if ((int) (timers->expires - now) <= 0)
                    expired = 1;

                if (expired) {
                    OsBlockSignals();
                    while (timers && (int) (timers->expires - now) <= 0)
                        DoTimer(timers, now, &timers);
                    OsReleaseSignals();

                    return FALSE;
                }
            }
        }
        else {
            /* check here for DDXes that queue events during Block/Wakeup */
            if (*checkForInput[0] != *checkForInput[1])
                return FALSE;

            if (timers) {
                int expired = 0;

                now = GetTimeInMillis();
                if ((int) (timers->expires - now) <= 0)
                    expired = 1;

                if (expired) {
                    OsBlockSignals();
                    while (timers && (int) (timers->expires - now) <= 0)
                        DoTimer(timers, now, &timers);
                    OsReleaseSignals();

                    return FALSE;
                }
            }

            are_ready = clients_are_ready();
            if (are_ready)
                break;
        }
    }

    if (are_ready) {
        were_ready = TRUE;
        if (!timer_is_running)
            SmartScheduleStartTimer();
    }

    return TRUE;
}

void
AdjustWaitForDelay(void *waitTime, int newdelay)
{
    int *timeoutp = waitTime;
    int timeout = *timeoutp;

    if (timeout < 0 || newdelay < timeout)
        *timeoutp = newdelay;
}

/* If time has rewound, re-run every affected timer.
 * Timers might drop out of the list, so we have to restart every time. */
static void
CheckAllTimers(void)
{
    OsTimerPtr timer;
    CARD32 now;

    input_lock();
 start:
    now = GetTimeInMillis();

    for (timer = timers; timer; timer = timer->next) {
        if (timer->expires - now > timer->delta + 250) {
            TimerForce(timer);
            goto start;
        }
    }
    input_unlock();
}

static void
DoTimer(OsTimerPtr timer, CARD32 now, volatile OsTimerPtr *prev)
{
    CARD32 newTime;

    input_lock();
    *prev = timer->next;
    timer->next = NULL;
    input_unlock();

    newTime = (*timer->callback) (timer, now, timer->arg);
    if (newTime)
        TimerSet(timer, 0, newTime, timer->callback, timer->arg);
}

OsTimerPtr
TimerSet(OsTimerPtr timer, int flags, CARD32 millis,
         OsTimerCallback func, void *arg)
{
    volatile OsTimerPtr *prev;
    CARD32 now = GetTimeInMillis();

    if (!timer) {
        timer = malloc(sizeof(struct _OsTimerRec));
        if (!timer)
            return NULL;
    }
    else {
        input_lock();
        for (prev = &timers; *prev; prev = &(*prev)->next) {
            if (*prev == timer) {
                *prev = timer->next;
                if (flags & TimerForceOld)
                    (void) (*timer->callback) (timer, now, timer->arg);
                break;
            }
        }
        input_unlock();
    }
    if (!millis)
        return timer;
    if (flags & TimerAbsolute) {
        timer->delta = millis - now;
    }
    else {
        timer->delta = millis;
        millis += now;
    }
    timer->expires = millis;
    timer->callback = func;
    timer->arg = arg;
    if ((int) (millis - now) <= 0) {
        timer->next = NULL;
        millis = (*timer->callback) (timer, now, timer->arg);
        if (!millis)
            return timer;
    }
    input_lock();
    for (prev = &timers;
         *prev && (int) ((*prev)->expires - millis) <= 0;
         prev = &(*prev)->next);
    timer->next = *prev;
    *prev = timer;
    input_unlock();
    return timer;
}

Bool
TimerForce(OsTimerPtr timer)
{
    int rc = FALSE;
    volatile OsTimerPtr *prev;

    input_lock();
    for (prev = &timers; *prev; prev = &(*prev)->next) {
        if (*prev == timer) {
            DoTimer(timer, GetTimeInMillis(), prev);
            rc = TRUE;
            break;
        }
    }
    input_unlock();
    return rc;
}

void
TimerCancel(OsTimerPtr timer)
{
    volatile OsTimerPtr *prev;

    if (!timer)
        return;
    input_lock();
    for (prev = &timers; *prev; prev = &(*prev)->next) {
        if (*prev == timer) {
            *prev = timer->next;
            break;
        }
    }
    input_unlock();
}

void
TimerFree(OsTimerPtr timer)
{
    if (!timer)
        return;
    TimerCancel(timer);
    free(timer);
}

void
TimerCheck(void)
{
    CARD32 now = GetTimeInMillis();

    if (timers && (int) (timers->expires - now) <= 0) {
        input_lock();
        while (timers && (int) (timers->expires - now) <= 0)
            DoTimer(timers, now, &timers);
        input_unlock();
    }
}

void
TimerInit(void)
{
    OsTimerPtr timer;

    while ((timer = timers)) {
        timers = timer->next;
        free(timer);
    }
}

#ifdef DPMSExtension

#define DPMS_CHECK_MODE(mode,time)\
    if (time > 0 && DPMSPowerLevel < mode && timeout >= time)\
	DPMSSet(serverClient, mode);

#define DPMS_CHECK_TIMEOUT(time)\
    if (time > 0 && (time - timeout) > 0)\
	return time - timeout;

static CARD32
NextDPMSTimeout(INT32 timeout)
{
    /*
     * Return the amount of time remaining until we should set
     * the next power level. Fallthroughs are intentional.
     */
    switch (DPMSPowerLevel) {
    case DPMSModeOn:
        DPMS_CHECK_TIMEOUT(DPMSStandbyTime)

    case DPMSModeStandby:
        DPMS_CHECK_TIMEOUT(DPMSSuspendTime)

    case DPMSModeSuspend:
        DPMS_CHECK_TIMEOUT(DPMSOffTime)

    default:                   /* DPMSModeOff */
        return 0;
    }
}
#endif                          /* DPMSExtension */

static CARD32
ScreenSaverTimeoutExpire(OsTimerPtr timer, CARD32 now, void *arg)
{
    INT32 timeout = now - LastEventTime(XIAllDevices).milliseconds;
    CARD32 nextTimeout = 0;

#ifdef DPMSExtension
    /*
     * Check each mode lowest to highest, since a lower mode can
     * have the same timeout as a higher one.
     */
    if (DPMSEnabled) {
        DPMS_CHECK_MODE(DPMSModeOff, DPMSOffTime)
            DPMS_CHECK_MODE(DPMSModeSuspend, DPMSSuspendTime)
            DPMS_CHECK_MODE(DPMSModeStandby, DPMSStandbyTime)

            nextTimeout = NextDPMSTimeout(timeout);
    }

    /*
     * Only do the screensaver checks if we're not in a DPMS
     * power saving mode
     */
    if (DPMSPowerLevel != DPMSModeOn)
        return nextTimeout;
#endif                          /* DPMSExtension */

    if (!ScreenSaverTime)
        return nextTimeout;

    if (timeout < ScreenSaverTime) {
        return nextTimeout > 0 ?
            min(ScreenSaverTime - timeout, nextTimeout) :
            ScreenSaverTime - timeout;
    }

    ResetOsBuffers();           /* not ideal, but better than nothing */
    dixSaveScreens(serverClient, SCREEN_SAVER_ON, ScreenSaverActive);

    if (ScreenSaverInterval > 0) {
        nextTimeout = nextTimeout > 0 ?
            min(ScreenSaverInterval, nextTimeout) : ScreenSaverInterval;
    }

    return nextTimeout;
}

static OsTimerPtr ScreenSaverTimer = NULL;

void
FreeScreenSaverTimer(void)
{
    if (ScreenSaverTimer) {
        TimerFree(ScreenSaverTimer);
        ScreenSaverTimer = NULL;
    }
}

void
SetScreenSaverTimer(void)
{
    CARD32 timeout = 0;

#ifdef DPMSExtension
    if (DPMSEnabled) {
        /*
         * A higher DPMS level has a timeout that's either less
         * than or equal to that of a lower DPMS level.
         */
        if (DPMSStandbyTime > 0)
            timeout = DPMSStandbyTime;

        else if (DPMSSuspendTime > 0)
            timeout = DPMSSuspendTime;

        else if (DPMSOffTime > 0)
            timeout = DPMSOffTime;
    }
#endif

    if (ScreenSaverTime > 0) {
        timeout = timeout > 0 ? min(ScreenSaverTime, timeout) : ScreenSaverTime;
    }

#ifdef SCREENSAVER
    if (timeout && !screenSaverSuspended) {
#else
    if (timeout) {
#endif
        ScreenSaverTimer = TimerSet(ScreenSaverTimer, 0, timeout,
                                    ScreenSaverTimeoutExpire, NULL);
    }
    else if (ScreenSaverTimer) {
        FreeScreenSaverTimer();
    }
}
