/**************************************************************
 *
 * Quartz-specific support for the Darwin X Server
 *
 **************************************************************/
/*
 * Copyright (c) 2001-2003 Greg Parker and Torrey T. Lyons.
 *                 All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written authorization.
 */
/* $XFree86: xc/programs/Xserver/hw/darwin/quartz/quartz.c,v 1.14 2003/11/24 05:39:02 torrey Exp $ */

#include "quartzCommon.h"
#include "quartz.h"
#include "darwin.h"
#include "quartzAudio.h"
#include "pseudoramiX.h"
#define _APPLEWM_SERVER_
#include "applewm.h"
#include "applewmExt.h"

// X headers
#include "scrnintstr.h"
#include "colormapst.h"

// System headers
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <IOKit/pwr_mgt/IOPMLib.h>

// Shared global variables for Quartz modes
int                     quartzEventWriteFD = -1;
int                     quartzStartClients = 1;
int                     quartzRootless = -1;
int                     quartzUseSysBeep = 0;
int                     quartzUseAGL = 1;
int                     quartzServerVisible = TRUE;
int                     quartzServerQuitting = FALSE;
int                     quartzScreenIndex = 0;
int                     aquaMenuBarHeight = 0;
int                     noPseudoramiXExtension = TRUE;
QuartzModeProcsPtr      quartzProcs = NULL;
const char             *quartzOpenGLBundle = NULL;

/*
===========================================================================

 Screen functions

===========================================================================
*/

/*
 * DarwinModeAddScreen
 *  Do mode dependent initialization of each screen for Quartz.
 */
Bool DarwinModeAddScreen(
    int index,
    ScreenPtr pScreen)
{
    // allocate space for private per screen Quartz specific storage
    QuartzScreenPtr displayInfo = xcalloc(sizeof(QuartzScreenRec), 1);
    QUARTZ_PRIV(pScreen) = displayInfo;

    // do Quartz mode specific initialization
    return quartzProcs->AddScreen(index, pScreen);
}


/*
 * DarwinModeSetupScreen
 *  Finalize mode specific setup of each screen.
 */
Bool DarwinModeSetupScreen(
    int index,
    ScreenPtr pScreen)
{
    // do Quartz mode specific setup
    if (! quartzProcs->SetupScreen(index, pScreen))
        return FALSE;

    // setup cursor support
    if (! quartzProcs->InitCursor(pScreen))
        return FALSE;

    return TRUE;
}


/*
 * DarwinModeInitOutput
 *  Quartz display initialization.
 */
void DarwinModeInitOutput(
    int argc,
    char **argv )
{
    static unsigned long generation = 0;

    // Allocate private storage for each screen's Quartz specific info
    if (generation != serverGeneration) {
        quartzScreenIndex = AllocateScreenPrivateIndex();
        generation = serverGeneration;
    }

    if (serverGeneration == 0) {
        QuartzAudioInit();
    }

    if (!RegisterBlockAndWakeupHandlers(QuartzBlockHandler,
                                        QuartzWakeupHandler,
                                        NULL))
    {
        FatalError("Could not register block and wakeup handlers.");
    }

    // Do display mode specific initialization
    quartzProcs->DisplayInit();

    // Init PseudoramiX implementation of Xinerama.
    // This should be in InitExtensions, but that causes link errors
    // for servers that don't link in pseudoramiX.c.
    if (!noPseudoramiXExtension) {
        PseudoramiXExtensionInit(argc, argv);
    }
}


/*
 * DarwinModeInitInput
 *  Inform the main thread the X server is ready to handle events.
 */
void DarwinModeInitInput(
    int argc,
    char **argv )
{
    QuartzMessageMainThread(kQuartzServerStarted, NULL, 0);

    // Do final display mode specific initialization before handling events
    if (quartzProcs->InitInput)
        quartzProcs->InitInput(argc, argv);
}


/*
 * QuartzShow
 *  Show the X server on screen. Does nothing if already shown.
 *  Calls mode specific screen resume to restore the X clip regions
 *  (if needed) and the X server cursor state.
 */
static void QuartzShow(
    int x,	// cursor location
    int y )
{
    int i;

    if (!quartzServerVisible) {
        quartzServerVisible = TRUE;
        for (i = 0; i < screenInfo.numScreens; i++) {
            if (screenInfo.screens[i]) {
                quartzProcs->ResumeScreen(screenInfo.screens[i], x, y);
            }
        }
    }
}


