/**************************************************************
 *
 * IOKit support for the Darwin X Server
 *
 * HISTORY:
 * Original port to Mac OS X Server by John Carmack
 * Port to Darwin 1.0 by Dave Zarzycki
 * Significantly rewritten for XFree86 4.0.1 by Torrey Lyons
 *
 **************************************************************/
/*
 * Copyright (c) 2001-2002 Torrey T. Lyons. All Rights Reserved.
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
/* $XFree86: xc/programs/Xserver/hw/darwin/iokit/xfIOKit.c,v 1.3 2003/12/09 04:42:36 torrey Exp $ */

#include "X.h"
#include "Xproto.h"
#include "os.h"
#include "servermd.h"
#include "inputstr.h"
#include "scrnintstr.h"
#include "mi.h"
#include "mibstore.h"
#include "mipointer.h"
#include "micmap.h"
#include "shadow.h"

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

#include <mach/mach_interface.h>

#define NO_CFPLUGIN
#include <IOKit/IOKitLib.h>
#include <IOKit/hidsystem/IOHIDShared.h>
#include <IOKit/graphics/IOGraphicsLib.h>
#include <drivers/event_status_driver.h>

// Define this to work around bugs in the display drivers for
// older PowerBook G3's. If the X server starts without this
// #define, you don't need it.
#undef OLD_POWERBOOK_G3

#include "darwin.h"
#include "xfIOKit.h"

// Globals
int             xfIOKitScreenIndex = 0;
io_connect_t    xfIOKitInputConnect = 0;

static pthread_t                inputThread;
static EvGlobals *              evg;
static mach_port_t              masterPort;
static mach_port_t              notificationPort;
static IONotificationPortRef    NotificationPortRef;
static mach_port_t              pmNotificationPort;
static io_iterator_t            fbIter;


/*
 * XFIOKitStoreColors
 * This is a callback from X to change the hardware colormap
 * when using PsuedoColor.
 */
static void XFIOKitStoreColors(
    ColormapPtr     pmap,
    int             numEntries,
    xColorItem      *pdefs)
{
    kern_return_t   kr;
    int             i;
    IOColorEntry    *newColors;
    ScreenPtr       pScreen = pmap->pScreen;
    XFIOKitScreenPtr iokitScreen = XFIOKIT_SCREEN_PRIV(pScreen);

    assert( newColors = (IOColorEntry *)
                xalloc( numEntries*sizeof(IOColorEntry) ));

    // Convert xColorItem values to IOColorEntry
    // assume the colormap is PsuedoColor
    // as we do not support DirectColor
    for (i = 0; i < numEntries; i++) {
        newColors[i].index = pdefs[i].pixel;
        newColors[i].red =   pdefs[i].red;
        newColors[i].green = pdefs[i].green;
        newColors[i].blue =  pdefs[i].blue;
    }

    kr = IOFBSetCLUT( iokitScreen->fbService, 0, numEntries,
                      kSetCLUTByValue, newColors );
    kern_assert( kr );

    xfree( newColors );
}


/*
 * DarwinModeBell
 *  FIXME
 */
void DarwinModeBell(
    int             loud,
    DeviceIntPtr    pDevice,
    pointer         ctrl,
    int             fbclass)
{
}


/*
 * DarwinModeGiveUp
 *  Closes the connections to IOKit services
 */
void DarwinModeGiveUp( void )
{
    int i;

    // we must close the HID System first
    // because it is a client of the framebuffer
    NXCloseEventStatus( darwinParamConnect );
    IOServiceClose( xfIOKitInputConnect );
    for (i = 0; i < screenInfo.numScreens; i++) {
        XFIOKitScreenPtr iokitScreen =
                            XFIOKIT_SCREEN_PRIV(screenInfo.screens[i]);
        IOServiceClose( iokitScreen->fbService );
    }
}


/*
 * ClearEvent
 *  Clear an event from the HID System event queue
 */
static void ClearEvent(NXEvent * ep)
{
    static NXEvent nullEvent = {NX_NULLEVENT, {0, 0 }, 0, -1, 0 };

    *ep = nullEvent;
    ep->data.compound.subType = ep->data.compound.misc.L[0] =
                                ep->data.compound.misc.L[1] = 0;
}


