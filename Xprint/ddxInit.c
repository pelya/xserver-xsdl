/* $Xorg: ddxInit.c,v 1.3 2000/08/17 19:48:07 cpqbld Exp $ */
/*
(c) Copyright 1996 Hewlett-Packard Company
(c) Copyright 1996 International Business Machines Corp.
(c) Copyright 1996 Sun Microsystems, Inc.
(c) Copyright 1996 Novell, Inc.
(c) Copyright 1996 Digital Equipment Corp.
(c) Copyright 1996 Fujitsu Limited
(c) Copyright 1996 Hitachi, Ltd.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the names of the copyright holders shall
not be used in advertising or otherwise to promote the sale, use or other
dealings in this Software without prior written authorization from said
copyright holders.
*/

#include "X.h"
#include "Xproto.h"
#include "screenint.h"
#include "input.h"
#include "misc.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "servermd.h"
#include "Xos.h"

static void Exit();
void _XpVoidNoop();

/*-
 *-----------------------------------------------------------------------
 * InitOutput --
 *	If this is built as a print-only server, then we must supply
 *      an InitOutput routine.  If a normal server's real ddx InitOutput
 *      is used, then it should call PrinterInitOutput if it so desires.
 *      The ddx-level hook is needed to allow the printer stuff to 
 *      create additional screens.  An extension can't reliably do
 *      this for two reasons:
 *
 *          1) If InitOutput doesn't create any screens, then main()
 *             exits before calling InitExtensions().
 *
 *          2) Other extensions may rely on knowing about all screens
 *             when they initialize, and we can't guarantee the order
 *             of extension initialization.
 *
 * Results:
 *	ScreenInfo filled in, and PrinterInitOutput is called to create
 *      the screens associated with printers.
 *
 * Side Effects:
 *	None
 *
 *-----------------------------------------------------------------------
 */

void 
InitOutput(pScreenInfo, argc, argv)
    ScreenInfo 	  *pScreenInfo;
    int     	  argc;
    char    	  **argv;
{
    int i;

    pScreenInfo->imageByteOrder = IMAGE_BYTE_ORDER;
    pScreenInfo->bitmapScanlineUnit = BITMAP_SCANLINE_UNIT;
    pScreenInfo->bitmapScanlinePad = BITMAP_SCANLINE_PAD;
    pScreenInfo->bitmapBitOrder = BITMAP_BIT_ORDER;

    pScreenInfo->numPixmapFormats = 0; /* get them in PrinterInitOutput */
    screenInfo.numVideoScreens = 0;
#ifdef PRINT_ONLY_SERVER
    PrinterInitOutput(pScreenInfo, argc, argv);
#endif
}

static void
BellProc(volume, pDev)
    int volume;
    DeviceIntPtr pDev;
{
    return;
}

static void
KeyControlProc(pDev, ctrl)
    DeviceIntPtr pDev;
    KeybdCtrl *ctrl;
{
    return;
}

static KeySym printKeyMap[256];
static CARD8 printModMap[256];

static int
KeyboardProc(pKbd, what, argc, argv)
    DevicePtr pKbd;
    int what;
    int argc;
    char *argv[];
{
    KeySymsRec keySyms;

    keySyms.minKeyCode = 8;
    keySyms.maxKeyCode = 8;
    keySyms.mapWidth = 1;
    keySyms.map = printKeyMap;

    switch(what)
    {
	case DEVICE_INIT:
	    InitKeyboardDeviceStruct(pKbd, &keySyms, printModMap, 
				     (BellProcPtr)BellProc,
				     KeyControlProc);
	    break;
	case DEVICE_ON:
	    break;
	case DEVICE_OFF:
	    break;
	case DEVICE_CLOSE:
	    break;
    }
    return Success;
}