/*
 * QuartzHide
 *  Remove the X server display from the screen. Does nothing if already
 *  hidden. Calls mode specific screen suspend to set X clip regions to
 *  prevent drawing (if needed) and restore the Aqua cursor.
 */
static void QuartzHide(void)
{
    int i;

    if (quartzServerVisible) {
        for (i = 0; i < screenInfo.numScreens; i++) {
            if (screenInfo.screens[i]) {
                quartzProcs->SuspendScreen(screenInfo.screens[i]);
            }
        }
    }
    quartzServerVisible = FALSE;
    QuartzMessageMainThread(kQuartzServerHidden, NULL, 0);
}


/*
 * QuartzSetRootClip
 *  Enable or disable rendering to the X screen.
 */
static void QuartzSetRootClip(
    BOOL enable)
{
    int i;

    if (!quartzServerVisible)
        return;

    for (i = 0; i < screenInfo.numScreens; i++) {
        if (screenInfo.screens[i]) {
            xf86SetRootClip(screenInfo.screens[i], enable);
        }
    }
}


/*
 * QuartzMessageServerThread
 *  Send the X server thread a message by placing it on the event queue.
 */
void
QuartzMessageServerThread(
    int type,
    int argc, ...)
{
    xEvent xe;
    INT32 *argv;
    int i, max_args;
    va_list args;

    memset(&xe, 0, sizeof(xe));
    xe.u.u.type = type;
    xe.u.clientMessage.u.l.type = type;

    argv = &xe.u.clientMessage.u.l.longs0;
    max_args = 4;

    if (argc > 0 && argc <= max_args) {
	va_start (args, argc);
	for (i = 0; i < argc; i++)
	    argv[i] = (int) va_arg (args, int);
	va_end (args);
    }

    DarwinEQEnqueue(&xe);
}


/*
 * DarwinModeProcessEvent
 *  Process Quartz specific events.
 */
void DarwinModeProcessEvent(
    xEvent *xe)
{
    switch (xe->u.u.type) {

        case kXDarwinActivate:
            QuartzShow(xe->u.keyButtonPointer.rootX,
                       xe->u.keyButtonPointer.rootY);
            AppleWMSendEvent(AppleWMActivationNotify,
                             AppleWMActivationNotifyMask,
                             AppleWMIsActive, 0);
            break;

        case kXDarwinDeactivate:
            AppleWMSendEvent(AppleWMActivationNotify,
                             AppleWMActivationNotifyMask,
                             AppleWMIsInactive, 0);
            QuartzHide();
            break;

        case kXDarwinSetRootClip:
            QuartzSetRootClip((BOOL)xe->u.clientMessage.u.l.longs0);
            break;

        case kXDarwinQuit:
            GiveUp(0);
            break;

        case kXDarwinReadPasteboard:
            QuartzReadPasteboard();
            break;

        case kXDarwinWritePasteboard:
            QuartzWritePasteboard();
            break;

        /*
         * AppleWM events
         */
        case kXDarwinControllerNotify:
            AppleWMSendEvent(AppleWMControllerNotify,
                             AppleWMControllerNotifyMask,
			     xe->u.clientMessage.u.l.longs0,
			     xe->u.clientMessage.u.l.longs1);
            break;

        case kXDarwinPasteboardNotify:
            AppleWMSendEvent(AppleWMPasteboardNotify,
                             AppleWMPasteboardNotifyMask,
                             xe->u.clientMessage.u.l.longs0,
                             xe->u.clientMessage.u.l.longs1);
            break;

        case kXDarwinDisplayChanged:
        case kXDarwinWindowState:
        case kXDarwinWindowMoved:
            // FIXME: Not implemented yet
            break;

        default:
            ErrorF("Unknown application defined event type %d.\n",
                   xe->u.u.type);
    }
}


/*
 * DarwinModeGiveUp
 *  Cleanup before X server shutdown
 *  Release the screen and restore the Aqua cursor.
 */
void DarwinModeGiveUp(void)
{
#if 0
// Trying to switch cursors when quitting causes deadlock
    int i;

    for (i = 0; i < screenInfo.numScreens; i++) {
        if (screenInfo.screens[i]) {
            QuartzSuspendXCursor(screenInfo.screens[i]);
        }
    }
#endif

    if (!quartzRootless)
        quartzProcs->ReleaseScreens();
}