/*
 * XFIOKitHIDThread
 *  Read the HID System event queue, translate it to an X event,
 *  and queue it for processing.
 */
static void *XFIOKitHIDThread(void *unused)
{
    for (;;) {
        NXEQElement             *oldHead;
        mach_msg_return_t       kr;
        mach_msg_empty_rcv_t    msg;

        kr = mach_msg((mach_msg_header_t*) &msg, MACH_RCV_MSG, 0,
                      sizeof(msg), notificationPort, 0, MACH_PORT_NULL);
        kern_assert(kr);

        while (evg->LLEHead != evg->LLETail) {
            NXEvent ev;
            xEvent xe;

            // Extract the next event from the kernel queue
            oldHead = (NXEQElement*)&evg->lleq[evg->LLEHead];
            ev_lock(&oldHead->sema);
            ev = oldHead->event;
            ClearEvent(&oldHead->event);
            evg->LLEHead = oldHead->next;
            ev_unlock(&oldHead->sema);

            memset(&xe, 0, sizeof(xe));

            // These fields should be filled in for every event
            xe.u.keyButtonPointer.rootX = ev.location.x;
            xe.u.keyButtonPointer.rootY = ev.location.y;
            xe.u.keyButtonPointer.time = GetTimeInMillis();

            switch( ev.type ) {
                case NX_MOUSEMOVED:
                    xe.u.u.type = MotionNotify;
                    break;

                case NX_LMOUSEDOWN:
                    xe.u.u.type = ButtonPress;
                    xe.u.u.detail = 1;
                    break;

                case NX_LMOUSEUP:
                    xe.u.u.type = ButtonRelease;
                    xe.u.u.detail = 1;
                    break;

                // A newer kernel generates multi-button events with
                // NX_SYSDEFINED. Button 2 isn't handled correctly by
                // older kernels anyway. Just let NX_SYSDEFINED events
                // handle these.
#if 0
                case NX_RMOUSEDOWN:
                    xe.u.u.type = ButtonPress;
                    xe.u.u.detail = 2;
                    break;

                case NX_RMOUSEUP:
                    xe.u.u.type = ButtonRelease;
                    xe.u.u.detail = 2;
                    break;
#endif

                case NX_KEYDOWN:
                    xe.u.u.type = KeyPress;
                    xe.u.u.detail = ev.data.key.keyCode;
                    break;

                case NX_KEYUP:
                    xe.u.u.type = KeyRelease;
                    xe.u.u.detail = ev.data.key.keyCode;
                    break;

                case NX_FLAGSCHANGED:
                    xe.u.u.type = kXDarwinUpdateModifiers;
                    xe.u.clientMessage.u.l.longs0 = ev.flags;
                    break;

                case NX_SYSDEFINED:
                    if (ev.data.compound.subType == 7) {
                        xe.u.u.type = kXDarwinUpdateButtons;
                        xe.u.clientMessage.u.l.longs0 =
                                        ev.data.compound.misc.L[0];
                        xe.u.clientMessage.u.l.longs1 =
                                        ev.data.compound.misc.L[1];
                    } else {
                        continue;
                    }
                    break;

                case NX_SCROLLWHEELMOVED:
                    xe.u.u.type = kXDarwinScrollWheel;
                    xe.u.clientMessage.u.s.shorts0 =
                                    ev.data.scrollWheel.deltaAxis1;
                    break;

                default:
                    continue;
            }
 
            DarwinEQEnqueue(&xe);
        }
    }

    return NULL;
}


/*
 * XFIOKitPMThread
 *  Handle power state notifications
 */