#include "../mi/mipointer.h"
static int
PointerProc(pPtr, what, argc, argv)
     DevicePtr pPtr;
     int what;
     int argc;
     char *argv[];
{
#define NUM_BUTTONS 1
    CARD8 map[NUM_BUTTONS];

    switch(what)
      {
        case DEVICE_INIT:
	  {
	      map[0] = 0;
	      InitPointerDeviceStruct(pPtr, map, NUM_BUTTONS, 
				      miPointerGetMotionEvents, 
				      (PtrCtrlProcPtr)_XpVoidNoop,
				      miPointerGetMotionBufferSize());
	      break;
	  }
        case DEVICE_ON:
	  break;
        case DEVICE_OFF:
	  break;
        case DEVICE_CLOSE:
	  break;
      }
    return Success;
}

void
InitInput(argc, argv)
     int	argc;
     char **argv;
{
    DevicePtr ptr, kbd;

    ptr = AddInputDevice((DeviceProc)PointerProc, TRUE);
    kbd = AddInputDevice((DeviceProc)KeyboardProc, TRUE);
    RegisterPointerDevice(ptr);
    RegisterKeyboardDevice(kbd);
    return;
}


Bool
LegalModifier(key, dev)
     unsigned int key;
     DevicePtr dev;
{
    return TRUE;
}

void
ProcessInputEvents()
{
}

#ifdef DDXOSINIT
void
OsVendorInit()
{
}
#endif

#ifdef DDXTIME
CARD32
GetTimeInMillis()
{
    struct timeval  tp;

    X_GETTIMEOFDAY(&tp);
    return(tp.tv_sec * 1000) + (tp.tv_usec / 1000);
}
#endif

/****************************************
* ddxUseMsg()
*
* Called my usemsg from os/utils/c
*
*****************************************/

void ddxUseMsg()
{
	/* Right now, let's just do nothing */
}

static void Exit (code)
    int	code;
{
    exit (code);
}

void AbortDDX ()
{
}

void ddxGiveUp()	/* Called by GiveUp() */
{
}

int
ddxProcessArgument (argc, argv, i)
    int argc;
    char *argv[];
    int i;

{
#ifdef PRINT_ONLY_SERVER
    return XprintOptions(argc, argv, i) - i;
#else
    return(0);
#endif
}

#ifdef XINPUT

#include "XI.h"
#include "XIproto.h"

extern  int     BadDevice;

ChangePointerDevice (old_dev, new_dev, x, y)
    DeviceIntPtr        old_dev;
    DeviceIntPtr        new_dev;
    unsigned char       x,y;
{
        return (BadDevice);
}

int
ChangeDeviceControl (client, dev, control)
    register    ClientPtr       client;
    DeviceIntPtr dev;
    xDeviceCtl  *control;
{
    return BadMatch;
}

OpenInputDevice (dev, client, status)
    DeviceIntPtr dev;
    ClientPtr client;
    int *status;
{
    return;
}

AddOtherInputDevices ()
{
    return;
}

CloseInputDevice (dev, client)
    DeviceIntPtr        dev;
    ClientPtr           client;
{
    return;
}

ChangeKeyboardDevice (old_dev, new_dev)
    DeviceIntPtr        old_dev;
    DeviceIntPtr        new_dev;
{
    return (Success);
}

SetDeviceMode (client, dev, mode)
    register    ClientPtr       client;
    DeviceIntPtr dev;
    int         mode;
{
    return BadMatch;
}

SetDeviceValuators (client, dev, valuators, first_valuator, num_valuators)
    register    ClientPtr       client;
    DeviceIntPtr dev;
    int         *valuators;
    int         first_valuator;
    int         num_valuators;
{
    return BadMatch;
}


#endif /* XINPUT */

#ifdef XTESTEXT1

void
XTestJumpPointer(x, y, dev)
    int x, y, dev;
{
    return;
}

void
XTestGetPointerPos(x, y)
{
    return;
}

void
XTestGenerateEvent(dev, keycode, keystate, x, y)
    int dev, keycode, keystate, x, y;
{
    return;
}

#endif /* XTESTEXT1 */

#ifdef AIXV3
/*
 * This is just to get the server to link on AIX, where some bits
 * that should be in os/ are instead in hw/ibm.
 */
int SelectWaitTime = 10000; /* usec */
#endif