static void *XFIOKitPMThread(void *arg)
{
    ScreenPtr pScreen = (ScreenPtr)arg;
    XFIOKitScreenPtr iokitScreen = XFIOKIT_SCREEN_PRIV(pScreen);

    for (;;) {
        mach_msg_return_t       kr;
        mach_msg_empty_rcv_t    msg;

        kr = mach_msg((mach_msg_header_t*) &msg, MACH_RCV_MSG, 0,
                      sizeof(msg), pmNotificationPort, 0, MACH_PORT_NULL);
        kern_assert(kr);

        // display is powering down
        if (msg.header.msgh_id == 0) {
            IOFBAcknowledgePM( iokitScreen->fbService );
            xf86SetRootClip(pScreen, FALSE);
        }
        // display just woke up
        else if (msg.header.msgh_id == 1) {
            xf86SetRootClip(pScreen, TRUE);
        }
    }
    return NULL;
}


/*
 * SetupFBandHID
 *  Setup an IOFramebuffer service and connect the HID system to it.
 */
static Bool SetupFBandHID(
    int                    index,
    DarwinFramebufferPtr   dfb,
    XFIOKitScreenPtr       iokitScreen)
{
    kern_return_t           kr;
    io_service_t            service;
    io_connect_t            fbService;
    vm_address_t            vram;
    vm_size_t               shmemSize;
    int                     i;
    UInt32                  numModes;
    IODisplayModeInformation modeInfo;
    IODisplayModeID         displayMode, *allModes;
    IOIndex                 displayDepth;
    IOFramebufferInformation fbInfo;
    IOPixelInformation      pixelInfo;
    StdFBShmem_t            *cshmem;

    // find and open the IOFrameBuffer service
    service = IOIteratorNext(fbIter);
    if (service == 0)
        return FALSE;

    kr = IOServiceOpen( service, mach_task_self(),
                        kIOFBServerConnectType, &iokitScreen->fbService );
    IOObjectRelease( service );
    if (kr != KERN_SUCCESS) {
        ErrorF("Failed to connect as window server to screen %i.\n", index);
        return FALSE;
    }
    fbService = iokitScreen->fbService;

    // create the slice of shared memory containing cursor state data
    kr = IOFBCreateSharedCursor( fbService,
                                 kIOFBCurrentShmemVersion,
                                 32, 32 );
    if (kr != KERN_SUCCESS)
        return FALSE;

    // Register for power management events for the framebuffer's device
    kr = IOCreateReceivePort(kOSNotificationMessageID, &pmNotificationPort);
    kern_assert(kr);
    kr = IOConnectSetNotificationPort( fbService, 0,
                                       pmNotificationPort, 0 );
    if (kr != KERN_SUCCESS) {
        ErrorF("Power management registration failed.\n");
    }

    // SET THE SCREEN PARAMETERS
    // get the current screen resolution, refresh rate and depth
    kr = IOFBGetCurrentDisplayModeAndDepth( fbService,
                                            &displayMode,
                                            &displayDepth );
    if (kr != KERN_SUCCESS)
        return FALSE;

    // use the current screen resolution if the user
    // only wants to change the refresh rate
    if (darwinDesiredRefresh != -1 && darwinDesiredWidth == 0) {
        kr = IOFBGetDisplayModeInformation( fbService,
                                            displayMode,
                                            &modeInfo );
        if (kr != KERN_SUCCESS)
            return FALSE;
        darwinDesiredWidth = modeInfo.nominalWidth;
        darwinDesiredHeight = modeInfo.nominalHeight;
    }

    // use the current resolution and refresh rate
    // if the user doesn't have a preference
    if (darwinDesiredWidth == 0) {

        // change the pixel depth if desired
        if (darwinDesiredDepth != -1) {
            kr = IOFBGetDisplayModeInformation( fbService,
                                                displayMode,
                                                &modeInfo );
            if (kr != KERN_SUCCESS)
                return FALSE;
            if (modeInfo.maxDepthIndex < darwinDesiredDepth) {
                ErrorF("Discarding screen %i:\n", index);
                ErrorF("Current screen resolution does not support desired pixel depth.\n");
                return FALSE;
            }

            displayDepth = darwinDesiredDepth;
            kr = IOFBSetDisplayModeAndDepth( fbService, displayMode,
                                             displayDepth );
            if (kr != KERN_SUCCESS)
                return FALSE;
        }

    // look for display mode with correct resolution and refresh rate
    } else {

        // get an array of all supported display modes
        kr = IOFBGetDisplayModeCount( fbService, &numModes );
        if (kr != KERN_SUCCESS)
            return FALSE;
        assert(allModes = (IODisplayModeID *)
                xalloc( numModes * sizeof(IODisplayModeID) ));
        kr = IOFBGetDisplayModes( fbService, numModes, allModes );
        if (kr != KERN_SUCCESS)
            return FALSE;

        for (i = 0; i < numModes; i++) {
            kr = IOFBGetDisplayModeInformation( fbService, allModes[i],
                                                &modeInfo );
            if (kr != KERN_SUCCESS)
                return FALSE;

            if (modeInfo.flags & kDisplayModeValidFlag &&
                modeInfo.nominalWidth == darwinDesiredWidth &&
                modeInfo.nominalHeight == darwinDesiredHeight) {

                if (darwinDesiredDepth == -1)
                    darwinDesiredDepth = modeInfo.maxDepthIndex;
                if (modeInfo.maxDepthIndex < darwinDesiredDepth) {
                    ErrorF("Discarding screen %i:\n", index);
                    ErrorF("Desired screen resolution does not support desired pixel depth.\n");
                    return FALSE;
                }

                if ((darwinDesiredRefresh == -1 ||
                    (darwinDesiredRefresh << 16) == modeInfo.refreshRate)) {
                    displayMode = allModes[i];
                    displayDepth = darwinDesiredDepth;
                    kr = IOFBSetDisplayModeAndDepth(fbService,
                                                    displayMode,
                                                    displayDepth);
                    if (kr != KERN_SUCCESS)
                        return FALSE;
                    break;
                }
            }
        }

        xfree( allModes );
        if (i >= numModes) {
            ErrorF("Discarding screen %i:\n", index);
            ErrorF("Desired screen resolution or refresh rate is not supported.\n");
            return FALSE;
        }
    }

    kr = IOFBGetPixelInformation( fbService, displayMode, displayDepth,
                                  kIOFBSystemAperture, &pixelInfo );
    if (kr != KERN_SUCCESS)
        return FALSE;

#ifdef __i386__
    /* x86 in 8bit mode currently needs fixed color map... */
    if( pixelInfo.bitsPerComponent == 8 ) {
        pixelInfo.pixelType = kIOFixedCLUTPixels;
    }
#endif

#ifdef OLD_POWERBOOK_G3
    if (pixelInfo.pixelType == kIOCLUTPixels)
        pixelInfo.pixelType = kIOFixedCLUTPixels;
#endif

    kr = IOFBGetFramebufferInformationForAperture( fbService,
                                                   kIOFBSystemAperture,
                                                   &fbInfo );
    if (kr != KERN_SUCCESS)
        return FALSE;

    // FIXME: 1x1 IOFramebuffers are sometimes used to indicate video
    // outputs without a monitor connected to them. Since IOKit Xinerama
    // does not really work, this often causes problems on PowerBooks.
    // For now we explicitly check and ignore these screens.
    if (fbInfo.activeWidth <= 1 || fbInfo.activeHeight <= 1) {
        ErrorF("Discarding screen %i:\n", index);
        ErrorF("Invalid width or height.\n");
        return FALSE;
    }

    kr = IOConnectMapMemory( fbService, kIOFBCursorMemory,
                             mach_task_self(), (vm_address_t *) &cshmem,
                             &shmemSize, kIOMapAnywhere );
    if (kr != KERN_SUCCESS)
        return FALSE;
    iokitScreen->cursorShmem = cshmem;

    kr = IOConnectMapMemory( fbService, kIOFBSystemAperture,
                             mach_task_self(), &vram, &shmemSize,
                             kIOMapAnywhere );
    if (kr != KERN_SUCCESS)
        return FALSE;

    iokitScreen->framebuffer = (void*)vram;
    dfb->x = cshmem->screenBounds.minx;
    dfb->y = cshmem->screenBounds.miny;
    dfb->width = fbInfo.activeWidth;
    dfb->height = fbInfo.activeHeight;
    dfb->pitch = fbInfo.bytesPerRow;
    dfb->bitsPerPixel = fbInfo.bitsPerPixel;
    dfb->colorBitsPerPixel = pixelInfo.componentCount *
                             pixelInfo.bitsPerComponent;
    dfb->bitsPerComponent = pixelInfo.bitsPerComponent;

    // allocate shadow framebuffer
    iokitScreen->shadowPtr = xalloc(dfb->pitch * dfb->height);
    dfb->framebuffer = iokitScreen->shadowPtr;

    // Note: Darwin kIORGBDirectPixels = X TrueColor, not DirectColor
    if (pixelInfo.pixelType == kIORGBDirectPixels) {
        dfb->colorType = TrueColor;
    } else if (pixelInfo.pixelType == kIOCLUTPixels) {
        dfb->colorType = PseudoColor;
    } else if (pixelInfo.pixelType == kIOFixedCLUTPixels) {
        dfb->colorType = StaticColor;
    }

    // Inform the HID system that the framebuffer is also connected to it.
    kr = IOConnectAddClient( xfIOKitInputConnect, fbService );
    kern_assert( kr );

    // We have to have added at least one screen
    // before we can enable the cursor.
    kr = IOHIDSetCursorEnable(xfIOKitInputConnect, TRUE);
    kern_assert( kr );

    return TRUE;
}


/*
 * DarwinModeAddScreen
 *  IOKit specific initialization for each screen.
 */
Bool DarwinModeAddScreen(
    int index,
    ScreenPtr pScreen)
{
    DarwinFramebufferPtr dfb = SCREEN_PRIV(pScreen);
    XFIOKitScreenPtr iokitScreen;

    // allocate space for private per screen storage
    iokitScreen = xalloc(sizeof(XFIOKitScreenRec));
    XFIOKIT_SCREEN_PRIV(pScreen) = iokitScreen;

    // setup hardware framebuffer
    iokitScreen->fbService = 0;
    if (! SetupFBandHID(index, dfb, iokitScreen)) {
        if (iokitScreen->fbService) {
            IOServiceClose(iokitScreen->fbService);
        }
        return FALSE;
    }

    return TRUE;
}


/*
 * XFIOKitShadowUpdate
 *  Update the damaged regions of the shadow framebuffer on the screen.
 */
static void XFIOKitShadowUpdate(ScreenPtr pScreen, 
                                shadowBufPtr pBuf)
{
    DarwinFramebufferPtr dfb = SCREEN_PRIV(pScreen);
    XFIOKitScreenPtr iokitScreen = XFIOKIT_SCREEN_PRIV(pScreen);
    RegionPtr damage = &pBuf->damage;
    int numBox = REGION_NUM_RECTS(damage);
    BoxPtr pBox = REGION_RECTS(damage);
    int pitch = dfb->pitch;
    int bpp = dfb->bitsPerPixel/8;

    // Loop through all the damaged boxes
    while (numBox--) {
        int width, height, offset;
        unsigned char *src, *dst;

        width = (pBox->x2 - pBox->x1) * bpp;
        height = pBox->y2 - pBox->y1;
        offset = (pBox->y1 * pitch) + (pBox->x1 * bpp);
        src = iokitScreen->shadowPtr + offset;
        dst = iokitScreen->framebuffer + offset;

        while (height--) {
            memcpy(dst, src, width);
            dst += pitch;
            src += pitch;
        }

        // Get the next box
        pBox++;
    }
}


/*
 * DarwinModeSetupScreen
 *  Finalize IOKit specific initialization of each screen.
 */
Bool DarwinModeSetupScreen(
    int index,
    ScreenPtr pScreen)
{
    DarwinFramebufferPtr dfb = SCREEN_PRIV(pScreen);
    pthread_t pmThread;

    // initalize cursor support
    if (! XFIOKitInitCursor(pScreen)) {
        return FALSE;
    }

    // initialize shadow framebuffer support
    if (! shadowInit(pScreen, XFIOKitShadowUpdate, NULL)) {
        ErrorF("Failed to initalize shadow framebuffer for screen %i.\n",
               index);
        return FALSE;
    }

    // initialize colormap handling as needed
    if (dfb->colorType == PseudoColor) {
        pScreen->StoreColors = XFIOKitStoreColors;
    }

    // initialize power manager handling
    pthread_create( &pmThread, NULL, XFIOKitPMThread,
                    (void *) pScreen );

    return TRUE;
}


/*
 * DarwinModeInitOutput
 *  One-time initialization of IOKit output support.
 */
void DarwinModeInitOutput(
    int argc,
    char **argv)
{
    static unsigned long    generation = 0;
    kern_return_t           kr;
    io_iterator_t           iter;
    io_service_t            service;
    vm_address_t            shmem;
    vm_size_t               shmemSize;

    ErrorF("Display mode: IOKit\n");

    // Allocate private storage for each screen's IOKit specific info
    if (generation != serverGeneration) {
        xfIOKitScreenIndex = AllocateScreenPrivateIndex();
        generation = serverGeneration;
    }

    kr = IOMasterPort(bootstrap_port, &masterPort);
    kern_assert( kr );

    // Find and open the HID System Service
    // Do this now to be sure the Mac OS X window server is not running.
    kr = IOServiceGetMatchingServices( masterPort,
                                       IOServiceMatching( kIOHIDSystemClass ),
                                       &iter );
    kern_assert( kr );

    assert( service = IOIteratorNext( iter ) );

    kr = IOServiceOpen( service, mach_task_self(), kIOHIDServerConnectType,
                        &xfIOKitInputConnect );
    if (kr != KERN_SUCCESS) {
        ErrorF("Failed to connect to the HID System as the window server!\n");
#ifdef DARWIN_WITH_QUARTZ
        FatalError("Quit the Mac OS X window server or use the -quartz option.\n");
#else
        FatalError("Make sure you have quit the Mac OS X window server.\n");
#endif
    }

    IOObjectRelease( service );
    IOObjectRelease( iter );

    // Setup the event queue in memory shared by the kernel and X server
    kr = IOHIDCreateSharedMemory( xfIOKitInputConnect,
                                  kIOHIDCurrentShmemVersion );
    kern_assert( kr );

    kr = IOConnectMapMemory( xfIOKitInputConnect, kIOHIDGlobalMemory,
                             mach_task_self(), &shmem, &shmemSize,
                             kIOMapAnywhere );
    kern_assert( kr );

    evg = (EvGlobals *)(shmem + ((EvOffsets *)shmem)->evGlobalsOffset);

    assert(sizeof(EvGlobals) == evg->structSize);

    NotificationPortRef = IONotificationPortCreate( masterPort );

    notificationPort = IONotificationPortGetMachPort(NotificationPortRef);

    kr = IOConnectSetNotificationPort( xfIOKitInputConnect,
                                       kIOHIDEventNotification,
                                       notificationPort, 0 );
    kern_assert( kr );

    evg->movedMask |= NX_MOUSEMOVEDMASK;

    // find number of framebuffers
    kr = IOServiceGetMatchingServices( masterPort,
                        IOServiceMatching( IOFRAMEBUFFER_CONFORMSTO ),
                        &fbIter );
    kern_assert( kr );

    darwinScreensFound = 0;
    while ((service = IOIteratorNext(fbIter))) {
        IOObjectRelease( service );
        darwinScreensFound++;
    }
    IOIteratorReset(fbIter);
}


/*
 * DarwinModeInitInput
 *  One-time initialization of IOKit input support.
 */
void DarwinModeInitInput(
    int argc,
    char **argv)
{
    kern_return_t           kr;
    int                     fd[2];

    kr = IOHIDSetEventsEnable(xfIOKitInputConnect, TRUE);
    kern_assert( kr );

    // Start event passing thread
    assert( pipe(fd) == 0 );
    darwinEventReadFD = fd[0];
    darwinEventWriteFD = fd[1];
    fcntl(darwinEventReadFD, F_SETFL, O_NONBLOCK);
    pthread_create(&inputThread, NULL,
                   XFIOKitHIDThread, NULL);

}


/*
 * DarwinModeProcessEvent
 *  Process IOKit specific events.
 */
void DarwinModeProcessEvent(
    xEvent *xe)
{
    // No mode specific events
    ErrorF("Unknown X event caught: %d\n", xe->u.u.type);
}
