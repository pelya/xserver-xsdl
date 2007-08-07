/************************************************************

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

********************************************************/

/* The panoramix components contained the following notice */
/*****************************************************************

Copyright (c) 1991, 1997 Digital Equipment Corporation, Maynard, Massachusetts.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
DIGITAL EQUIPMENT CORPORATION BE LIABLE FOR ANY CLAIM, DAMAGES, INCLUDING,
BUT NOT LIMITED TO CONSEQUENTIAL OR INCIDENTAL DAMAGES, OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Digital Equipment Corporation
shall not be used in advertising or otherwise to promote the sale, use or other
dealings in this Software without prior written authorization from Digital
Equipment Corporation.

******************************************************************/

/*****************************************************************

Copyright 2003-2005 Sun Microsystems, Inc.

All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, and/or sell copies of the Software, and to permit persons
to whom the Software is furnished to do so, provided that the above
copyright notice(s) and this permission notice appear in all copies of
the Software and that both the above copyright notice(s) and this
permission notice appear in supporting documentation.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
HOLDERS INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL
INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING
FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

Except as contained in this notice, the name of a copyright holder
shall not be used in advertising or otherwise to promote the sale, use
or other dealings in this Software without prior written authorization
of the copyright holder.

******************************************************************/

/* 
 * MPX additions
 * Copyright 2006 by Peter Hutterer
 * Author: Peter Hutterer <peter@cs.unisa.edu.au>
 */

/** @file
 * This file handles event delivery and a big part of the server-side protocol
 * handling (the parts for input devices).
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include <X11/keysym.h>
#include "misc.h"
#include "resource.h"
#define NEED_EVENTS
#define NEED_REPLIES
#include <X11/Xproto.h>
#include "windowstr.h"
#include "inputstr.h"
#include "scrnintstr.h"
#include "cursorstr.h"

#include "dixstruct.h"
#ifdef PANORAMIX
#include "panoramiX.h"
#include "panoramiXsrv.h"
#endif
#include "globals.h"

#ifdef XKB
#include <X11/extensions/XKBproto.h>
#include <xkbsrv.h>
extern Bool XkbFilterEvents(ClientPtr, int, xEvent *);
#endif

#include "xace.h"

#ifdef XSERVER_DTRACE
#include <sys/types.h>
typedef const char *string;
#include "Xserver-dtrace.h"
#endif

#ifdef XEVIE
extern WindowPtr *WindowTable;
extern int       xevieFlag;
extern int       xevieClientIndex;
extern DeviceIntPtr     xeviemouse;
extern DeviceIntPtr     xeviekb;
extern Mask      xevieMask;
extern Mask      xevieFilters[128];
extern int       xevieEventSent;
extern int       xevieKBEventSent;
int    xeviegrabState = 0;
static xEvent *xeviexE;
#endif

#include <X11/extensions/XIproto.h>
#include <X11/extensions/XI.h>
#include "exglobals.h"
#include "exevents.h"
#include "exglobals.h"
#include "extnsionst.h"

#include "dixevents.h"
#include "dixgrabs.h"
#include "dispatch.h"

#include <X11/extensions/ge.h>
#include "geext.h"
#include "geint.h"

/**
 * Extension events type numbering starts at EXTENSION_EVENT_BASE.
 */
#define EXTENSION_EVENT_BASE  64

#define NoSuchEvent 0x80000000	/* so doesn't match NoEventMask */
#define StructureAndSubMask ( StructureNotifyMask | SubstructureNotifyMask )
#define AllButtonsMask ( \
	Button1Mask | Button2Mask | Button3Mask | Button4Mask | Button5Mask )
#define MotionMask ( \
	PointerMotionMask | Button1MotionMask | \
	Button2MotionMask | Button3MotionMask | Button4MotionMask | \
	Button5MotionMask | ButtonMotionMask )
#define PropagateMask ( \
	KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | \
	MotionMask )
#define PointerGrabMask ( \
	ButtonPressMask | ButtonReleaseMask | \
	EnterWindowMask | LeaveWindowMask | \
	PointerMotionHintMask | KeymapStateMask | \
	MotionMask )
#define AllModifiersMask ( \
	ShiftMask | LockMask | ControlMask | Mod1Mask | Mod2Mask | \
	Mod3Mask | Mod4Mask | Mod5Mask )
#define AllEventMasks (lastEventMask|(lastEventMask-1))

/**
 * Used to indicate a implicit passive grab created by a ButtonPress event.
 * See DeliverEventsToWindow().
 */
#define ImplicitGrabMask (1 << 7)
/*
 * The following relies on the fact that the Button<n>MotionMasks are equal
 * to the corresponding Button<n>Masks from the current modifier/button state.
 */
#define Motion_Filter(class) (PointerMotionMask | \
			      (class)->state | (class)->motionMask)


#define WID(w) ((w) ? ((w)->drawable.id) : 0)

#define XE_KBPTR (xE->u.keyButtonPointer)


#define rClient(obj) (clients[CLIENT_ID((obj)->resource)])

_X_EXPORT CallbackListPtr EventCallback;
_X_EXPORT CallbackListPtr DeviceEventCallback;

#define DNPMCOUNT 8

Mask DontPropagateMasks[DNPMCOUNT];
static int DontPropagateRefCnts[DNPMCOUNT];

/**
 * Main input device struct. 
 *     inputInfo.pointer 
 *     is the core pointer. Referred to as "virtual core pointer", "VCP",
 *     "core pointer" or inputInfo.pointer. There is exactly one core pointer,
 *     but multiple devices may send core events. The VCP is only used if no
 *     physical device is connected and does not have a visible cursor. 
 *     Before the integration of MPX, any core request would operate on the
 *     VCP/VCK. Core events would always come from one of those two. Now both
 *     are only fallback devices if no physical devices are available.
 * 
 *     inputInfo.keyboard
 *     is the core keyboard ("virtual core keyboard", "VCK", "core keyboard").
 *     See inputInfo.pointer.
 * 
 *     inputInfo.devices
 *     linked list containing all devices BUT NOT INCLUDING VCK and VCP. 
 *
 *     inputInfo.off_devices
 *     Devices that have not been initialized and are thus turned off.
 *
 *     inputInfo.numDevices
 *     Total number of devices (not counting VCP and VCK).
 */
_X_EXPORT InputInfo inputInfo;

static struct {
    QdEventPtr		pending, *pendtail;
    DeviceIntPtr	replayDev;	/* kludgy rock to put flag for */
    WindowPtr		replayWin;	/*   ComputeFreezes            */
    Bool		playingEvents;
    TimeStamp		time;
} syncEvents;

#define RootWindow(dev) dev->spriteInfo->sprite->spriteTrace[0]

static xEvent* swapEvent = NULL;
static int swapEventLen = 0;

/** 
 * True if device owns a cursor, false if device shares a cursor sprite with
 * another device.
 */
_X_EXPORT Bool
DevHasCursor(DeviceIntPtr pDev) 
{
    return (pDev != inputInfo.pointer && pDev->spriteInfo->spriteOwner);
}

/*
 * Return true if a device is a pointer, check is the same as used by XI to
 * fill the 'use' field.
 */
_X_EXPORT Bool
IsPointerDevice(DeviceIntPtr dev)
{
    return ((dev->valuator && dev->button) || dev == inputInfo.pointer);
}

/*
 * Return true if a device is a keyboard, check is the same as used by XI to
 * fill the 'use' field.
 */
_X_EXPORT Bool
IsKeyboardDevice(DeviceIntPtr dev)
{
    return ((dev->key && dev->kbdfeed) || dev == inputInfo.keyboard);
}

#ifdef XEVIE
_X_EXPORT WindowPtr xeviewin;
_X_EXPORT HotSpot xeviehot;
#endif

static void DoEnterLeaveEvents(
    DeviceIntPtr pDev, 
    WindowPtr fromWin,
    WindowPtr toWin,
    int mode
); 

static WindowPtr XYToWindow(
    DeviceIntPtr pDev,
    int x,
    int y
);

/**
 * Max event opcode.
 */
extern int lastEvent;

static Mask lastEventMask;

#ifdef XINPUT
extern int DeviceMotionNotify;
#endif

#define CantBeFiltered NoEventMask
static Mask filters[128] =
{
	NoSuchEvent,		       /* 0 */
	NoSuchEvent,		       /* 1 */
	KeyPressMask,		       /* KeyPress */
	KeyReleaseMask,		       /* KeyRelease */
	ButtonPressMask,	       /* ButtonPress */
	ButtonReleaseMask,	       /* ButtonRelease */
	PointerMotionMask,	       /* MotionNotify (initial state) */
	EnterWindowMask,	       /* EnterNotify */
	LeaveWindowMask,	       /* LeaveNotify */
	FocusChangeMask,	       /* FocusIn */
	FocusChangeMask,	       /* FocusOut */
	KeymapStateMask,	       /* KeymapNotify */
	ExposureMask,		       /* Expose */
	CantBeFiltered,		       /* GraphicsExpose */
	CantBeFiltered,		       /* NoExpose */
	VisibilityChangeMask,	       /* VisibilityNotify */
	SubstructureNotifyMask,	       /* CreateNotify */
	StructureAndSubMask,	       /* DestroyNotify */
	StructureAndSubMask,	       /* UnmapNotify */
	StructureAndSubMask,	       /* MapNotify */
	SubstructureRedirectMask,      /* MapRequest */
	StructureAndSubMask,	       /* ReparentNotify */
	StructureAndSubMask,	       /* ConfigureNotify */
	SubstructureRedirectMask,      /* ConfigureRequest */
	StructureAndSubMask,	       /* GravityNotify */
	ResizeRedirectMask,	       /* ResizeRequest */
	StructureAndSubMask,	       /* CirculateNotify */
	SubstructureRedirectMask,      /* CirculateRequest */
	PropertyChangeMask,	       /* PropertyNotify */
	CantBeFiltered,		       /* SelectionClear */
	CantBeFiltered,		       /* SelectionRequest */
	CantBeFiltered,		       /* SelectionNotify */
	ColormapChangeMask,	       /* ColormapNotify */
	CantBeFiltered,		       /* ClientMessage */
	CantBeFiltered		       /* MappingNotify */
};


/** 
 * same principle as filters, but one set of filters for each extension.
 * The extension is responsible for setting the filters by calling 
 * SetGenericFilter().
 */
static Mask* generic_filters[MAXEXTENSIONS];

static CARD8 criticalEvents[32] =
{
    0x7c				/* key and button events */
};

#ifdef PANORAMIX
static void PostNewCursor(DeviceIntPtr pDev);

#define SyntheticMotion(dev, x, y) \
    PostSyntheticMotion(dev, x, y, noPanoramiXExtension ? 0 : \
                              dev->spriteInfo->sprite->screen->myNum, \
                        syncEvents.playingEvents ? \
                          syncEvents.time.milliseconds : \
                          currentTime.milliseconds);

static Bool
XineramaSetCursorPosition(
    DeviceIntPtr pDev,
    int x, 
    int y, 
    Bool generateEvent
){
    ScreenPtr pScreen;
    BoxRec box;
    int i;
    SpritePtr pSprite = pDev->spriteInfo->sprite;

    /* x,y are in Screen 0 coordinates.  We need to decide what Screen
       to send the message too and what the coordinates relative to 
       that screen are. */

    pScreen = pSprite->screen;
    x += panoramiXdataPtr[0].x;
    y += panoramiXdataPtr[0].y;

    if(!POINT_IN_REGION(pScreen, &XineramaScreenRegions[pScreen->myNum],
								x, y, &box)) 
    {
	FOR_NSCREENS(i) 
	{
	    if(i == pScreen->myNum) 
		continue;
	    if(POINT_IN_REGION(pScreen, &XineramaScreenRegions[i], x, y, &box))
	    {
		pScreen = screenInfo.screens[i];
		break;
	    }
	}
    }

    pSprite->screen = pScreen;
    pSprite->hotPhys.x = x - panoramiXdataPtr[0].x;
    pSprite->hotPhys.y = y - panoramiXdataPtr[0].y;
    x -= panoramiXdataPtr[pScreen->myNum].x;
    y -= panoramiXdataPtr[pScreen->myNum].y;

    return (*pScreen->SetCursorPosition)(pDev, pScreen, x, y, generateEvent);
}


static void
XineramaConstrainCursor(DeviceIntPtr pDev)
{
    SpritePtr pSprite = pDev->spriteInfo->sprite;
    ScreenPtr pScreen;
    BoxRec newBox;

    pScreen = pSprite->screen;
    newBox = pSprite->physLimits;

    /* Translate the constraining box to the screen
       the sprite is actually on */
    newBox.x1 += panoramiXdataPtr[0].x - panoramiXdataPtr[pScreen->myNum].x;
    newBox.x2 += panoramiXdataPtr[0].x - panoramiXdataPtr[pScreen->myNum].x;
    newBox.y1 += panoramiXdataPtr[0].y - panoramiXdataPtr[pScreen->myNum].y;
    newBox.y2 += panoramiXdataPtr[0].y - panoramiXdataPtr[pScreen->myNum].y;

    (* pScreen->ConstrainCursor)(pDev, pScreen, &newBox);
}

static void
XineramaCheckPhysLimits(
    DeviceIntPtr pDev,
    CursorPtr cursor,
    Bool generateEvents
){
    HotSpot new;
    SpritePtr pSprite = pDev->spriteInfo->sprite;

    if (!cursor)
	return;
 
    new = pSprite->hotPhys;

    /* I don't care what the DDX has to say about it */
    pSprite->physLimits = pSprite->hotLimits;

    /* constrain the pointer to those limits */
    if (new.x < pSprite->physLimits.x1)
	new.x = pSprite->physLimits.x1;
    else
	if (new.x >= pSprite->physLimits.x2)
	    new.x = pSprite->physLimits.x2 - 1;
    if (new.y < pSprite->physLimits.y1)
	new.y = pSprite->physLimits.y1;
    else
	if (new.y >= pSprite->physLimits.y2)
	    new.y = pSprite->physLimits.y2 - 1;

    if (pSprite->hotShape)  /* more work if the shape is a mess */
	ConfineToShape(pDev, pSprite->hotShape, &new.x, &new.y);

    if((new.x != pSprite->hotPhys.x) || (new.y != pSprite->hotPhys.y))
    {
	XineramaSetCursorPosition (pDev, new.x, new.y, generateEvents);
	if (!generateEvents)
	    SyntheticMotion(pDev, new.x, new.y);
    }

    /* Tell DDX what the limits are */
    XineramaConstrainCursor(pDev);
}


static Bool
XineramaSetWindowPntrs(DeviceIntPtr pDev, WindowPtr pWin)
{
    SpritePtr pSprite = pDev->spriteInfo->sprite;

    if(pWin == WindowTable[0]) {
	    memcpy(pSprite->windows, WindowTable, 
				PanoramiXNumScreens*sizeof(WindowPtr));
    } else {
	PanoramiXRes *win;
	int i;

	win = (PanoramiXRes*)LookupIDByType(pWin->drawable.id, XRT_WINDOW);

	if(!win)
	    return FALSE;

	for(i = 0; i < PanoramiXNumScreens; i++) {
	   pSprite->windows[i] = LookupIDByType(win->info[i].id, RT_WINDOW);
	   if(!pSprite->windows[i])  /* window is being unmapped */
		return FALSE;
	}
    }
    return TRUE;
}

static void
XineramaCheckVirtualMotion(
   DeviceIntPtr pDev,
   QdEventPtr qe,
   WindowPtr pWin) 
{
    SpritePtr pSprite = pDev->spriteInfo->sprite;

    if (qe)
    {
	pSprite->hot.pScreen = qe->pScreen;  /* should always be Screen 0 */
	pSprite->hot.x = qe->event->u.keyButtonPointer.rootX;
	pSprite->hot.y = qe->event->u.keyButtonPointer.rootY;
	pWin = pDev->deviceGrab.grab ? pDev->deviceGrab.grab->confineTo :
					 NullWindow;
    }
    if (pWin)
    {
	int x, y, off_x, off_y, i;
	BoxRec lims;

	if(!XineramaSetWindowPntrs(pDev, pWin))
	    return;

	i = PanoramiXNumScreens - 1;
	
	REGION_COPY(pSprite->screen, &pSprite->Reg2, 
					&pSprite->windows[i]->borderSize); 
	off_x = panoramiXdataPtr[i].x;
	off_y = panoramiXdataPtr[i].y;

	while(i--) {
	    x = off_x - panoramiXdataPtr[i].x;
	    y = off_y - panoramiXdataPtr[i].y;

	    if(x || y)
		REGION_TRANSLATE(pSprite->screen, &pSprite->Reg2, x, y);
		
	    REGION_UNION(pSprite->screen, &pSprite->Reg2, &pSprite->Reg2, 
					&pSprite->windows[i]->borderSize);

	    off_x = panoramiXdataPtr[i].x;
	    off_y = panoramiXdataPtr[i].y;
	}

	lims = *REGION_EXTENTS(pSprite->screen, &pSprite->Reg2);

        if (pSprite->hot.x < lims.x1)
            pSprite->hot.x = lims.x1;
        else if (pSprite->hot.x >= lims.x2)
            pSprite->hot.x = lims.x2 - 1;
        if (pSprite->hot.y < lims.y1)
            pSprite->hot.y = lims.y1;
        else if (pSprite->hot.y >= lims.y2)
            pSprite->hot.y = lims.y2 - 1;

	if (REGION_NUM_RECTS(&pSprite->Reg2) > 1) 
	    ConfineToShape(pDev, &pSprite->Reg2, 
                    &pSprite->hot.x, &pSprite->hot.y);

	if (qe)
	{
	    qe->pScreen = pSprite->hot.pScreen;
	    qe->event->u.keyButtonPointer.rootX = pSprite->hot.x;
	    qe->event->u.keyButtonPointer.rootY = pSprite->hot.y;
	}
    }
#ifdef XEVIE
    xeviehot.x = pSprite->hot.x;
    xeviehot.y = pSprite->hot.y;
#endif
}


static Bool
XineramaCheckMotion(xEvent *xE, DeviceIntPtr pDev)
{
    WindowPtr prevSpriteWin;
    SpritePtr pSprite = pDev->spriteInfo->sprite;

    prevSpriteWin = pSprite->win;

    if (xE && !syncEvents.playingEvents)
    {
	/* Motion events entering DIX get translated to Screen 0
	   coordinates.  Replayed events have already been 
	   translated since they've entered DIX before */
	XE_KBPTR.rootX += panoramiXdataPtr[pSprite->screen->myNum].x -
			  panoramiXdataPtr[0].x;
	XE_KBPTR.rootY += panoramiXdataPtr[pSprite->screen->myNum].y -
			  panoramiXdataPtr[0].y;
	pSprite->hot.x = XE_KBPTR.rootX;
	pSprite->hot.y = XE_KBPTR.rootY;
	if (pSprite->hot.x < pSprite->physLimits.x1)
	    pSprite->hot.x = pSprite->physLimits.x1;
	else if (pSprite->hot.x >= pSprite->physLimits.x2)
	    pSprite->hot.x = pSprite->physLimits.x2 - 1;
	if (pSprite->hot.y < pSprite->physLimits.y1)
	    pSprite->hot.y = pSprite->physLimits.y1;
	else if (pSprite->hot.y >= pSprite->physLimits.y2)
	    pSprite->hot.y = pSprite->physLimits.y2 - 1;

	if (pSprite->hotShape) 
	    ConfineToShape(pDev, pSprite->hotShape, &pSprite->hot.x, &pSprite->hot.y);

	pSprite->hotPhys = pSprite->hot;
	if ((pSprite->hotPhys.x != XE_KBPTR.rootX) ||
	    (pSprite->hotPhys.y != XE_KBPTR.rootY))
	{
	    XineramaSetCursorPosition(
			pDev, pSprite->hotPhys.x, pSprite->hotPhys.y, FALSE);
	}
	XE_KBPTR.rootX = pSprite->hot.x;
	XE_KBPTR.rootY = pSprite->hot.y;
    }

#ifdef XEVIE
    xeviehot.x = pSprite->hot.x;
    xeviehot.y = pSprite->hot.y;
    xeviewin =
#endif
    pSprite->win = XYToWindow(pDev, pSprite->hot.x, pSprite->hot.y);

    if (pSprite->win != prevSpriteWin)
    {
	if (prevSpriteWin != NullWindow) {
	    if (!xE)
		UpdateCurrentTimeIf();
            DoEnterLeaveEvents(pDev, prevSpriteWin, pSprite->win,
                               NotifyNormal); 
        }
	PostNewCursor(pDev);
        return FALSE;
    }
    return TRUE;
}


static void
XineramaConfineCursorToWindow(DeviceIntPtr pDev, 
                              WindowPtr pWin, 
                              Bool generateEvents)
{
    SpritePtr pSprite = pDev->spriteInfo->sprite;

    if (syncEvents.playingEvents)
    {
	XineramaCheckVirtualMotion(pDev, (QdEventPtr)NULL, pWin);
	SyntheticMotion(pDev, pSprite->hot.x, pSprite->hot.y);
    }
    else
    {
	int x, y, off_x, off_y, i;

	if(!XineramaSetWindowPntrs(pDev, pWin))
	    return;

	i = PanoramiXNumScreens - 1;
	
	REGION_COPY(pSprite->screen, &pSprite->Reg1, 
					&pSprite->windows[i]->borderSize); 
	off_x = panoramiXdataPtr[i].x;
	off_y = panoramiXdataPtr[i].y;

	while(i--) {
	    x = off_x - panoramiXdataPtr[i].x;
	    y = off_y - panoramiXdataPtr[i].y;

	    if(x || y)
		REGION_TRANSLATE(pSprite->screen, &pSprite->Reg1, x, y);
		
	    REGION_UNION(pSprite->screen, &pSprite->Reg1, &pSprite->Reg1, 
					&pSprite->windows[i]->borderSize);

	    off_x = panoramiXdataPtr[i].x;
	    off_y = panoramiXdataPtr[i].y;
	}

	pSprite->hotLimits = *REGION_EXTENTS(pSprite->screen, &pSprite->Reg1);

	if(REGION_NUM_RECTS(&pSprite->Reg1) > 1)
	   pSprite->hotShape = &pSprite->Reg1;
	else
	   pSprite->hotShape = NullRegion;
	
	pSprite->confined = FALSE;
	pSprite->confineWin = (pWin == WindowTable[0]) ? NullWindow : pWin;

        XineramaCheckPhysLimits(pDev, pSprite->current,
                                generateEvents); 
    }
}


static void
XineramaChangeToCursor(DeviceIntPtr pDev, CursorPtr cursor)
{
    SpritePtr pSprite = pDev->spriteInfo->sprite;

    if (cursor != pSprite->current)
    {
	if ((pSprite->current->bits->xhot != cursor->bits->xhot) ||
		(pSprite->current->bits->yhot != cursor->bits->yhot))
	    XineramaCheckPhysLimits(pDev, cursor, FALSE);
    	(*pSprite->screen->DisplayCursor)(pDev, pSprite->screen, cursor);
	FreeCursor(pSprite->current, (Cursor)0);
	pSprite->current = cursor;
	pSprite->current->refcnt++;
    }
}

#else
#define SyntheticMotion(x, y) \
     PostSyntheticMotion(x, y, \
                         0, \
                         syncEvents.playingEvents ? \
                           syncEvents.time.milliseconds : \
                           currentTime.milliseconds);

#endif  /* PANORAMIX */

void
SetMaskForEvent(Mask mask, int event)
{
    if ((event < LASTEvent) || (event >= 128))
	FatalError("SetMaskForEvent: bogus event number");
    filters[event] = mask;
}

_X_EXPORT void
SetCriticalEvent(int event)
{
    if (event >= 128)
	FatalError("SetCriticalEvent: bogus event number");
    criticalEvents[event >> 3] |= 1 << (event & 7);
}

#ifdef SHAPE
void
ConfineToShape(DeviceIntPtr pDev, RegionPtr shape, int *px, int *py)
{
    BoxRec box;
    int x = *px, y = *py;
    int incx = 1, incy = 1;
    SpritePtr pSprite = pDev->spriteInfo->sprite;

    if (POINT_IN_REGION(pSprite->hot.pScreen, shape, x, y, &box))
	return;
    box = *REGION_EXTENTS(pSprite->hot.pScreen, shape);
    /* this is rather crude */
    do {
	x += incx;
	if (x >= box.x2)
	{
	    incx = -1;
	    x = *px - 1;
	}
	else if (x < box.x1)
	{
	    incx = 1;
	    x = *px;
	    y += incy;
	    if (y >= box.y2)
	    {
		incy = -1;
		y = *py - 1;
	    }
	    else if (y < box.y1)
		return; /* should never get here! */
	}
    } while (!POINT_IN_REGION(pSprite->hot.pScreen, shape, x, y, &box));
    *px = x;
    *py = y;
}
#endif

static void
CheckPhysLimits(
    DeviceIntPtr pDev, 
    CursorPtr cursor,
    Bool generateEvents,
    Bool confineToScreen,
    ScreenPtr pScreen)
{
    HotSpot new;
    SpritePtr pSprite = pDev->spriteInfo->sprite;

    if (!cursor)
	return;
    new = pSprite->hotPhys;
    if (pScreen)
	new.pScreen = pScreen;
    else
	pScreen = new.pScreen;
    (*pScreen->CursorLimits) (pDev, pScreen, cursor, &pSprite->hotLimits,
			      &pSprite->physLimits);
    pSprite->confined = confineToScreen;
    (* pScreen->ConstrainCursor)(pDev, pScreen, &pSprite->physLimits);
    if (new.x < pSprite->physLimits.x1)
	new.x = pSprite->physLimits.x1;
    else
	if (new.x >= pSprite->physLimits.x2)
	    new.x = pSprite->physLimits.x2 - 1;
    if (new.y < pSprite->physLimits.y1)
	new.y = pSprite->physLimits.y1;
    else
	if (new.y >= pSprite->physLimits.y2)
	    new.y = pSprite->physLimits.y2 - 1;
#ifdef SHAPE
    if (pSprite->hotShape)
	ConfineToShape(pDev, pSprite->hotShape, &new.x, &new.y); 
#endif
    if ((pScreen != pSprite->hotPhys.pScreen) ||
	(new.x != pSprite->hotPhys.x) || (new.y != pSprite->hotPhys.y))
    {
	if (pScreen != pSprite->hotPhys.pScreen)
	    pSprite->hotPhys = new;
        (*pScreen->SetCursorPosition) 
            (pDev, pScreen, new.x, new.y, generateEvents); 
        if (!generateEvents)
	    SyntheticMotion(pDev, new.x, new.y);
    }
}

static void
CheckVirtualMotion(
    DeviceIntPtr pDev,
    QdEventPtr qe,
    WindowPtr pWin)
{
    SpritePtr pSprite = pDev->spriteInfo->sprite;

#ifdef PANORAMIX
    if(!noPanoramiXExtension) {
	XineramaCheckVirtualMotion(pDev, qe, pWin);
	return;
    }
#endif
    if (qe)
    {
	pSprite->hot.pScreen = qe->pScreen;
	pSprite->hot.x = qe->event->u.keyButtonPointer.rootX;
	pSprite->hot.y = qe->event->u.keyButtonPointer.rootY;
	pWin = pDev->deviceGrab.grab ? pDev->deviceGrab.grab->confineTo : NullWindow;
    }
    if (pWin)
    {
	BoxRec lims;

	if (pSprite->hot.pScreen != pWin->drawable.pScreen)
	{
	    pSprite->hot.pScreen = pWin->drawable.pScreen;
	    pSprite->hot.x = pSprite->hot.y = 0;
	}
	lims = *REGION_EXTENTS(pWin->drawable.pScreen, &pWin->borderSize);
	if (pSprite->hot.x < lims.x1)
	    pSprite->hot.x = lims.x1;
	else if (pSprite->hot.x >= lims.x2)
	    pSprite->hot.x = lims.x2 - 1;
	if (pSprite->hot.y < lims.y1)
	    pSprite->hot.y = lims.y1;
	else if (pSprite->hot.y >= lims.y2)
	    pSprite->hot.y = lims.y2 - 1;
#ifdef SHAPE
	if (wBoundingShape(pWin))
	    ConfineToShape(pDev, &pWin->borderSize, 
                    &pSprite->hot.x, &pSprite->hot.y);
#endif
	if (qe)
	{
	    qe->pScreen = pSprite->hot.pScreen;
	    qe->event->u.keyButtonPointer.rootX = pSprite->hot.x;
	    qe->event->u.keyButtonPointer.rootY = pSprite->hot.y;
	}
    }
#ifdef XEVIE
    xeviehot.x = pSprite->hot.x;
    xeviehot.y = pSprite->hot.y;
#endif
    RootWindow(pDev) = WindowTable[pSprite->hot.pScreen->myNum];
}

static void
ConfineCursorToWindow(DeviceIntPtr pDev, WindowPtr pWin, Bool generateEvents, Bool confineToScreen)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    SpritePtr pSprite = pDev->spriteInfo->sprite;

#ifdef PANORAMIX
    if(!noPanoramiXExtension) {
	XineramaConfineCursorToWindow(pDev, pWin, generateEvents);
	return;
    }	
#endif

    if (syncEvents.playingEvents)
    {
	CheckVirtualMotion(pDev, (QdEventPtr)NULL, pWin);
	SyntheticMotion(pDev, pSprite->hot.x, pSprite->hot.y);
    }
    else
    {
	pSprite->hotLimits = *REGION_EXTENTS( pScreen, &pWin->borderSize);
#ifdef SHAPE
	pSprite->hotShape = wBoundingShape(pWin) ? &pWin->borderSize
					       : NullRegion;
#endif
        CheckPhysLimits(pDev, pSprite->current, generateEvents,
                        confineToScreen, pScreen);
    }
}

_X_EXPORT Bool
PointerConfinedToScreen(DeviceIntPtr pDev)
{
    return pDev->spriteInfo->sprite->confined;
}

/**
 * Update the sprite cursor to the given cursor.
 *
 * ChangeToCursor() will display the new cursor and free the old cursor (if
 * applicable). If the provided cursor is already the updated cursor, nothing
 * happens.
 */
static void
ChangeToCursor(DeviceIntPtr pDev, CursorPtr cursor)
{
    SpritePtr pSprite = pDev->spriteInfo->sprite;

#ifdef PANORAMIX
    if(!noPanoramiXExtension) {
	XineramaChangeToCursor(pDev, cursor);
	return;
    }
#endif

    if (cursor != pSprite->current)
    {
	if ((pSprite->current->bits->xhot != cursor->bits->xhot) ||
		(pSprite->current->bits->yhot != cursor->bits->yhot))
	    CheckPhysLimits(pDev, cursor, FALSE, pSprite->confined,
			    (ScreenPtr)NULL);
        (*pSprite->hotPhys.pScreen->DisplayCursor) (pDev,
                                                   pSprite->hotPhys.pScreen,
                                                   cursor);
	FreeCursor(pSprite->current, (Cursor)0);
	pSprite->current = cursor;
	pSprite->current->refcnt++;
    }
}

/**
 * @returns true if b is a descendent of a 
 */
Bool
IsParent(WindowPtr a, WindowPtr b)
{
    for (b = b->parent; b; b = b->parent)
	if (b == a) return TRUE;
    return FALSE;
}

/**
 * Update the cursor displayed on the screen.
 *
 * Called whenever a cursor may have changed shape or position.  
 */
static void
PostNewCursor(DeviceIntPtr pDev)
{
    WindowPtr win;
    GrabPtr grab = pDev->deviceGrab.grab;
    SpritePtr   pSprite = pDev->spriteInfo->sprite;
    CursorPtr   pCursor;

    if (syncEvents.playingEvents)
	return;
    if (grab)
    {
	if (grab->cursor)
	{
	    ChangeToCursor(pDev, grab->cursor);
	    return;
	}
	if (IsParent(grab->window, pSprite->win))
	    win = pSprite->win;
	else
	    win = grab->window;
    }
    else
	win = pSprite->win;
    for (; win; win = win->parent)
    {
	if (win->optional) 
        {
            pCursor = WindowGetDeviceCursor(win, pDev);
            if (!pCursor && win->optional->cursor != NullCursor)
                pCursor = win->optional->cursor;
            if (pCursor)
            {
                ChangeToCursor(pDev, pCursor);
                return;
            }
	}
    }
}


/**
 * @param dev device which you want to know its current root window
 * @return root window where dev's sprite is located
 */
_X_EXPORT WindowPtr
GetCurrentRootWindow(DeviceIntPtr dev)
{
    return RootWindow(dev);
}

/**
 * @return window underneath the cursor sprite.
 */
_X_EXPORT WindowPtr
GetSpriteWindow(DeviceIntPtr pDev)
{
    return pDev->spriteInfo->sprite->win;
}

/**
 * @return current sprite cursor.
 */
_X_EXPORT CursorPtr
GetSpriteCursor(DeviceIntPtr pDev)
{
    return pDev->spriteInfo->sprite->current;
}

/**
 * Set x/y current sprite position in screen coordinates.
 */
_X_EXPORT void
GetSpritePosition(DeviceIntPtr pDev, int *px, int *py)
{
    SpritePtr pSprite = pDev->spriteInfo->sprite;
    *px = pSprite->hotPhys.x;
    *py = pSprite->hotPhys.y;
}

#ifdef PANORAMIX
_X_EXPORT int
XineramaGetCursorScreen(DeviceIntPtr pDev)
{
    if(!noPanoramiXExtension) {
	return pDev->spriteInfo->sprite->screen->myNum;
    } else {
	return 0;
    }
}
#endif /* PANORAMIX */

#define TIMESLOP (5 * 60 * 1000) /* 5 minutes */

static void
MonthChangedOrBadTime(xEvent *xE)
{
    /* If the ddx/OS is careless about not processing timestamped events from
     * different sources in sorted order, then it's possible for time to go
     * backwards when it should not.  Here we ensure a decent time.
     */
    if ((currentTime.milliseconds - XE_KBPTR.time) > TIMESLOP)
	currentTime.months++;
    else
	XE_KBPTR.time = currentTime.milliseconds;
}

#define NoticeTime(xE) { \
    if ((xE)->u.keyButtonPointer.time < currentTime.milliseconds) \
	MonthChangedOrBadTime(xE); \
    currentTime.milliseconds = (xE)->u.keyButtonPointer.time; \
    lastDeviceEventTime = currentTime; }

void
NoticeEventTime(xEvent *xE)
{
    if (!syncEvents.playingEvents)
	NoticeTime(xE);
}

/**************************************************************************
 *            The following procedures deal with synchronous events       *
 **************************************************************************/

void
EnqueueEvent(xEvent *xE, DeviceIntPtr device, int count)
{
    QdEventPtr tail = *syncEvents.pendtail;
    QdEventPtr qe;
    xEvent		*qxE;
    SpritePtr pSprite = device->spriteInfo->sprite;

    NoticeTime(xE);

#ifdef XKB
    /* Fix for key repeating bug. */
    if (device->key != NULL && device->key->xkbInfo != NULL && 
	xE->u.u.type == KeyRelease)
	AccessXCancelRepeatKey(device->key->xkbInfo, xE->u.u.detail);
#endif

    if (DeviceEventCallback)
    {
	DeviceEventInfoRec eventinfo;
	/*  The RECORD spec says that the root window field of motion events
	 *  must be valid.  At this point, it hasn't been filled in yet, so
	 *  we do it here.  The long expression below is necessary to get
	 *  the current root window; the apparently reasonable alternative
	 *  GetCurrentRootWindow()->drawable.id doesn't give you the right
	 *  answer on the first motion event after a screen change because
	 *  the data that GetCurrentRootWindow relies on hasn't been
	 *  updated yet.
	 */
	if (xE->u.u.type == MotionNotify)
	    XE_KBPTR.root =
		WindowTable[pSprite->hotPhys.pScreen->myNum]->drawable.id;
	eventinfo.events = xE;
	eventinfo.count = count;
	CallCallbacks(&DeviceEventCallback, (pointer)&eventinfo);
    }
    if (xE->u.u.type == MotionNotify)
    {
#ifdef PANORAMIX
	if(!noPanoramiXExtension) {
	    XE_KBPTR.rootX += panoramiXdataPtr[pSprite->screen->myNum].x -
			      panoramiXdataPtr[0].x;
	    XE_KBPTR.rootY += panoramiXdataPtr[pSprite->screen->myNum].y -
			      panoramiXdataPtr[0].y;
	}
#endif
	pSprite->hotPhys.x = XE_KBPTR.rootX;
	pSprite->hotPhys.y = XE_KBPTR.rootY;
	/* do motion compression */
	if (tail &&
	    (tail->event->u.u.type == MotionNotify) &&
	    (tail->pScreen == pSprite->hotPhys.pScreen))
	{
	    tail->event->u.keyButtonPointer.rootX = pSprite->hotPhys.x;
	    tail->event->u.keyButtonPointer.rootY = pSprite->hotPhys.y;
	    tail->event->u.keyButtonPointer.time = XE_KBPTR.time;
	    tail->months = currentTime.months;
	    return;
	}
    }
    qe = (QdEventPtr)xalloc(sizeof(QdEventRec) + (count * sizeof(xEvent)));
    if (!qe)
	return;
    qe->next = (QdEventPtr)NULL;
    qe->device = device;
    qe->pScreen = pSprite->hotPhys.pScreen;
    qe->months = currentTime.months;
    qe->event = (xEvent *)(qe + 1);
    qe->evcount = count;
    for (qxE = qe->event; --count >= 0; qxE++, xE++)
	*qxE = *xE;
    if (tail)
	syncEvents.pendtail = &tail->next;
    *syncEvents.pendtail = qe;
}

static void
PlayReleasedEvents(void)
{
    QdEventPtr *prev, qe;
    DeviceIntPtr dev;
    DeviceIntPtr pDev;

    prev = &syncEvents.pending;
    while ( (qe = *prev) )
    {
	if (!qe->device->deviceGrab.sync.frozen)
	{
	    *prev = qe->next;
            pDev = qe->device;
	    if (*syncEvents.pendtail == *prev)
		syncEvents.pendtail = prev;
	    if (qe->event->u.u.type == MotionNotify)
		CheckVirtualMotion(pDev, qe, NullWindow);
	    syncEvents.time.months = qe->months;
	    syncEvents.time.milliseconds = qe->event->u.keyButtonPointer.time;
#ifdef PANORAMIX
	   /* Translate back to the sprite screen since processInputProc
	      will translate from sprite screen to screen 0 upon reentry
	      to the DIX layer */
	    if(!noPanoramiXExtension) {
		qe->event->u.keyButtonPointer.rootX += 
			panoramiXdataPtr[0].x - 
			panoramiXdataPtr[pDev->spriteInfo->sprite->screen->myNum].x;
		qe->event->u.keyButtonPointer.rootY += 
			panoramiXdataPtr[0].y - 
			panoramiXdataPtr[pDev->spriteInfo->sprite->screen->myNum].y;
	    }
#endif
	    (*qe->device->public.processInputProc)(qe->event, qe->device,
						   qe->evcount);
	    xfree(qe);
	    for (dev = inputInfo.devices; dev && dev->deviceGrab.sync.frozen; dev = dev->next)
		;
	    if (!dev)
		break;
	    /* Playing the event may have unfrozen another device. */
	    /* So to play it safe, restart at the head of the queue */
	    prev = &syncEvents.pending;
	}
	else
	    prev = &qe->next;
    } 
}

/**
 * Freeze or thaw the given devices. The device's processing proc is
 * switched to either the real processing proc (in case of thawing) or an
 * enqueuing processing proc (usually EnqueueEvent()).
 *
 * @param dev The device to freeze/thaw
 * @param frozen True to freeze or false to thaw.
 */
static void
FreezeThaw(DeviceIntPtr dev, Bool frozen)
{
    dev->deviceGrab.sync.frozen = frozen;
    if (frozen)
	dev->public.processInputProc = dev->public.enqueueInputProc;
    else
	dev->public.processInputProc = dev->public.realInputProc;
}

/**
 * Unfreeze devices and replay all events to the respective clients.
 *
 * ComputeFreezes takes the first event in the device's frozen event queue. It
 * runs up the sprite tree (spriteTrace) and searches for the window to replay
 * the events from. If it is found, it checks for passive grabs one down from
 * the window or delivers the events.
 */
void
ComputeFreezes(void)
{
    DeviceIntPtr replayDev = syncEvents.replayDev;
    int i;
    WindowPtr w;
    xEvent *xE;
    int count;
    GrabPtr grab;
    DeviceIntPtr dev;

    for (dev = inputInfo.devices; dev; dev = dev->next)
	FreezeThaw(dev, dev->deviceGrab.sync.other || 
                (dev->deviceGrab.sync.state >= FROZEN));
    if (syncEvents.playingEvents || (!replayDev && !syncEvents.pending))
	return;
    syncEvents.playingEvents = TRUE;
    if (replayDev)
    {
	xE = replayDev->deviceGrab.sync.event;
	count = replayDev->deviceGrab.sync.evcount;
	syncEvents.replayDev = (DeviceIntPtr)NULL;

        w = XYToWindow(replayDev, XE_KBPTR.rootX, XE_KBPTR.rootY);
	for (i = 0; i < replayDev->spriteInfo->sprite->spriteTraceGood; i++)
	{
	    if (syncEvents.replayWin ==
		replayDev->spriteInfo->sprite->spriteTrace[i])
	    {
		if (!CheckDeviceGrabs(replayDev, xE, i+1, count)) {
		    if (replayDev->focus)
			DeliverFocusedEvent(replayDev, xE, w, count);
		    else
			DeliverDeviceEvents(w, xE, NullGrab, NullWindow,
					        replayDev, count);
		}
		goto playmore;
	    }
	}
	/* must not still be in the same stack */
	if (replayDev->focus)
	    DeliverFocusedEvent(replayDev, xE, w, count);
	else
	    DeliverDeviceEvents(w, xE, NullGrab, NullWindow, replayDev, count);
    }
playmore:
    for (dev = inputInfo.devices; dev; dev = dev->next)
    {
	if (!dev->deviceGrab.sync.frozen)
	{
	    PlayReleasedEvents();
	    break;
	}
    }
    syncEvents.playingEvents = FALSE;
    for (dev = inputInfo.devices; dev; dev = dev->next)
    {
        if (DevHasCursor(dev))
        {
            /* the following may have been skipped during replay, 
              so do it now */
            if ((grab = dev->deviceGrab.grab) && grab->confineTo)
            {
                if (grab->confineTo->drawable.pScreen !=
                        dev->spriteInfo->sprite->hotPhys.pScreen) 
                    dev->spriteInfo->sprite->hotPhys.x =
                        dev->spriteInfo->sprite->hotPhys.y = 0;
                ConfineCursorToWindow(dev, grab->confineTo, TRUE, TRUE);
            }
            else
                ConfineCursorToWindow(dev,
                        WindowTable[dev->spriteInfo->sprite->hotPhys.pScreen->myNum],
                        TRUE, FALSE);
            PostNewCursor(dev);
        }
    }
}

#ifdef RANDR
void
ScreenRestructured (ScreenPtr pScreen)
{
    GrabPtr grab;
    DeviceIntPtr pDev;

    for (pDev = inputInfo.devices; pDev; pDev = pDev->next)
    {

        /* GrabDevice doesn't have a confineTo field, so we don't need to
         * worry about it. */
        if ((grab = pDev->deviceGrab.grab) && grab->confineTo)
        {
            if (grab->confineTo->drawable.pScreen 
                    != pDev->spriteInfo->sprite->hotPhys.pScreen)
                pDev->spriteInfo->sprite->hotPhys.x = pDev->spriteInfo->sprite->hotPhys.y = 0;
            ConfineCursorToWindow(pDev, grab->confineTo, TRUE, TRUE);
        }
        else
            ConfineCursorToWindow(pDev, 
                    WindowTable[pDev->spriteInfo->sprite->hotPhys.pScreen->myNum],
                    TRUE, FALSE);
    }
}
#endif

void
CheckGrabForSyncs(DeviceIntPtr thisDev, Bool thisMode, Bool otherMode)
{
    GrabPtr grab = thisDev->deviceGrab.grab;
    DeviceIntPtr dev;

    if (thisMode == GrabModeSync)
	thisDev->deviceGrab.sync.state = FROZEN_NO_EVENT;
    else
    {	/* free both if same client owns both */
	thisDev->deviceGrab.sync.state = THAWED;
	if (thisDev->deviceGrab.sync.other &&
	    (CLIENT_BITS(thisDev->deviceGrab.sync.other->resource) ==
	     CLIENT_BITS(grab->resource)))
	    thisDev->deviceGrab.sync.other = NullGrab;
    }
    for (dev = inputInfo.devices; dev; dev = dev->next)
    {
	if (dev != thisDev)
	{
	    if (otherMode == GrabModeSync)
		dev->deviceGrab.sync.other = grab;
	    else
	    {	/* free both if same client owns both */
		if (dev->deviceGrab.sync.other &&
		    (CLIENT_BITS(dev->deviceGrab.sync.other->resource) ==
		     CLIENT_BITS(grab->resource)))
		    dev->deviceGrab.sync.other = NullGrab;
	    }
	}
    }
    ComputeFreezes();
}

/**
 * Activate a pointer grab on the given device. A pointer grab will cause all
 * core pointer events of this device to be delivered to the grabbing client only. 
 * No other device will send core events to the grab client while the grab is
 * on, but core events will be sent to other clients.
 * Can cause the cursor to change if a grab cursor is set.
 * 
 * Note that parameter autoGrab may be (True & ImplicitGrabMask) if the grab
 * is an implicit grab caused by a ButtonPress event.
 * 
 * @param mouse The device to grab.
 * @param grab The grab structure, needs to be setup.
 * @param autoGrab True if the grab was caused by a button down event and not
 * explicitely by a client. 
 */
void
ActivatePointerGrab(DeviceIntPtr mouse, GrabPtr grab, 
                    TimeStamp time, Bool autoGrab)
{
    GrabInfoPtr grabinfo = &mouse->deviceGrab;
    WindowPtr oldWin = (grabinfo->grab) ? 
                        grabinfo->grab->window
                        : mouse->spriteInfo->sprite->win;

    if (grab->confineTo)
    {
	if (grab->confineTo->drawable.pScreen 
                != mouse->spriteInfo->sprite->hotPhys.pScreen)
	    mouse->spriteInfo->sprite->hotPhys.x = 
                mouse->spriteInfo->sprite->hotPhys.y = 0;
	ConfineCursorToWindow(mouse, grab->confineTo, FALSE, TRUE);
    }
    DoEnterLeaveEvents(mouse, oldWin, grab->window, NotifyGrab);
    mouse->valuator->motionHintWindow = NullWindow;
    if (syncEvents.playingEvents)
        grabinfo->grabTime = syncEvents.time;
    else
	grabinfo->grabTime = time;
    if (grab->cursor)
	grab->cursor->refcnt++;
    grabinfo->activeGrab = *grab;
    grabinfo->grab = &grabinfo->activeGrab;
    grabinfo->fromPassiveGrab = autoGrab & ~ImplicitGrabMask;
    grabinfo->implicitGrab = autoGrab & ImplicitGrabMask;
    PostNewCursor(mouse);
    CheckGrabForSyncs(mouse,(Bool)grab->pointerMode, (Bool)grab->keyboardMode);
}

/**
 * Delete grab on given device, update the sprite.
 *
 * Extension devices are set up for ActivateKeyboardGrab().
 */
void
DeactivatePointerGrab(DeviceIntPtr mouse)
{
    GrabPtr grab = mouse->deviceGrab.grab;
    DeviceIntPtr dev;

    mouse->valuator->motionHintWindow = NullWindow;
    mouse->deviceGrab.grab = NullGrab;
    mouse->deviceGrab.sync.state = NOT_GRABBED;
    mouse->deviceGrab.fromPassiveGrab = FALSE;
    for (dev = inputInfo.devices; dev; dev = dev->next)
    {
	if (dev->deviceGrab.sync.other == grab)
	    dev->deviceGrab.sync.other = NullGrab;
    }
    DoEnterLeaveEvents(mouse, grab->window, 
                       mouse->spriteInfo->sprite->win, NotifyUngrab);
    if (grab->confineTo)
	ConfineCursorToWindow(mouse, RootWindow(mouse), FALSE, FALSE);
    PostNewCursor(mouse);
    if (grab->cursor)
	FreeCursor(grab->cursor, (Cursor)0);
    ComputeFreezes();
}

/**
 * Activate a keyboard grab on the given device. 
 *
 * Extension devices have ActivateKeyboardGrab() set as their grabbing proc.
 */
void
ActivateKeyboardGrab(DeviceIntPtr keybd, GrabPtr grab, TimeStamp time, Bool passive)
{
    GrabInfoPtr grabinfo = &keybd->deviceGrab;
    WindowPtr oldWin;

    if (grabinfo->grab)
	oldWin = grabinfo->grab->window;
    else if (keybd->focus)
	oldWin = keybd->focus->win;
    else
	oldWin = keybd->spriteInfo->sprite->win;
    if (oldWin == FollowKeyboardWin)
	oldWin = inputInfo.keyboard->focus->win;
    if (keybd->valuator)
	keybd->valuator->motionHintWindow = NullWindow;
    DoFocusEvents(keybd, oldWin, grab->window, NotifyGrab);
    if (syncEvents.playingEvents)
	grabinfo->grabTime = syncEvents.time;
    else
	grabinfo->grabTime = time;
    grabinfo->activeGrab = *grab;
    grabinfo->grab = &grabinfo->activeGrab;
    grabinfo->fromPassiveGrab = passive;
    CheckGrabForSyncs(keybd, (Bool)grab->keyboardMode, (Bool)grab->pointerMode);
}

/**
 * Delete keyboard grab for the given device. 
 */
void
DeactivateKeyboardGrab(DeviceIntPtr keybd)
{
    GrabPtr grab = keybd->deviceGrab.grab;
    DeviceIntPtr dev;
    WindowPtr focusWin = keybd->focus ? keybd->focus->win
                                           : keybd->spriteInfo->sprite->win;

    if (!grab)
        grab = keybd->deviceGrab.grab;

    if (focusWin == FollowKeyboardWin)
	focusWin = inputInfo.keyboard->focus->win;
    if (keybd->valuator)
	keybd->valuator->motionHintWindow = NullWindow;
    keybd->deviceGrab.grab = NullGrab;
    keybd->deviceGrab.sync.state = NOT_GRABBED;
    keybd->deviceGrab.fromPassiveGrab = FALSE;
    for (dev = inputInfo.devices; dev; dev = dev->next)
    {
	if (dev->deviceGrab.sync.other == grab)
	    dev->deviceGrab.sync.other = NullGrab;
    }
    DoFocusEvents(keybd, grab->window, focusWin, NotifyUngrab);
    ComputeFreezes();
}

void
AllowSome(ClientPtr client, 
          TimeStamp time, 
          DeviceIntPtr thisDev, 
          int newState, 
          Bool core)
{
    Bool thisGrabbed, otherGrabbed, othersFrozen, thisSynced;
    TimeStamp grabTime;
    DeviceIntPtr dev;
    GrabInfoPtr devgrabinfo, 
                grabinfo = &thisDev->deviceGrab;

    thisGrabbed = grabinfo->grab && SameClient(grabinfo->grab, client);
    thisSynced = FALSE;
    otherGrabbed = FALSE;
    othersFrozen = TRUE;
    grabTime = grabinfo->grabTime;
    for (dev = inputInfo.devices; dev; dev = dev->next)
    {
        devgrabinfo = &dev->deviceGrab;

	if (dev == thisDev)
	    continue;
	if (devgrabinfo->grab && SameClient(devgrabinfo->grab, client))
	{
	    if (!(thisGrabbed || otherGrabbed) ||
		(CompareTimeStamps(devgrabinfo->grabTime, grabTime) == LATER))
		grabTime = devgrabinfo->grabTime;
	    otherGrabbed = TRUE;
	    if (grabinfo->sync.other == devgrabinfo->grab)
		thisSynced = TRUE;
	    if (devgrabinfo->sync.state < FROZEN)
		othersFrozen = FALSE;
	}
	else if (!devgrabinfo->sync.other || !SameClient(devgrabinfo->sync.other, client))
	    othersFrozen = FALSE;
    }
    if (!((thisGrabbed && grabinfo->sync.state >= FROZEN) || thisSynced))
	return;
    if ((CompareTimeStamps(time, currentTime) == LATER) ||
	(CompareTimeStamps(time, grabTime) == EARLIER))
	return;
    switch (newState)
    {
	case THAWED:	 	       /* Async */
	    if (thisGrabbed)
		grabinfo->sync.state = THAWED;
	    if (thisSynced)
		grabinfo->sync.other = NullGrab;
	    ComputeFreezes();
	    break;
	case FREEZE_NEXT_EVENT:		/* Sync */
	    if (thisGrabbed)
	    {
		grabinfo->sync.state = FREEZE_NEXT_EVENT;
		if (thisSynced)
		    grabinfo->sync.other = NullGrab;
		ComputeFreezes();
	    }
	    break;
	case THAWED_BOTH:		/* AsyncBoth */
	    if (othersFrozen)
	    {
		for (dev = inputInfo.devices; dev; dev = dev->next)
		{
                    devgrabinfo = &dev->deviceGrab;
		    if (devgrabinfo->grab 
                            && SameClient(devgrabinfo->grab, client))
			devgrabinfo->sync.state = THAWED;
		    if (devgrabinfo->sync.other && 
                            SameClient(devgrabinfo->sync.other, client))
			devgrabinfo->sync.other = NullGrab;
		}
		ComputeFreezes();
	    }
	    break;
	case FREEZE_BOTH_NEXT_EVENT:	/* SyncBoth */
	    if (othersFrozen)
	    {
		for (dev = inputInfo.devices; dev; dev = dev->next)
		{
                    devgrabinfo = &dev->deviceGrab;
		    if (devgrabinfo->grab 
                            && SameClient(devgrabinfo->grab, client))
			devgrabinfo->sync.state = FREEZE_BOTH_NEXT_EVENT;
		    if (devgrabinfo->sync.other 
                            && SameClient(devgrabinfo->sync.other, client))
			devgrabinfo->sync.other = NullGrab;
		}
		ComputeFreezes();
	    }
	    break;
	case NOT_GRABBED:		/* Replay */
	    if (thisGrabbed && grabinfo->sync.state == FROZEN_WITH_EVENT)
	    {
		if (thisSynced)
		    grabinfo->sync.other = NullGrab;
		syncEvents.replayDev = thisDev;
		syncEvents.replayWin = grabinfo->grab->window;
		(*grabinfo->DeactivateGrab)(thisDev);
		syncEvents.replayDev = (DeviceIntPtr)NULL;
	    }
	    break;
	case THAW_OTHERS:		/* AsyncOthers */
	    if (othersFrozen)
	    {
		for (dev = inputInfo.devices; dev; dev = dev->next)
		{
		    if (dev == thisDev)
			continue;
                    devgrabinfo = (core) ? &dev->deviceGrab : &dev->deviceGrab;
		    if (devgrabinfo->grab 
                            && SameClient(devgrabinfo->grab, client))
			devgrabinfo->sync.state = THAWED;
		    if (devgrabinfo->sync.other 
                            && SameClient(devgrabinfo->sync.other, client))
			devgrabinfo->sync.other = NullGrab;
		}
		ComputeFreezes();
	    }
	    break;
    }
}

/**
 * Server-side protocol handling for AllowEvents request.
 *
 * Release some events from a frozen device. 
 */
int
ProcAllowEvents(ClientPtr client)
{
    TimeStamp		time;
    DeviceIntPtr	mouse = PickPointer(client);
    DeviceIntPtr	keybd = PickKeyboard(client);
    REQUEST(xAllowEventsReq);

    REQUEST_SIZE_MATCH(xAllowEventsReq);
    time = ClientTimeToServerTime(stuff->time);
    switch (stuff->mode)
    {
	case ReplayPointer:
	    AllowSome(client, time, mouse, NOT_GRABBED, True);
	    break;
	case SyncPointer: 
	    AllowSome(client, time, mouse, FREEZE_NEXT_EVENT, True);
	    break;
	case AsyncPointer: 
	    AllowSome(client, time, mouse, THAWED, True);
	    break;
	case ReplayKeyboard: 
	    AllowSome(client, time, keybd, NOT_GRABBED, True);
	    break;
	case SyncKeyboard: 
	    AllowSome(client, time, keybd, FREEZE_NEXT_EVENT, True);
	    break;
	case AsyncKeyboard: 
	    AllowSome(client, time, keybd, THAWED, True);
	    break;
	case SyncBoth:
	    AllowSome(client, time, keybd, FREEZE_BOTH_NEXT_EVENT, True);
	    break;
	case AsyncBoth:
	    AllowSome(client, time, keybd, THAWED_BOTH, True);
	    break;
	default: 
	    client->errorValue = stuff->mode;
	    return BadValue;
    }
    return Success;
}

/**
 * Deactivate grabs from any device that has been grabbed by the client.
 */
void
ReleaseActiveGrabs(ClientPtr client)
{
    DeviceIntPtr dev;
    Bool    done;

    /* XXX CloseDownClient should remove passive grabs before
     * releasing active grabs.
     */
    do {
    	done = TRUE;
    	for (dev = inputInfo.devices; dev; dev = dev->next)
    	{
	    if (dev->deviceGrab.grab && SameClient(dev->deviceGrab.grab, client))
	    {
	    	(*dev->deviceGrab.DeactivateGrab)(dev);
	    	done = FALSE;
	    }

	    if (dev->deviceGrab.grab && SameClient(dev->deviceGrab.grab, client))
	    {
	    	(*dev->deviceGrab.DeactivateGrab)(dev);
	    	done = FALSE;
	    }
    	}
    } while (!done);
}

/**************************************************************************
 *            The following procedures deal with delivering events        *
 **************************************************************************/

/**
 * Deliver the given events to the given client.
 *
 * More than one event may be delivered at a time. This is the case with
 * DeviceMotionNotifies which may be followed by DeviceValuator events.
 *
 * TryClientEvents() is the last station before actually writing the events to
 * the socket. Anything that is not filtered here, will get delivered to the
 * client. 
 * An event is only delivered if 
 *   - mask and filter match up.
 *   - no other client has a grab on the device that caused the event.
 * 
 *
 * @param client The target client to deliver to.
 * @param pEvents The events to be delivered.
 * @param count Number of elements in pEvents.
 * @param mask Event mask as set by the window.
 * @param filter Mask based on event type.
 * @param grab Possible grab on the device that caused the event. 
 *
 * @return 1 if event was delivered, 0 if not or -1 if grab was not set by the
 * client.
 */
_X_EXPORT int
TryClientEvents (ClientPtr client, xEvent *pEvents, int count, Mask mask, 
                 Mask filter, GrabPtr grab)
{
    int i;
    int type;

#ifdef DEBUG_EVENTS
    ErrorF("Event([%d, %d], mask=0x%x), client=%d",
	pEvents->u.u.type, pEvents->u.u.detail, mask, client->index);
#endif
    if ((client) && (client != serverClient) && (!client->clientGone) &&
	((filter == CantBeFiltered) || (mask & filter)))
    {
	if (grab && !SameClient(grab, client))
	    return -1; /* don't send, but notify caller */
	type = pEvents->u.u.type;
	if (type == MotionNotify)
	{
	    if (mask & PointerMotionHintMask)
	    {
		if (WID(inputInfo.pointer->valuator->motionHintWindow) ==
		    pEvents->u.keyButtonPointer.event)
		{
#ifdef DEBUG_EVENTS
		    ErrorF("\n");
	    ErrorF("motionHintWindow == keyButtonPointer.event\n");
#endif
		    return 1; /* don't send, but pretend we did */
		}
		pEvents->u.u.detail = NotifyHint;
	    }
	    else
	    {
		pEvents->u.u.detail = NotifyNormal;
	    }
	}
#ifdef XINPUT
	else
	{
	    if ((type == DeviceMotionNotify) &&
		MaybeSendDeviceMotionNotifyHint
			((deviceKeyButtonPointer*)pEvents, mask) != 0)
		return 1;
	}
#endif
	type &= 0177;
	if (type != KeymapNotify)
	{
	    /* all extension events must have a sequence number */
	    for (i = 0; i < count; i++)
		pEvents[i].u.u.sequenceNumber = client->sequence;
	}

	if (BitIsOn(criticalEvents, type))
	{
#ifdef SMART_SCHEDULE
	    if (client->smart_priority < SMART_MAX_PRIORITY)
		client->smart_priority++;
#endif
	    SetCriticalOutputPending();
	}

	WriteEventsToClient(client, count, pEvents);
#ifdef DEBUG_EVENTS
	ErrorF(  " delivered\n");
#endif
	return 1;
    }
    else
    {
#ifdef DEBUG_EVENTS
	ErrorF("\n");
#endif
	return 0;
    }
}

/**
 * Deliver events to a window. At this point, we do not yet know if the event
 * actually needs to be delivered. May activate a grab if the event is a
 * button press.
 * 
 * Core events are always delivered to the window owner. If the filter is
 * something other than CantBeFiltered, the event is also delivered to other
 * clients with the matching mask on the window.
 *
 * More than one event may be delivered at a time. This is the case with
 * DeviceMotionNotifies which may be followed by DeviceValuator events.
 * 
 * @param pWin The window that would get the event.
 * @param pEvents The events to be delivered.
 * @param count Number of elements in pEvents.
 * @param filter Mask based on event type.
 * @param grab Possible grab on the device that caused the event. 
 * @param mskidx Mask index, depending on device that caused event.
 *
 * @return Number of events delivered to various clients.
 */
int
DeliverEventsToWindow(DeviceIntPtr pDev, WindowPtr pWin, xEvent
        *pEvents, int count, Mask filter, GrabPtr grab, int mskidx)
{
    int deliveries = 0, nondeliveries = 0;
    int attempt;
    InputClients *other;
    ClientPtr client = NullClient;
    Mask deliveryMask = 0; /* If a grab occurs due to a button press, then
		              this mask is the mask of the grab. */
    int type = pEvents->u.u.type;
    
    /* if a  is denied, we return 0. This could cause the caller to
     * traverse the parent. May be bad! (whot) */
    if (!ACDeviceAllowed(pWin, pDev))
        return 0;

    /* CantBeFiltered means only window owner gets the event */
    if ((filter == CantBeFiltered) || 
            (!(type & EXTENSION_EVENT_BASE) && type != GenericEvent))
    {
	/* if nobody ever wants to see this event, skip some work */
	if (filter != CantBeFiltered &&
	    !((wOtherEventMasks(pWin)|pWin->eventMask) & filter))
	    return 0;
        
        if (!(type & EXTENSION_EVENT_BASE) && 
            IsInterferingGrab(wClient(pWin), pDev, pEvents))
                return 0;

	if ( (attempt = TryClientEvents(wClient(pWin), pEvents, count,
				      pWin->eventMask, filter, grab)) )
	{
	    if (attempt > 0)
	    {
		deliveries++;
		client = wClient(pWin);
		deliveryMask = pWin->eventMask;
	    } else
		nondeliveries--;
	}
    }
    if (filter != CantBeFiltered)
    {
        /* Handle generic events */
        if (type == GenericEvent)
        {
            GenericMaskPtr pClient;
            /* We don't do more than one GenericEvent at a time. */
            if (count > 1)
            {
                ErrorF("Do not send more than one GenericEvent at a time!\n");
                return 0;
            }

            /* if we get here, filter should be set to the GE specific mask.
               check if any client wants it */
            if (!GEMaskIsSet(pWin, GEEXT(pEvents), filter))
                return 0;

            /* run through all clients, deliver event */
            for (pClient = GECLIENT(pWin); pClient; pClient = pClient->next)
            {
                if (pClient->eventMask[GEEXTIDX(pEvents)] & filter)
                {
                    if (TryClientEvents(pClient->client, pEvents, count, 
                            pClient->eventMask[GEEXTIDX(pEvents)], filter, grab) > 0)
                    {
                        deliveries++;
                    } else 
                        nondeliveries--;
                }
            }
        }
        else {
            /* Traditional event */
            if (type & EXTENSION_EVENT_BASE)
            {
                OtherInputMasks *inputMasks;

                inputMasks = wOtherInputMasks(pWin);
                if (!inputMasks ||
                        !(inputMasks->inputEvents[mskidx] & filter))
                    return 0;
                other = inputMasks->inputClients;
            }
            else
                other = (InputClients *)wOtherClients(pWin);
            for (; other; other = other->next)
            {
                /* core event? check for grab interference */
                if (!(type & EXTENSION_EVENT_BASE) &&
                        IsInterferingGrab(rClient(other), pDev, pEvents))
                    continue;

                if ( (attempt = TryClientEvents(rClient(other), pEvents, count,
                                other->mask[mskidx], filter, grab)) )
                {
                    if (attempt > 0)
                    {
                        deliveries++;
                        client = rClient(other);
                        deliveryMask = other->mask[mskidx];
                    } else
                        nondeliveries--;
                }
            }
        }
    }
    if ((type == ButtonPress) && deliveries && (!grab))
    {
	GrabRec tempGrab;
        OtherInputMasks *inputMasks;

        tempGrab.next = NULL;
	tempGrab.device = pDev;
	tempGrab.resource = client->clientAsMask;
	tempGrab.window = pWin;
	tempGrab.ownerEvents = (deliveryMask & OwnerGrabButtonMask) ? TRUE : FALSE;
	tempGrab.eventMask = deliveryMask;
	tempGrab.keyboardMode = GrabModeAsync;
	tempGrab.pointerMode = GrabModeAsync;
	tempGrab.confineTo = NullWindow;
	tempGrab.cursor = NullCursor;
        tempGrab.coreGrab = True;

        /* get the XI device mask */
        inputMasks = wOtherInputMasks(pWin);
        tempGrab.deviceMask = (inputMasks) ? inputMasks->inputEvents[pDev->id]: 0;

        /* get the XGE event mask. 
         * FIXME: needs to be freed somewhere too.
         */
        tempGrab.genericMasks = NULL;
        if (pWin->optional && pWin->optional->geMasks)
        {
            GenericClientMasksPtr gemasks = pWin->optional->geMasks;
            GenericMaskPtr geclient = gemasks->geClients;
            while(geclient && geclient->client != client)
                geclient = geclient->next;
            if (geclient)
            {
                tempGrab.genericMasks = xcalloc(1, sizeof(GenericMaskRec));
                *tempGrab.genericMasks = *geclient;
                tempGrab.genericMasks->next = NULL;
            }
        }
	(*inputInfo.pointer->deviceGrab.ActivateGrab)(pDev, &tempGrab,
					   currentTime, 
                                           TRUE | ImplicitGrabMask);
    }
    else if ((type == MotionNotify) && deliveries)
	pDev->valuator->motionHintWindow = pWin;
#ifdef XINPUT
    else
    {
	if (((type == DeviceMotionNotify)
#ifdef XKB
	     || (type == DeviceButtonPress)
#endif
	    ) && deliveries)
	    CheckDeviceGrabAndHintWindow (pWin, type,
					  (deviceKeyButtonPointer*) pEvents,
					  grab, client, deliveryMask);
    }
#endif
    if (deliveries)
	return deliveries;
    return nondeliveries;
}

/* If the event goes to dontClient, don't send it and return 0.  if
   send works,  return 1 or if send didn't work, return 2.
   Only works for core events.
*/

#ifdef PANORAMIX
static int 
XineramaTryClientEventsResult(
    ClientPtr client,
    GrabPtr grab,
    Mask mask, 
    Mask filter
){
    if ((client) && (client != serverClient) && (!client->clientGone) &&
        ((filter == CantBeFiltered) || (mask & filter)))
    {
        if (grab && !SameClient(grab, client)) return -1;
	else return 1;
    }
    return 0;
}
#endif

/**
 * Try to deliver events to the interested parties.
 *
 * @param pWin The window that would get the event.
 * @param pEvents The events to be delivered.
 * @param count Number of elements in pEvents.
 * @param filter Mask based on event type.
 * @param dontClient Don't deliver to the dontClient.
 */
int
MaybeDeliverEventsToClient(WindowPtr pWin, xEvent *pEvents, 
                           int count, Mask filter, ClientPtr dontClient)
{
    OtherClients *other;


    if (pWin->eventMask & filter)
    {
        if (wClient(pWin) == dontClient)
	    return 0;
#ifdef PANORAMIX
	if(!noPanoramiXExtension && pWin->drawable.pScreen->myNum) 
	    return XineramaTryClientEventsResult(
			wClient(pWin), NullGrab, pWin->eventMask, filter);
#endif
	return TryClientEvents(wClient(pWin), pEvents, count,
			       pWin->eventMask, filter, NullGrab);
    }
    for (other = wOtherClients(pWin); other; other = other->next)
    {
	if (other->mask & filter)
	{
            if (SameClient(other, dontClient))
		return 0;
#ifdef PANORAMIX
	    if(!noPanoramiXExtension && pWin->drawable.pScreen->myNum) 
	      return XineramaTryClientEventsResult(
			rClient(other), NullGrab, other->mask, filter);
#endif
	    return TryClientEvents(rClient(other), pEvents, count,
				   other->mask, filter, NullGrab);
	}
    }
    return 2;
}

/**
 * Adjust event fields to comply with the window properties.
 *
 * @param xE Event to be modified in place
 * @param pWin The window to get the information from.
 * @param child Child window setting for event (if applicable)
 * @param calcChild If True, calculate the child window.
 */
static void
FixUpEventFromWindow(
    DeviceIntPtr pDev,
    xEvent *xE,
    WindowPtr pWin,
    Window child,
    Bool calcChild)
{
    SpritePtr pSprite = pDev->spriteInfo->sprite;

    if (xE->u.u.type == GenericEvent) /* just a safety barrier */
        return;

    if (calcChild)
    {
        WindowPtr w= pSprite->spriteTrace[pSprite->spriteTraceGood-1];
	/* If the search ends up past the root should the child field be 
	 	set to none or should the value in the argument be passed 
		through. It probably doesn't matter since everyone calls 
		this function with child == None anyway. */

        while (w) 
        {
            /* If the source window is same as event window, child should be
		none.  Don't bother going all all the way back to the root. */

 	    if (w == pWin)
	    { 
   		child = None;
 		break;
	    }
	    
	    if (w->parent == pWin)
	    {
		child = w->drawable.id;
		break;
            }
 	    w = w->parent;
        } 	    
    }
    XE_KBPTR.root = RootWindow(pDev)->drawable.id;
    XE_KBPTR.event = pWin->drawable.id;
    if (pSprite->hot.pScreen == pWin->drawable.pScreen)
    {
	XE_KBPTR.sameScreen = xTrue;
	XE_KBPTR.child = child;
	XE_KBPTR.eventX =
	XE_KBPTR.rootX - pWin->drawable.x;
	XE_KBPTR.eventY =
	XE_KBPTR.rootY - pWin->drawable.y;
    }
    else
    {
	XE_KBPTR.sameScreen = xFalse;
	XE_KBPTR.child = None;
	XE_KBPTR.eventX = 0;
	XE_KBPTR.eventY = 0;
    }
}

/**
 * Deliver events caused by input devices. Called for all core input events
 * and XI events. No filtering of events happens before DeliverDeviceEvents(),
 * it will be called for any event that comes out of the event queue.
 *
 * @param pWin Window to deliver event to.
 * @param xE Events to deliver.
 * @param grab Possible grab on a device.
 * @param stopAt Don't recurse up to the root window.
 * @param dev The device that is responsible for the event.
 * @param count number of events in xE.
 *
 */
int
DeliverDeviceEvents(WindowPtr pWin, xEvent *xE, GrabPtr grab, 
                    WindowPtr stopAt, DeviceIntPtr dev, int count)
{
    Window child = None;
    int type = xE->u.u.type;
    Mask filter = filters[type];
    int deliveries = 0;

    if (type & EXTENSION_EVENT_BASE)
    {
	OtherInputMasks *inputMasks;
	int mskidx = dev->id;

	inputMasks = wOtherInputMasks(pWin);
	if (inputMasks && !(filter & inputMasks->deliverableEvents[mskidx]))
	    return 0;
	while (pWin)
	{
	    if (inputMasks && (inputMasks->inputEvents[mskidx] & filter))
	    {
		FixUpEventFromWindow(dev, xE, pWin, child, FALSE);
		deliveries = DeliverEventsToWindow(dev, pWin, xE, count, filter,
						   grab, mskidx);
		if (deliveries > 0)
		    return deliveries;
	    }
	    if ((deliveries < 0) ||
		(pWin == stopAt) ||
		(inputMasks &&
		 (filter & inputMasks->dontPropagateMask[mskidx])))
		return 0;
	    child = pWin->drawable.id;
	    pWin = pWin->parent;
	    if (pWin)
		inputMasks = wOtherInputMasks(pWin);
	}
    }
    else
    {
        /* handle generic events */
        if (type == GenericEvent)
        {
            xGenericEvent* ge = (xGenericEvent*)xE;

            if (count > 1)
            {
                ErrorF("Do not send more than one GenericEvent at a time!\n");
                return 0;
            }
            filter = generic_filters[GEEXTIDX(xE)][ge->evtype];

            while(pWin)
            {
                if (GEMaskIsSet(pWin, GEEXT(xE), filter))
                {
                    if (GEExtensions[GEEXTIDX(xE)].evfill)
                        GEExtensions[GEEXTIDX(xE)].evfill(ge, dev, pWin, grab);
                    deliveries = DeliverEventsToWindow(dev, pWin, xE, count, 
                                                        filter, grab, 0);
                    if (deliveries > 0)
                        return deliveries;
                }

                pWin = pWin->parent;
            }
        } 
        else
        {
            /* core protocol events */
            if (!(filter & pWin->deliverableEvents))
                return 0;
            while (pWin)
            {
                if ((wOtherEventMasks(pWin)|pWin->eventMask) & filter)
                {
                    FixUpEventFromWindow(dev, xE, pWin, child, FALSE);
                    deliveries = DeliverEventsToWindow(dev, pWin, xE, count, filter,
                            grab, 0);
                    if (deliveries > 0)
                        return deliveries;
                }
                if ((deliveries < 0) ||
                        (pWin == stopAt) ||
                        (filter & wDontPropagateMask(pWin)))
                    return 0;
                child = pWin->drawable.id;
                pWin = pWin->parent;
            }
	}
    }
    return 0;
}

/**
 * Deliver event to a window and it's immediate parent. Used for most window
 * events (CreateNotify, ConfigureNotify, etc.). Not useful for events that
 * propagate up the tree or extension events 
 *
 * In case of a ReparentNotify event, the event will be delivered to the
 * otherParent as well.
 *
 * @param pWin Window to deliver events to.
 * @param xE Events to deliver.
 * @param count number of events in xE.
 * @param otherParent Used for ReparentNotify events.
 */
_X_EXPORT int
DeliverEvents(WindowPtr pWin, xEvent *xE, int count, 
              WindowPtr otherParent)
{
    Mask filter;
    int     deliveries;

#ifdef PANORAMIX
    if(!noPanoramiXExtension && pWin->drawable.pScreen->myNum)
	return count;
#endif

    if (!count)
	return 0;
    filter = filters[xE->u.u.type];
    if ((filter & SubstructureNotifyMask) && (xE->u.u.type != CreateNotify))
	xE->u.destroyNotify.event = pWin->drawable.id;
    if (filter != StructureAndSubMask)
	return DeliverEventsToWindow(inputInfo.pointer, pWin, xE, count, filter, NullGrab, 0);
    deliveries = DeliverEventsToWindow(inputInfo.pointer, pWin, xE, count, StructureNotifyMask,
				       NullGrab, 0);
    if (pWin->parent)
    {
	xE->u.destroyNotify.event = pWin->parent->drawable.id;
	deliveries += DeliverEventsToWindow(inputInfo.pointer, pWin->parent, xE, count,
					    SubstructureNotifyMask, NullGrab,
					    0);
	if (xE->u.u.type == ReparentNotify)
	{
	    xE->u.destroyNotify.event = otherParent->drawable.id;
            deliveries += DeliverEventsToWindow(inputInfo.pointer,
                    otherParent, xE, count, SubstructureNotifyMask,
						NullGrab, 0);
	}
    }
    return deliveries;
}


static Bool 
PointInBorderSize(WindowPtr pWin, int x, int y)
{
    BoxRec box;
    SpritePtr pSprite = inputInfo.pointer->spriteInfo->sprite;

    if(POINT_IN_REGION(pWin->drawable.pScreen, &pWin->borderSize, x, y, &box))
	return TRUE;

#ifdef PANORAMIX
    if(!noPanoramiXExtension && 
            XineramaSetWindowPntrs(inputInfo.pointer, pWin)) {
	int i;

	for(i = 1; i < PanoramiXNumScreens; i++) {
	   if(POINT_IN_REGION(pSprite->screen, 
			&pSprite->windows[i]->borderSize, 
			x + panoramiXdataPtr[0].x - panoramiXdataPtr[i].x, 
			y + panoramiXdataPtr[0].y - panoramiXdataPtr[i].y, 
			&box))
		return TRUE;
	}
    }
#endif
    return FALSE;
}

/**
 * Traversed from the root window to the window at the position x/y. While
 * traversing, it sets up the traversal history in the spriteTrace array.
 * After completing, the spriteTrace history is set in the following way:
 *   spriteTrace[0] ... root window
 *   spriteTrace[1] ... top level window that encloses x/y
 *       ...
 *   spriteTrace[spriteTraceGood - 1] ... window at x/y
 *
 * @returns the window at the given coordinates.
 */
static WindowPtr 
XYToWindow(DeviceIntPtr pDev, int x, int y)
{
    WindowPtr  pWin;
    BoxRec		box;
    SpritePtr pSprite;

    pSprite = pDev->spriteInfo->sprite;
    pSprite->spriteTraceGood = 1;	/* root window still there */
    pWin = RootWindow(pDev)->firstChild;
    while (pWin)
    {
	if ((pWin->mapped) &&
	    (x >= pWin->drawable.x - wBorderWidth (pWin)) &&
	    (x < pWin->drawable.x + (int)pWin->drawable.width +
	     wBorderWidth(pWin)) &&
	    (y >= pWin->drawable.y - wBorderWidth (pWin)) &&
	    (y < pWin->drawable.y + (int)pWin->drawable.height +
	     wBorderWidth (pWin))
#ifdef SHAPE
	    /* When a window is shaped, a further check
	     * is made to see if the point is inside
	     * borderSize
	     */
	    && (!wBoundingShape(pWin) || PointInBorderSize(pWin, x, y))
	    && (!wInputShape(pWin) ||
		POINT_IN_REGION(pWin->drawable.pScreen,
				wInputShape(pWin),
				x - pWin->drawable.x,
				y - pWin->drawable.y, &box))
#endif
	    )
	{
	    if (pSprite->spriteTraceGood >= pSprite->spriteTraceSize)
	    {
		pSprite->spriteTraceSize += 10;
		Must_have_memory = TRUE; /* XXX */
		pSprite->spriteTrace = (WindowPtr *)xrealloc(
		                    pSprite->spriteTrace,
		                    pSprite->spriteTraceSize*sizeof(WindowPtr));
		Must_have_memory = FALSE; /* XXX */
	    }
	    pSprite->spriteTrace[pSprite->spriteTraceGood++] = pWin;
	    pWin = pWin->firstChild;
	}
	else
	    pWin = pWin->nextSib;
    }
    return pSprite->spriteTrace[pSprite->spriteTraceGood-1];
}

/**
 * Update the sprite coordinates based on the event. Update the cursor
 * position, then update the event with the new coordinates that may have been
 * changed. If the window underneath the sprite has changed, change to new
 * cursor and send enter/leave events.
 *
 * CheckMotion() will not do anything and return FALSE if the event is not a
 * pointer event.
 *
 * @return TRUE if the sprite has moved or FALSE otherwise. 
 */
Bool
CheckMotion(xEvent *xE, DeviceIntPtr pDev)
{
    INT16     *rootX, *rootY;
    WindowPtr prevSpriteWin;
    SpritePtr pSprite = pDev->spriteInfo->sprite;
        
    prevSpriteWin = pSprite->win;

#ifdef PANORAMIX
    if(!noPanoramiXExtension)
	return XineramaCheckMotion(xE, pDev);
#endif

    if (xE && !syncEvents.playingEvents)
    {
        /* GetPointerEvents() guarantees that pointer events have the correct
           rootX/Y set already. */
        switch(xE->u.u.type)
        {
            case ButtonPress:
            case ButtonRelease:
            case MotionNotify:
                rootX = &XE_KBPTR.rootX;
                rootY = &XE_KBPTR.rootY;
                break;
            default:
                if (xE->u.u.type == DeviceButtonPress ||
                        xE->u.u.type == DeviceButtonRelease ||
                        xE->u.u.type == DeviceMotionNotify)
                {
                    rootX = &((deviceKeyButtonPointer*)xE)->root_x;
                    rootY = &((deviceKeyButtonPointer*)xE)->root_y;
                    break;
                }
                /* all other events return FALSE */
                return FALSE;
        }

        if (pSprite->hot.pScreen != pSprite->hotPhys.pScreen)
        {
            pSprite->hot.pScreen = pSprite->hotPhys.pScreen;
            RootWindow(pDev) = WindowTable[pSprite->hot.pScreen->myNum];
        }
        pSprite->hot.x = *rootX;
        pSprite->hot.y = *rootY;
        if (pSprite->hot.x < pSprite->physLimits.x1)
            pSprite->hot.x = pSprite->physLimits.x1;
        else if (pSprite->hot.x >= pSprite->physLimits.x2)
            pSprite->hot.x = pSprite->physLimits.x2 - 1;
        if (pSprite->hot.y < pSprite->physLimits.y1)
            pSprite->hot.y = pSprite->physLimits.y1;
        else if (pSprite->hot.y >= pSprite->physLimits.y2)
            pSprite->hot.y = pSprite->physLimits.y2 - 1;
#ifdef SHAPE
	if (pSprite->hotShape)
	    ConfineToShape(pDev, pSprite->hotShape, &pSprite->hot.x, &pSprite->hot.y);
#endif
#ifdef XEVIE
        xeviehot.x = pSprite->hot.x;
        xeviehot.y = pSprite->hot.y;
#endif
	pSprite->hotPhys = pSprite->hot;

	if ((pSprite->hotPhys.x != *rootX) ||
	    (pSprite->hotPhys.y != *rootY))
	{
	    (*pSprite->hotPhys.pScreen->SetCursorPosition)(
                pDev, pSprite->hotPhys.pScreen,
		pSprite->hotPhys.x, pSprite->hotPhys.y, FALSE);
	}

	*rootX = pSprite->hot.x;
	*rootY = pSprite->hot.y;
    }

#ifdef XEVIE
    xeviewin =
#endif
    pSprite->win = XYToWindow(pDev, pSprite->hot.x, pSprite->hot.y);
#ifdef notyet
    if (!(pSprite->win->deliverableEvents &
	  Motion_Filter(pDev->button))
	!syncEvents.playingEvents)
    {
	/* XXX Do PointerNonInterestBox here */
    }
#endif
    if (pSprite->win != prevSpriteWin)
    {
	if (prevSpriteWin != NullWindow) {
	    if (!xE)
		UpdateCurrentTimeIf();
            DoEnterLeaveEvents(pDev, prevSpriteWin, pSprite->win,
                               NotifyNormal); 
        }
	PostNewCursor(pDev);
        return FALSE;
    }
    return TRUE;
}

/**
 * Windows have restructured, we need to update the sprite position and the
 * sprite's cursor.
 */
_X_EXPORT void
WindowsRestructured(void)
{
    DeviceIntPtr pDev = inputInfo.devices;
    while(pDev)
    {
        if (DevHasCursor(pDev))
            CheckMotion((xEvent *)NULL, pDev);
        pDev = pDev->next;
    }
}

#ifdef PANORAMIX
/* This was added to support reconfiguration under Xdmx.  The problem is
 * that if the 0th screen (i.e., WindowTable[0]) is moved to an origin
 * other than 0,0, the information in the private sprite structure must
 * be updated accordingly, or XYToWindow (and other routines) will not
 * compute correctly. */
void ReinitializeRootWindow(WindowPtr win, int xoff, int yoff)
{
    GrabPtr   grab;
    DeviceIntPtr pDev;
    SpritePtr pSprite;

    if (noPanoramiXExtension) return;

    pDev = inputInfo.devices;
    while(pDev)
    {
        if (DevHasCursor(pDev))
        {
            pSprite = pDev->spriteInfo->sprite;
            pSprite->hot.x        -= xoff;
            pSprite->hot.y        -= yoff;

            pSprite->hotPhys.x    -= xoff;
            pSprite->hotPhys.y    -= yoff;

            pSprite->hotLimits.x1 -= xoff; 
            pSprite->hotLimits.y1 -= yoff;
            pSprite->hotLimits.x2 -= xoff;
            pSprite->hotLimits.y2 -= yoff;

            if (REGION_NOTEMPTY(pSprite->screen, &pSprite->Reg1))
                REGION_TRANSLATE(pSprite->screen, &pSprite->Reg1,    xoff, yoff);
            if (REGION_NOTEMPTY(pSprite->screen, &pSprite->Reg2))
                REGION_TRANSLATE(pSprite->screen, &pSprite->Reg2,    xoff, yoff);

            /* FIXME: if we call ConfineCursorToWindow, must we do anything else? */
            if ((grab = pDev->deviceGrab.grab) && grab->confineTo) {
                if (grab->confineTo->drawable.pScreen 
                        != pSprite->hotPhys.pScreen)
                    pSprite->hotPhys.x = pSprite->hotPhys.y = 0;
                ConfineCursorToWindow(pDev, grab->confineTo, TRUE, TRUE);
            } else
                ConfineCursorToWindow(
                        pDev,
                        WindowTable[pSprite->hotPhys.pScreen->myNum],
                        TRUE, FALSE);

        }
        pDev = pDev->next;
    }
}
#endif

/**
 * Called from main() with the root window on the first screen. Used to do a
 * lot more when MPX wasn't around yet. Things change.
 */
void
DefineInitialRootWindow(WindowPtr win)
{
#ifdef XEVIE
    xeviewin = win;
#endif

}

/**
 * Initialize a sprite for the given device and set it to some sane values. If
 * the device already has a sprite alloc'd, don't realloc but just reset to
 * default values.
 * If a window is supplied, the sprite will be initialized with the window's
 * cursor and positioned in the center of the window's screen. The root window
 * is a good choice to pass in here.
 *
 * It's a good idea to call it only for pointer devices, unless you have a
 * really talented keyboard.
 *
 * @param pDev The device to initialize.
 * @param pWin The window where to generate the sprite in.
 * 
 */
void 
InitializeSprite(DeviceIntPtr pDev, WindowPtr pWin)
{
    SpritePtr pSprite;
    ScreenPtr pScreen; 

    if (!pDev->spriteInfo->sprite)
    {
        DeviceIntPtr it;

        pDev->spriteInfo->sprite = (SpritePtr)xcalloc(1, sizeof(SpriteRec));
        if (!pDev->spriteInfo->sprite)
            FatalError("InitializeSprite: failed to allocate sprite struct");

        /* We may have paired another device with this device before our
         * device had a actual sprite. We need to check for this and reset the
         * sprite field for all paired devices.
         *
         * The VCK is always paired with the VCP before the VCP has a sprite.
         */
        for (it = inputInfo.devices; it; it = it->next)
        {
            if (it->spriteInfo->paired == pDev)
                it->spriteInfo->sprite = pDev->spriteInfo->sprite;
        }
        if (inputInfo.keyboard->spriteInfo->paired == pDev)
            inputInfo.keyboard->spriteInfo->sprite = pDev->spriteInfo->sprite;
    }

    pSprite = pDev->spriteInfo->sprite;
    pDev->spriteInfo->spriteOwner = TRUE;

    pScreen = (pWin) ? pWin->drawable.pScreen : (ScreenPtr)NULL;
    pSprite->hot.pScreen = pScreen;
    pSprite->hotPhys.pScreen = pScreen;
    if (pScreen)
    {
        pSprite->hotPhys.x = pScreen->width / 2;
        pSprite->hotPhys.y = pScreen->height / 2;
        pSprite->hotLimits.x2 = pScreen->width;
        pSprite->hotLimits.y2 = pScreen->height;
    }

    pSprite->hot = pSprite->hotPhys;
    pSprite->win = pWin;

    if (pWin)
    {
        pSprite->current = wCursor(pWin);
        pSprite->current->refcnt++;
 	pSprite->spriteTrace = (WindowPtr *)xcalloc(1, 32*sizeof(WindowPtr));
	if (!pSprite->spriteTrace)
	    FatalError("Failed to allocate spriteTrace");
	pSprite->spriteTraceSize = 32;

	RootWindow(pDev) = pWin;
	pSprite->spriteTraceGood = 1;

	pSprite->pEnqueueScreen = pScreen;
	pSprite->pDequeueScreen = pSprite->pEnqueueScreen;

    } else {
        pSprite->current = NullCursor;
	pSprite->spriteTrace = NULL;
	pSprite->spriteTraceSize = 0;
	pSprite->spriteTraceGood = 0;
	pSprite->pEnqueueScreen = screenInfo.screens[0];
	pSprite->pDequeueScreen = pSprite->pEnqueueScreen;
    }

    if (pScreen)
    {
        (*pScreen->CursorLimits) ( pDev, pScreen, pSprite->current,
                                   &pSprite->hotLimits, &pSprite->physLimits);
        pSprite->confined = FALSE;

        (*pScreen->ConstrainCursor) (pDev, pScreen,
                                     &pSprite->physLimits);
        (*pScreen->SetCursorPosition) (pDev, pScreen, pSprite->hot.x,
                                       pSprite->hot.y,
                                       FALSE); 
        (*pScreen->DisplayCursor) (pDev, pScreen, pSprite->current);
    }
#ifdef PANORAMIX
    if(!noPanoramiXExtension) {
        pSprite->hotLimits.x1 = -panoramiXdataPtr[0].x;
        pSprite->hotLimits.y1 = -panoramiXdataPtr[0].y;
        pSprite->hotLimits.x2 = PanoramiXPixWidth  - panoramiXdataPtr[0].x;
        pSprite->hotLimits.y2 = PanoramiXPixHeight - panoramiXdataPtr[0].y;
        pSprite->physLimits = pSprite->hotLimits;
        pSprite->confineWin = NullWindow;
#ifdef SHAPE
        pSprite->hotShape = NullRegion;
#endif
        pSprite->screen = pScreen;
        /* gotta UNINIT these someplace */
        REGION_NULL(pScreen, &pSprite->Reg1);
        REGION_NULL(pScreen, &pSprite->Reg2);
    }
#endif
}

/*
 * This does not take any shortcuts, and even ignores its argument, since
 * it does not happen very often, and one has to walk up the tree since
 * this might be a newly instantiated cursor for an intermediate window
 * between the one the pointer is in and the one that the last cursor was
 * instantiated from.
 */
void
WindowHasNewCursor(WindowPtr pWin)
{
    DeviceIntPtr pDev;

    for(pDev = inputInfo.devices; pDev; pDev = pDev->next)
        if (DevHasCursor(pDev))
            PostNewCursor(pDev);
}

_X_EXPORT void
NewCurrentScreen(DeviceIntPtr pDev, ScreenPtr newScreen, int x, int y)
{
    SpritePtr pSprite = pDev->spriteInfo->sprite;

    pSprite->hotPhys.x = x;
    pSprite->hotPhys.y = y;
#ifdef PANORAMIX
    if(!noPanoramiXExtension) {
	pSprite->hotPhys.x += panoramiXdataPtr[newScreen->myNum].x - 
			    panoramiXdataPtr[0].x;
	pSprite->hotPhys.y += panoramiXdataPtr[newScreen->myNum].y - 
			    panoramiXdataPtr[0].y;
	if (newScreen != pSprite->screen) {
	    pSprite->screen = newScreen;
	    /* Make sure we tell the DDX to update its copy of the screen */
	    if(pSprite->confineWin)
		XineramaConfineCursorToWindow(pDev, 
                        pSprite->confineWin, TRUE);
	    else
		XineramaConfineCursorToWindow(pDev, WindowTable[0], TRUE);
	    /* if the pointer wasn't confined, the DDX won't get 
	       told of the pointer warp so we reposition it here */
	    if(!syncEvents.playingEvents)
		(*pSprite->screen->SetCursorPosition)(
                                                      pDev,
                                                      pSprite->screen,
		    pSprite->hotPhys.x + panoramiXdataPtr[0].x - 
			panoramiXdataPtr[pSprite->screen->myNum].x,
		    pSprite->hotPhys.y + panoramiXdataPtr[0].y - 
			panoramiXdataPtr[pSprite->screen->myNum].y, FALSE);
	}
    } else 
#endif
    if (newScreen != pSprite->hotPhys.pScreen)
	ConfineCursorToWindow(pDev, WindowTable[newScreen->myNum], 
                TRUE, FALSE);
}

#ifdef PANORAMIX

static Bool
XineramaPointInWindowIsVisible(
    WindowPtr pWin,
    int x,
    int y
)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    BoxRec box;
    int i, xoff, yoff;

    if (!pWin->realized) return FALSE;

    if (POINT_IN_REGION(pScreen, &pWin->borderClip, x, y, &box))
        return TRUE;
    
    if(!XineramaSetWindowPntrs(inputInfo.pointer, pWin)) return FALSE;

    xoff = x + panoramiXdataPtr[0].x;  
    yoff = y + panoramiXdataPtr[0].y;  

    for(i = 1; i < PanoramiXNumScreens; i++) {
	pWin = inputInfo.pointer->spriteInfo->sprite->windows[i];
	pScreen = pWin->drawable.pScreen;
	x = xoff - panoramiXdataPtr[i].x;
	y = yoff - panoramiXdataPtr[i].y;

	if(POINT_IN_REGION(pScreen, &pWin->borderClip, x, y, &box)
	   && (!wInputShape(pWin) ||
	       POINT_IN_REGION(pWin->drawable.pScreen,
			       wInputShape(pWin),
			       x - pWin->drawable.x, 
			       y - pWin->drawable.y, &box)))
            return TRUE;

    }

    return FALSE;
}

static int
XineramaWarpPointer(ClientPtr client)
{
    WindowPtr	dest = NULL;
    int		x, y, rc;
    SpritePtr   pSprite = PickPointer(client)->spriteInfo->sprite;

    REQUEST(xWarpPointerReq);


    if (stuff->dstWid != None) {
	rc = dixLookupWindow(&dest, stuff->dstWid, client, DixReadAccess);
	if (rc != Success)
	    return rc;
    }
    x = pSprite->hotPhys.x;
    y = pSprite->hotPhys.y;

    if (stuff->srcWid != None)
    {
	int     winX, winY;
 	XID 	winID = stuff->srcWid;
        WindowPtr source;
	
	rc = dixLookupWindow(&source, winID, client, DixReadAccess);
	if (rc != Success)
	    return rc;

	winX = source->drawable.x;
	winY = source->drawable.y;
	if(source == WindowTable[0]) {
	    winX -= panoramiXdataPtr[0].x;
	    winY -= panoramiXdataPtr[0].y;
	}
	if (x < winX + stuff->srcX ||
	    y < winY + stuff->srcY ||
	    (stuff->srcWidth != 0 &&
	     winX + stuff->srcX + (int)stuff->srcWidth < x) ||
	    (stuff->srcHeight != 0 &&
	     winY + stuff->srcY + (int)stuff->srcHeight < y) ||
	    !XineramaPointInWindowIsVisible(source, x, y))
	    return Success;
    }
    if (dest) {
	x = dest->drawable.x;
	y = dest->drawable.y;
	if(dest == WindowTable[0]) {
	    x -= panoramiXdataPtr[0].x;
	    y -= panoramiXdataPtr[0].y;
	}
    } 

    x += stuff->dstX;
    y += stuff->dstY;

    if (x < pSprite->physLimits.x1)
	x = pSprite->physLimits.x1;
    else if (x >= pSprite->physLimits.x2)
	x = pSprite->physLimits.x2 - 1;
    if (y < pSprite->physLimits.y1)
	y = pSprite->physLimits.y1;
    else if (y >= pSprite->physLimits.y2)
	y = pSprite->physLimits.y2 - 1;
    if (pSprite->hotShape)
	ConfineToShape(PickPointer(client), pSprite->hotShape, &x, &y);

    XineramaSetCursorPosition(PickPointer(client), x, y, TRUE);

    return Success;
}

#endif


/**
 * Server-side protocol handling for WarpPointer request.
 * Warps the cursor position to the coordinates given in the request.
 */
int
ProcWarpPointer(ClientPtr client)
{
    WindowPtr	dest = NULL;
    int		x, y, rc;
    ScreenPtr	newScreen;
    SpritePtr   pSprite = PickPointer(client)->spriteInfo->sprite;

    REQUEST(xWarpPointerReq);

    REQUEST_SIZE_MATCH(xWarpPointerReq);

#ifdef PANORAMIX
    if(!noPanoramiXExtension)
	return XineramaWarpPointer(client);
#endif

    if (stuff->dstWid != None) {
	rc = dixLookupWindow(&dest, stuff->dstWid, client, DixReadAccess);
	if (rc != Success)
	    return rc;
    }
    x = pSprite->hotPhys.x;
    y = pSprite->hotPhys.y;

    if (stuff->srcWid != None)
    {
	int     winX, winY;
 	XID 	winID = stuff->srcWid;
        WindowPtr source;
	
	rc = dixLookupWindow(&source, winID, client, DixReadAccess);
	if (rc != Success)
	    return rc;

	winX = source->drawable.x;
	winY = source->drawable.y;
	if (source->drawable.pScreen != pSprite->hotPhys.pScreen ||
	    x < winX + stuff->srcX ||
	    y < winY + stuff->srcY ||
	    (stuff->srcWidth != 0 &&
	     winX + stuff->srcX + (int)stuff->srcWidth < x) ||
	    (stuff->srcHeight != 0 &&
	     winY + stuff->srcY + (int)stuff->srcHeight < y) ||
	    !PointInWindowIsVisible(source, x, y))
	    return Success;
    }
    if (dest) 
    {
	x = dest->drawable.x;
	y = dest->drawable.y;
	newScreen = dest->drawable.pScreen;
    } else 
	newScreen = pSprite->hotPhys.pScreen;

    x += stuff->dstX;
    y += stuff->dstY;

    if (x < 0)
	x = 0;
    else if (x >= newScreen->width)
	x = newScreen->width - 1;
    if (y < 0)
	y = 0;
    else if (y >= newScreen->height)
	y = newScreen->height - 1;

    if (newScreen == pSprite->hotPhys.pScreen)
    {
	if (x < pSprite->physLimits.x1)
	    x = pSprite->physLimits.x1;
	else if (x >= pSprite->physLimits.x2)
	    x = pSprite->physLimits.x2 - 1;
	if (y < pSprite->physLimits.y1)
	    y = pSprite->physLimits.y1;
	else if (y >= pSprite->physLimits.y2)
	    y = pSprite->physLimits.y2 - 1;
#if defined(SHAPE)
	if (pSprite->hotShape)
	    ConfineToShape(PickPointer(client), pSprite->hotShape, &x, &y);
#endif
        (*newScreen->SetCursorPosition)(PickPointer(client), newScreen, x, y,
                                        TRUE); 
    }
    else if (!PointerConfinedToScreen(PickPointer(client)))
    {
	NewCurrentScreen(PickPointer(client), newScreen, x, y);
    }
    return Success;
}

static Bool 
BorderSizeNotEmpty(DeviceIntPtr pDev, WindowPtr pWin)
{
     if(REGION_NOTEMPTY(pDev->spriteInfo->sprite->hotPhys.pScreen, &pWin->borderSize))
	return TRUE;

#ifdef PANORAMIX
     if(!noPanoramiXExtension && XineramaSetWindowPntrs(pDev, pWin)) {
	int i;

	for(i = 1; i < PanoramiXNumScreens; i++) {
	    if(REGION_NOTEMPTY(pDev->spriteInfo->sprite->screen, 
                        &pDev->spriteInfo->sprite->windows[i]->borderSize))
		return TRUE;
	}
     }
#endif
     return FALSE;
}

/** 
 * "CheckPassiveGrabsOnWindow" checks to see if the event passed in causes a
 * passive grab set on the window to be activated. 
 * If a passive grab is activated, the event will be delivered to the client.
 * 
 * @param pWin The window that may be subject to a passive grab.
 * @param device Device that caused the event.
 * @param xE List of events (multiple ones for DeviceMotionNotify)
 * @count number of elements in xE.
 */

static Bool
CheckPassiveGrabsOnWindow(
    WindowPtr pWin,
    DeviceIntPtr device,
    xEvent *xE,
    int count)
{
    GrabPtr grab = wPassiveGrabs(pWin);
    GrabRec tempGrab;
    GrabInfoPtr grabinfo;
    xEvent *dxE;

    if (!grab)
	return FALSE;
    tempGrab.window = pWin;
    tempGrab.device = device;
    tempGrab.type = xE->u.u.type;
    tempGrab.detail.exact = xE->u.u.detail;
    tempGrab.detail.pMask = NULL;
    tempGrab.modifiersDetail.pMask = NULL;
    tempGrab.next = NULL;
    for (; grab; grab = grab->next)
    {
#ifdef XKB
	DeviceIntPtr	gdev;
	XkbSrvInfoPtr	xkbi;

	gdev= grab->modifierDevice;
	xkbi= gdev->key->xkbInfo;
#endif
	tempGrab.modifierDevice = grab->modifierDevice;
	if ((device == grab->modifierDevice) &&
	    ((xE->u.u.type == KeyPress)
#if defined(XINPUT) && defined(XKB)
	     || (xE->u.u.type == DeviceKeyPress)
#endif
	     ))
	    tempGrab.modifiersDetail.exact =
#ifdef XKB
		(noXkbExtension?gdev->key->prev_state:xkbi->state.grab_mods);
#else
		grab->modifierDevice->key->prev_state;
#endif
	else
	    tempGrab.modifiersDetail.exact =
#ifdef XKB
		(noXkbExtension ? gdev->key->state : xkbi->state.grab_mods);
#else
		grab->modifierDevice->key->state;
#endif
	if (GrabMatchesSecond(&tempGrab, grab) &&
	    (!grab->confineTo ||
	     (grab->confineTo->realized && 
				BorderSizeNotEmpty(device, grab->confineTo))))
	{
	    if (!XaceHook(XACE_DEVICE_ACCESS, wClient(pWin), device, FALSE))
		return FALSE;
#ifdef XKB
	    if (!noXkbExtension) {
		XE_KBPTR.state &= 0x1f00;
		XE_KBPTR.state |=
				tempGrab.modifiersDetail.exact&(~0x1f00);
	    }
#endif
            grabinfo = &device->deviceGrab;
	    (*grabinfo->ActivateGrab)(device, grab, currentTime, TRUE);
 
	    FixUpEventFromWindow(device, xE, grab->window, None, TRUE);

	    (void) TryClientEvents(rClient(grab), xE, count,
				   filters[xE->u.u.type],
				   filters[xE->u.u.type],  grab);

	    if (grabinfo->sync.state == FROZEN_NO_EVENT)
	    {
		if (grabinfo->sync.evcount < count)
		{
		    Must_have_memory = TRUE; /* XXX */
		    grabinfo->sync.event = (xEvent *)xrealloc(grabinfo->sync.event,
							    count*
							    sizeof(xEvent));
		    Must_have_memory = FALSE; /* XXX */
		}
		grabinfo->sync.evcount = count;
		for (dxE = grabinfo->sync.event; --count >= 0; dxE++, xE++)
		    *dxE = *xE;
	    	grabinfo->sync.state = FROZEN_WITH_EVENT;
            }	
	    return TRUE;
	}
    }
    return FALSE;
}

/**
 * CheckDeviceGrabs handles both keyboard and pointer events that may cause
 * a passive grab to be activated.  
 *
 * If the event is a keyboard event, the ancestors of the focus window are
 * traced down and tried to see if they have any passive grabs to be
 * activated.  If the focus window itself is reached and it's descendants
 * contain the pointer, the ancestors of the window that the pointer is in
 * are then traced down starting at the focus window, otherwise no grabs are
 * activated.  
 * If the event is a pointer event, the ancestors of the window that the
 * pointer is in are traced down starting at the root until CheckPassiveGrabs
 * causes a passive grab to activate or all the windows are
 * tried. PRH
 *
 * If a grab is activated, the event has been sent to the client already!
 *
 * @param device The device that caused the event.
 * @param xE The event to handle (most likely {Device}ButtonPress).
 * @param count Number of events in list.
 * @return TRUE if a grab has been activated or false otherwise.
*/

Bool
CheckDeviceGrabs(DeviceIntPtr device, xEvent *xE, 
                 int checkFirst, int count)
{
    int i;
    WindowPtr pWin = NULL;
    FocusClassPtr focus = device->focus;

    if (((xE->u.u.type == ButtonPress)
#if defined(XINPUT) && defined(XKB)
	 || (xE->u.u.type == DeviceButtonPress)
#endif
	 ) && (device->button->buttonsDown != 1))
	return FALSE;

    i = checkFirst;

    if (focus)
    {
	for (; i < focus->traceGood; i++)
	{
	    pWin = focus->trace[i];
	    if (pWin->optional &&
		CheckPassiveGrabsOnWindow(pWin, device, xE, count))
		return TRUE;
	}
  
	if ((focus->win == NoneWin) ||
	    (i >= device->spriteInfo->sprite->spriteTraceGood) ||
	    ((i > checkFirst) &&
             (pWin != device->spriteInfo->sprite->spriteTrace[i-1])))
	    return FALSE;
    }

    for (; i < device->spriteInfo->sprite->spriteTraceGood; i++)
    {
	pWin = device->spriteInfo->sprite->spriteTrace[i];
	if (pWin->optional &&
	    CheckPassiveGrabsOnWindow(pWin, device, xE, count))
	    return TRUE;
    }

    return FALSE;
}

/**
 * Called for keyboard events to deliver event to whatever client owns the
 * focus. Event is delivered to the keyboard's focus window, the root window
 * or to the window owning the input focus.
 *
 * @param keybd The keyboard originating the event.
 * @param xE The event list.
 * @param window Window underneath the sprite.
 * @param count number of events in xE.
 */
void
DeliverFocusedEvent(DeviceIntPtr keybd, xEvent *xE, WindowPtr window, int count)
{
    DeviceIntPtr pointer;
    WindowPtr focus = keybd->focus->win;
    int mskidx = 0;
    if (focus == FollowKeyboardWin)
	focus = inputInfo.keyboard->focus->win;
    if (!focus)
	return;
    if (focus == PointerRootWin)
    {
	DeliverDeviceEvents(window, xE, NullGrab, NullWindow, keybd, count);
	return;
    }
    if ((focus == window) || IsParent(focus, window))
    {
	if (DeliverDeviceEvents(window, xE, NullGrab, focus, keybd, count))
	    return;
    }
    pointer = GetPairedPointer(keybd);
    if (!pointer)
        pointer = inputInfo.pointer;
    /* just deliver it to the focus window */
    FixUpEventFromWindow(pointer, xE, focus, None, FALSE);
    if (xE->u.u.type & EXTENSION_EVENT_BASE)
	mskidx = keybd->id;
    (void)DeliverEventsToWindow(keybd, focus, xE, count, filters[xE->u.u.type],
				NullGrab, mskidx);
}

/**
 * Deliver an event from a device that is currently grabbed. Uses
 * DeliverDeviceEvents() for further delivery if a ownerEvents is set on the
 * grab. If not, TryClientEvents() is used.
 *
 * @param deactivateGrab True if the device's grab should be deactivated.
 */
void
DeliverGrabbedEvent(xEvent *xE, DeviceIntPtr thisDev, 
                    Bool deactivateGrab, int count)
{
    GrabPtr grab;
    GrabInfoPtr grabinfo;
    int deliveries = 0;
    DeviceIntPtr dev;
    xEvent *dxE;
    SpritePtr pSprite = thisDev->spriteInfo->sprite;

    grabinfo = &thisDev->deviceGrab;
    grab = grabinfo->grab;

    if (grab->ownerEvents)
    {
	WindowPtr focus;

	if (thisDev->focus)
	{
	    focus = thisDev->focus->win;
	    if (focus == FollowKeyboardWin)
		focus = inputInfo.keyboard->focus->win;
	}
	else
	    focus = PointerRootWin;
	if (focus == PointerRootWin)
	    deliveries = DeliverDeviceEvents(pSprite->win, xE, grab, 
                                             NullWindow, thisDev, count);
	else if (focus && (focus == pSprite->win || 
                    IsParent(focus, pSprite->win)))
	    deliveries = DeliverDeviceEvents(pSprite->win, xE, grab, focus,
					     thisDev, count);
	else if (focus)
	    deliveries = DeliverDeviceEvents(focus, xE, grab, focus,
					     thisDev, count);
    }
    if (!deliveries)
    {
        if (ACDeviceAllowed(grab->window, thisDev))
        {
            if (xE->u.u.type == GenericEvent)
            {
                /* find evmask for event's extension */
                xGenericEvent* ge = ((xGenericEvent*)xE);
                GenericMaskPtr    gemask = grab->genericMasks;

                if (!gemask || !gemask->eventMask[GEEXTIDX(ge)])
                    return;

                if (GEEventFill(xE))
                    GEEventFill(xE)(ge, thisDev, grab->window, grab);
                deliveries = TryClientEvents(rClient(grab), xE, count, 
                        gemask->eventMask[GEEXTIDX(ge)],
                        generic_filters[GEEXTIDX(ge)][ge->evtype],
                        grab);
            } else 
            {
                Mask mask = grab->eventMask;
                if (grabinfo->fromPassiveGrab  &&
                        grabinfo->implicitGrab && 
                        (xE->u.u.type & EXTENSION_EVENT_BASE))
                    mask = grab->deviceMask;

                FixUpEventFromWindow(thisDev, xE, grab->window, None, TRUE);

                if (!(!(xE->u.u.type & EXTENSION_EVENT_BASE) &&
                        IsInterferingGrab(rClient(grab), thisDev, xE)))
                {
                    deliveries = TryClientEvents(rClient(grab), xE, count,
                            mask, filters[xE->u.u.type], grab);
                }
            }
            if (deliveries && (xE->u.u.type == MotionNotify
#ifdef XINPUT
                        || xE->u.u.type == DeviceMotionNotify
#endif
                        ))
                thisDev->valuator->motionHintWindow = grab->window;
        }
    }
    if (deliveries && !deactivateGrab && (xE->u.u.type != MotionNotify
#ifdef XINPUT
					  && xE->u.u.type != DeviceMotionNotify
#endif
					  ))
	switch (grabinfo->sync.state)
	{
	case FREEZE_BOTH_NEXT_EVENT:
	    for (dev = inputInfo.devices; dev; dev = dev->next)
	    {
		if (dev == thisDev)
		    continue;
		FreezeThaw(dev, TRUE);
		if ((grabinfo->sync.state == FREEZE_BOTH_NEXT_EVENT) &&
		    (CLIENT_BITS(grab->resource) ==
		     CLIENT_BITS(grab->resource)))
		    grabinfo->sync.state = FROZEN_NO_EVENT;
		else
		    grabinfo->sync.other = grab;
	    }
	    /* fall through */
	case FREEZE_NEXT_EVENT:
	    grabinfo->sync.state = FROZEN_WITH_EVENT;
	    FreezeThaw(thisDev, TRUE);
	    if (grabinfo->sync.evcount < count)
	    {
		Must_have_memory = TRUE; /* XXX */
		grabinfo->sync.event = (xEvent *)xrealloc(grabinfo->sync.event,
							 count*sizeof(xEvent));
		Must_have_memory = FALSE; /* XXX */
	    }
	    grabinfo->sync.evcount = count;
	    for (dxE = grabinfo->sync.event; --count >= 0; dxE++, xE++)
		*dxE = *xE;
	    break;
	}
}

/**
 * Main keyboard event processing function for core keyboard events. 
 * Updates the events fields from the current pointer state and delivers the
 * event.
 *
 * For key events, xE will always be a single event.
 *
 * @param xE Event list
 * @param keybd The device that caused an event.
 * @param count Number of elements in xE.
 */
void
#ifdef XKB
CoreProcessKeyboardEvent (xEvent *xE, DeviceIntPtr keybd, int count)
#else
ProcessKeyboardEvent (xEvent *xE, DeviceIntPtr keybd, int count)
#endif
{
    int             key, bit;
    BYTE   *kptr;
    int    i;
    CARD8  modifiers;
    CARD16 mask;
    GrabPtr         grab;
    GrabInfoPtr     grabinfo;
    Bool            deactivateGrab = FALSE;
    KeyClassPtr keyc = keybd->key;
#ifdef XEVIE
    static Window           rootWin = 0;

    if(!xeviegrabState && xevieFlag && clients[xevieClientIndex] &&
          (xevieMask & xevieFilters[xE->u.u.type])) {
      key = xE->u.u.detail;
      kptr = &keyc->down[key >> 3];
      bit = 1 << (key & 7);
      if((xE->u.u.type == KeyPress &&  (*kptr & bit)) ||
         (xE->u.u.type == KeyRelease && !(*kptr & bit)))
      {} else {
#ifdef XKB
        if(!noXkbExtension)
	    xevieKBEventSent = 1;
#endif
        if(!xevieKBEventSent)
        {
          xeviekb = keybd;
          if(!rootWin) {
	      rootWin = GetCurrentRootWindow(keybd)->drawable.id;
          }
          xE->u.keyButtonPointer.event = xeviewin->drawable.id;
          xE->u.keyButtonPointer.root = rootWin;
          xE->u.keyButtonPointer.child = (xeviewin->firstChild) ? xeviewin->firstChild->
drawable.id:0;
          xE->u.keyButtonPointer.rootX = xeviehot.x;
          xE->u.keyButtonPointer.rootY = xeviehot.y;
          xE->u.keyButtonPointer.state = keyc->state;
          WriteToClient(clients[xevieClientIndex], sizeof(xEvent), (char *)xE);
#ifdef XKB
          if(noXkbExtension)
#endif
            return;
        } else {
	    xevieKBEventSent = 0;
        }
      }
    }
#endif

    if (xE->u.u.type & EXTENSION_EVENT_BASE)
        grabinfo = &keybd->deviceGrab;
    else
        grabinfo = &keybd->deviceGrab;

    grab = grabinfo->grab;

    if (!syncEvents.playingEvents)
    {
	NoticeTime(xE);
	if (DeviceEventCallback)
	{
	    DeviceEventInfoRec eventinfo;
	    eventinfo.events = xE;
	    eventinfo.count = count;
	    CallCallbacks(&DeviceEventCallback, (pointer)&eventinfo);
	}
    }
#ifdef XEVIE
    /* fix for bug5094030: don't change the state bit if the event is from XEvIE client */
    if(!(!xeviegrabState && xevieFlag && clients[xevieClientIndex] &&
	 (xevieMask & xevieFilters[xE->u.u.type]
#ifdef XKB
	  && !noXkbExtension
#endif
    )))
#endif
    XE_KBPTR.state = (keyc->state | GetPairedPointer(keybd)->button->state);
    XE_KBPTR.rootX = keybd->spriteInfo->sprite->hot.x;
    XE_KBPTR.rootY = keybd->spriteInfo->sprite->hot.y;
    key = xE->u.u.detail;
    kptr = &keyc->down[key >> 3];
    bit = 1 << (key & 7);
    modifiers = keyc->modifierMap[key];
#if defined(XKB) && defined(XEVIE)
    if(!noXkbExtension && !xeviegrabState &&
       xevieFlag && clients[xevieClientIndex] &&
       (xevieMask & xevieFilters[xE->u.u.type])) {
	switch(xE->u.u.type) {
	  case KeyPress: *kptr &= ~bit; break;
	  case KeyRelease: *kptr |= bit; break;
	}
    }
#endif

    switch (xE->u.u.type)
    {
	case KeyPress: 
	    if (*kptr & bit) /* allow ddx to generate multiple downs */
	    {   
		if (!modifiers)
		{
		    xE->u.u.type = KeyRelease;
		    (*keybd->public.processInputProc)(xE, keybd, count);
		    xE->u.u.type = KeyPress;
		    /* release can have side effects, don't fall through */
		    (*keybd->public.processInputProc)(xE, keybd, count);
		}
		return;
	    }
	    GetPairedPointer(keybd)->valuator->motionHintWindow = NullWindow;
	    *kptr |= bit;
	    keyc->prev_state = keyc->state;
	    for (i = 0, mask = 1; modifiers; i++, mask <<= 1)
	    {
		if (mask & modifiers)
		{
		    /* This key affects modifier "i" */
		    keyc->modifierKeyCount[i]++;
		    keyc->state |= mask;
		    modifiers &= ~mask;
		}
	    }
	    if (!grab && CheckDeviceGrabs(keybd, xE, 0, count))
	    {
		grabinfo->activatingKey = key;
		return;
	    }
	    break;
	case KeyRelease: 
	    if (!(*kptr & bit)) /* guard against duplicates */
		return;
	    GetPairedPointer(keybd)->valuator->motionHintWindow = NullWindow;
	    *kptr &= ~bit;
	    keyc->prev_state = keyc->state;
	    for (i = 0, mask = 1; modifiers; i++, mask <<= 1)
	    {
		if (mask & modifiers) {
		    /* This key affects modifier "i" */
		    if (--keyc->modifierKeyCount[i] <= 0) {
			keyc->state &= ~mask;
			keyc->modifierKeyCount[i] = 0;
		    }
		    modifiers &= ~mask;
		}
	    }
	    if (grabinfo->fromPassiveGrab && (key == grabinfo->activatingKey))
		deactivateGrab = TRUE;
	    break;
	default: 
	    FatalError("Impossible keyboard event");
    }
    if (grab)
	DeliverGrabbedEvent(xE, keybd, deactivateGrab, count);
    else
	DeliverFocusedEvent(keybd, xE, keybd->spriteInfo->sprite->win, count);
    if (deactivateGrab)
        (*grabinfo->DeactivateGrab)(keybd);

    XaceHook(XACE_KEY_AVAIL, xE, keybd, count);
}

#ifdef XKB
/* This function is used to set the key pressed or key released state -
   this is only used when the pressing of keys does not cause 
   CoreProcessKeyEvent to be called, as in for example Mouse Keys.
*/
void
FixKeyState (xEvent *xE, DeviceIntPtr keybd)
{
    int             key, bit;
    BYTE   *kptr;
    KeyClassPtr keyc = keybd->key;

    key = xE->u.u.detail;
    kptr = &keyc->down[key >> 3];
    bit = 1 << (key & 7);

    if (((xE->u.u.type==KeyPress)||(xE->u.u.type==KeyRelease))) {
	DebugF("FixKeyState: Key %d %s\n",key,
			(xE->u.u.type==KeyPress?"down":"up"));
    }

    switch (xE->u.u.type)
    {
	case KeyPress: 
	    *kptr |= bit;
	    break;
	case KeyRelease: 
	    *kptr &= ~bit;
	    break;
	default: 
	    FatalError("Impossible keyboard event");
    }
}
#endif

/** 
 * Main pointer event processing function for core pointer events. 
 * For motion events: update the sprite.
 * For all other events: Update the event fields based on the current sprite
 * state.
 *
 * For core pointer events, xE will always be a single event.
 *
 * @param xE Event list
 * @param mouse The device that caused an event.
 * @param count Number of elements in xE.
 */
void
#ifdef XKB
CoreProcessPointerEvent (xEvent *xE, DeviceIntPtr mouse, int count)
#else
ProcessPointerEvent (xEvent *xE, DeviceIntPtr mouse, int count)
#endif
{
    GrabPtr	        grab = mouse->deviceGrab.grab;
    Bool                deactivateGrab = FALSE;
    ButtonClassPtr      butc = mouse->button;
    SpritePtr           pSprite = mouse->spriteInfo->sprite;

#ifdef XKB
    XkbSrvInfoPtr xkbi= inputInfo.keyboard->key->xkbInfo;
#endif
#ifdef XEVIE
    if(xevieFlag && clients[xevieClientIndex] && !xeviegrabState &&
       (xevieMask & xevieFilters[xE->u.u.type])) {
      if(xevieEventSent)
        xevieEventSent = 0;
      else {
        xeviemouse = mouse;
        WriteToClient(clients[xevieClientIndex], sizeof(xEvent), (char *)xE);
        return;
      }
    }
#endif

    if (!syncEvents.playingEvents)
	NoticeTime(xE)
    XE_KBPTR.state = (butc->state | (
#ifdef XKB
			(noXkbExtension ?
				inputInfo.keyboard->key->state :
				xkbi->state.grab_mods)
#else
			inputInfo.keyboard->key->state
#endif
				    ));
    {
	NoticeTime(xE);
	if (DeviceEventCallback)
	{
	    DeviceEventInfoRec eventinfo;
	    /* see comment in EnqueueEvents regarding the next three lines */
	    if (xE->u.u.type == MotionNotify)
		XE_KBPTR.root =
		    WindowTable[pSprite->hotPhys.pScreen->myNum]->drawable.id;
	    eventinfo.events = xE;
	    eventinfo.count = count;
	    CallCallbacks(&DeviceEventCallback, (pointer)&eventinfo);
	}
    }
    /* We need to call CheckMotion for each event. It doesn't really give us
       any benefit for relative devices, but absolute devices may not send
       button events to the right position otherwise. */
    if (!CheckMotion(xE, mouse) && xE->u.u.type == MotionNotify)
            return;
    if (xE->u.u.type != MotionNotify)
    {
	int  key;
	BYTE *kptr;
	int           bit;

	XE_KBPTR.rootX = pSprite->hot.x;
	XE_KBPTR.rootY = pSprite->hot.y;

	key = xE->u.u.detail;
	kptr = &butc->down[key >> 3];
	bit = 1 << (key & 7);
	switch (xE->u.u.type)
	{
	case ButtonPress: 
	    mouse->valuator->motionHintWindow = NullWindow;
	    if (!(*kptr & bit))
		butc->buttonsDown++;
	    butc->motionMask = ButtonMotionMask;
	    *kptr |= bit;
	    if (xE->u.u.detail == 0)
		return;
	    if (xE->u.u.detail <= 5)
		butc->state |= (Button1Mask >> 1) << xE->u.u.detail;
	    filters[MotionNotify] = Motion_Filter(butc);
	    if (!grab)
		if (CheckDeviceGrabs(mouse, xE, 0, count))
		    return;
	    break;
	case ButtonRelease: 
	    mouse->valuator->motionHintWindow = NullWindow;
	    if (*kptr & bit)
		--butc->buttonsDown;
	    if (!butc->buttonsDown)
		butc->motionMask = 0;
	    *kptr &= ~bit;
	    if (xE->u.u.detail == 0)
		return;
	    if (xE->u.u.detail <= 5)
		butc->state &= ~((Button1Mask >> 1) << xE->u.u.detail);
	    filters[MotionNotify] = Motion_Filter(butc);
	    if (!butc->state && mouse->deviceGrab.fromPassiveGrab)
		deactivateGrab = TRUE;
	    break;
	default: 
	    FatalError("bogus pointer event from ddx");
	}
    }

    if (grab)
	DeliverGrabbedEvent(xE, mouse, deactivateGrab, count);
    else
	DeliverDeviceEvents(pSprite->win, xE, NullGrab, NullWindow,
			    mouse, count);
    if (deactivateGrab)
        (*mouse->deviceGrab.DeactivateGrab)(mouse);
}

#define AtMostOneClient \
	(SubstructureRedirectMask | ResizeRedirectMask | ButtonPressMask)

/**
 * Recalculate which events may be deliverable for the given window.
 * Recalculated mask is used for quicker determination which events may be
 * delivered to a window.
 *
 * The otherEventMasks on a WindowOptional is the combination of all event
 * masks set by all clients on the window.
 * deliverableEventMask is the combination of the eventMask and the
 * otherEventMask.
 *
 * Traverses to siblings and parents of the window.
 */
void
RecalculateDeliverableEvents(pWin)
    WindowPtr pWin;
{
    OtherClients *others;
    WindowPtr pChild;

    pChild = pWin;
    while (1)
    {
	if (pChild->optional)
	{
	    pChild->optional->otherEventMasks = 0;
	    for (others = wOtherClients(pChild); others; others = others->next)
	    {
		pChild->optional->otherEventMasks |= others->mask;
	    }
	}
	pChild->deliverableEvents = pChild->eventMask|
				    wOtherEventMasks(pChild);
	if (pChild->parent)
	    pChild->deliverableEvents |=
		(pChild->parent->deliverableEvents &
		 ~wDontPropagateMask(pChild) & PropagateMask);
	if (pChild->firstChild)
	{
	    pChild = pChild->firstChild;
	    continue;
	}
	while (!pChild->nextSib && (pChild != pWin))
	    pChild = pChild->parent;
	if (pChild == pWin)
	    break;
	pChild = pChild->nextSib;
    }
}

/**
 *
 *  \param value must conform to DeleteType
 */
int
OtherClientGone(pointer value, XID id)
{
    OtherClientsPtr other, prev;
    WindowPtr pWin = (WindowPtr)value;

    prev = 0;
    for (other = wOtherClients(pWin); other; other = other->next)
    {
	if (other->resource == id)
	{
	    if (prev)
		prev->next = other->next;
	    else
	    {
		if (!(pWin->optional->otherClients = other->next))
		    CheckWindowOptionalNeed (pWin);
	    }
	    xfree(other);
	    RecalculateDeliverableEvents(pWin);
	    return(Success);
	}
	prev = other;
    }
    FatalError("client not on event list");
    /*NOTREACHED*/
    return -1; /* make compiler happy */
}

int
EventSelectForWindow(WindowPtr pWin, ClientPtr client, Mask mask)
{
    Mask check;
    OtherClients * others;

    if (mask & ~AllEventMasks)
    {
	client->errorValue = mask;
	return BadValue;
    }
    check = (mask & AtMostOneClient);
    if (check & (pWin->eventMask|wOtherEventMasks(pWin)))
    {				       /* It is illegal for two different
				          clients to select on any of the
				          events for AtMostOneClient. However,
				          it is OK, for some client to
				          continue selecting on one of those
				          events.  */
	if ((wClient(pWin) != client) && (check & pWin->eventMask))
	    return BadAccess;
	for (others = wOtherClients (pWin); others; others = others->next)
	{
	    if (!SameClient(others, client) && (check & others->mask))
		return BadAccess;
	}
    }
    if (wClient (pWin) == client)
    {
	check = pWin->eventMask;
	pWin->eventMask = mask;
    }
    else
    {
	for (others = wOtherClients (pWin); others; others = others->next)
	{
	    if (SameClient(others, client))
	    {
		check = others->mask;
		if (mask == 0)
		{
		    FreeResource(others->resource, RT_NONE);
		    return Success;
		}
		else
		    others->mask = mask;
		goto maskSet;
	    }
	}
	check = 0;
	if (!pWin->optional && !MakeWindowOptional (pWin))
	    return BadAlloc;
	others = (OtherClients *) xalloc(sizeof(OtherClients));
	if (!others)
	    return BadAlloc;
	others->mask = mask;
	others->resource = FakeClientID(client->index);
	others->next = pWin->optional->otherClients;
	pWin->optional->otherClients = others;
	if (!AddResource(others->resource, RT_OTHERCLIENT, (pointer)pWin))
	    return BadAlloc;
    }
maskSet: 
    if ((inputInfo.pointer->valuator->motionHintWindow == pWin) &&
	(mask & PointerMotionHintMask) &&
	!(check & PointerMotionHintMask) &&
	!inputInfo.pointer->deviceGrab.grab) /* VCP shouldn't have deviceGrab */
	inputInfo.pointer->valuator->motionHintWindow = NullWindow;
    RecalculateDeliverableEvents(pWin);
    return Success;
}

int
EventSuppressForWindow(WindowPtr pWin, ClientPtr client, 
                       Mask mask, Bool *checkOptional)
{
    int i, free;

    if (mask & ~PropagateMask)
    {
	client->errorValue = mask;
	return BadValue;
    }
    if (pWin->dontPropagate)
	DontPropagateRefCnts[pWin->dontPropagate]--;
    if (!mask)
	i = 0;
    else
    {
	for (i = DNPMCOUNT, free = 0; --i > 0; )
	{
	    if (!DontPropagateRefCnts[i])
		free = i;
	    else if (mask == DontPropagateMasks[i])
		break;
	}
	if (!i && free)
	{
	    i = free;
	    DontPropagateMasks[i] = mask;
	}
    }
    if (i || !mask)
    {
	pWin->dontPropagate = i;
	if (i)
	    DontPropagateRefCnts[i]++;
	if (pWin->optional)
	{
	    pWin->optional->dontPropagateMask = mask;
	    *checkOptional = TRUE;
	}
    }
    else
    {
	if (!pWin->optional && !MakeWindowOptional (pWin))
	{
	    if (pWin->dontPropagate)
		DontPropagateRefCnts[pWin->dontPropagate]++;
	    return BadAlloc;
	}
	pWin->dontPropagate = 0;
        pWin->optional->dontPropagateMask = mask;
    }
    RecalculateDeliverableEvents(pWin);
    return Success;
}

/**
 * @return The window that is the first ancestor of both a and b.
 */
static WindowPtr 
CommonAncestor(
    WindowPtr a,
    WindowPtr b)
{
    for (b = b->parent; b; b = b->parent)
	if (IsParent(b, a)) return b;
    return NullWindow;
}

/**
 * Assembles an EnterNotify or LeaveNotify and sends it event to the client. 
 * Uses the paired keyboard to get some additional information.
 */
static void
EnterLeaveEvent(
    DeviceIntPtr mouse,
    int type,
    int mode,
    int detail,
    WindowPtr pWin,
    Window child)
{
    xEvent              event;
    WindowPtr		focus;
    DeviceIntPtr        keybd;
    GrabPtr	        grab = mouse->deviceGrab.grab;
    GrabPtr	        devgrab = mouse->deviceGrab.grab;
    Mask		mask;
    int*                inWindow; /* no of sprites inside pWin */
    Bool                sendevent = FALSE;        

    deviceEnterNotify   *devEnterLeave;
    int                 mskidx;
    OtherInputMasks     *inputMasks;

    if (!(keybd = GetPairedKeyboard(mouse)))
        keybd = inputInfo.keyboard;

    if ((pWin == mouse->valuator->motionHintWindow) &&
	(detail != NotifyInferior))
	mouse->valuator->motionHintWindow = NullWindow;
    if (grab)
    {
	mask = (pWin == grab->window) ? grab->eventMask : 0;
	if (grab->ownerEvents)
	    mask |= EventMaskForClient(pWin, rClient(grab));
    }
    else
    {
	mask = pWin->eventMask | wOtherEventMasks(pWin);
    }

    event.u.u.type = type;
    event.u.u.detail = detail;
    event.u.enterLeave.time = currentTime.milliseconds;
    event.u.enterLeave.rootX = mouse->spriteInfo->sprite->hot.x;
    event.u.enterLeave.rootY = mouse->spriteInfo->sprite->hot.y;
    /* Counts on the same initial structure of crossing & button events! */
    FixUpEventFromWindow(mouse, &event, pWin, None, FALSE);
    /* Enter/Leave events always set child */
    event.u.enterLeave.child = child;
    event.u.enterLeave.flags = event.u.keyButtonPointer.sameScreen ?
        ELFlagSameScreen : 0;
#ifdef XKB
    if (!noXkbExtension) {
        event.u.enterLeave.state = mouse->button->state & 0x1f00;
        event.u.enterLeave.state |= 
            XkbGrabStateFromRec(&keybd->key->xkbInfo->state);
    } else
#endif
        event.u.enterLeave.state = keybd->key->state | mouse->button->state;
    event.u.enterLeave.mode = mode;
    focus = keybd->focus->win;
    if ((focus != NoneWin) &&
            ((pWin == focus) || (focus == PointerRootWin) ||
             IsParent(focus, pWin)))
        event.u.enterLeave.flags |= ELFlagFocus;

    inWindow = &((FocusSemaphoresPtr)pWin->devPrivates[FocusPrivatesIndex].ptr)->enterleave;

    /*
     * Sending multiple core enter/leave events to the same window confuse the
     * client.  
     * We can send multiple events that have detail NotifyVirtual or
     * NotifyNonlinearVirtual however.
     *
     * For standard events (NotifyAncestor, NotifyInferior, NotifyNonlinear)
     * we only send an enter event for the first pointer to enter. A leave
     * event is sent for the last pointer to leave. 
     *
     * For events with Virtual detail, we send them only to a window that does
     * not have a pointer inside.
     *
     * For a window tree in the form of 
     *
     * A -> Bp -> C -> D 
     *  \               (where B and E have pointers)
     *    -> Ep         
     *    
     * If the pointer moves from E into D, a LeaveNotify is sent to E, an
     * EnterNotify is sent to D, an EnterNotify with detail
     * NotifyNonlinearVirtual to C and nothing to B.
     */

    if (event.u.u.detail != NotifyVirtual && 
            event.u.u.detail != NotifyNonlinearVirtual)
    {
        (type == EnterNotify) ? (*inWindow)++ : (*inWindow)--;

        if (((*inWindow) == (LeaveNotify - type)))
            sendevent = TRUE;
    } else
    {
        if (!(*inWindow))
            sendevent = TRUE;
    }

    if ((mask & filters[type]) && sendevent)
    {
        if (grab)
            (void)TryClientEvents(rClient(grab), &event, 1, mask,
                                  filters[type], grab);
        else
            (void)DeliverEventsToWindow(mouse, pWin, &event, 1, filters[type],
                                        NullGrab, 0);
    }

    /* we don't have enough bytes, so we squash flags and mode into 
       one byte, and use the last byte for the deviceid. */
    devEnterLeave = (deviceEnterNotify*)&event;
    devEnterLeave->type = (type == EnterNotify) ? DeviceEnterNotify :
        DeviceLeaveNotify;
    devEnterLeave->type = (type == EnterNotify) ? DeviceEnterNotify :
        DeviceLeaveNotify;
    devEnterLeave->mode |= (event.u.enterLeave.flags << 4);
    devEnterLeave->deviceid = mouse->id;
    mskidx = mouse->id;
    inputMasks = wOtherInputMasks(pWin);
    if (inputMasks && 
       (filters[devEnterLeave->type] & inputMasks->deliverableEvents[mskidx]))
    {
        if (devgrab)
            (void)TryClientEvents(rClient(devgrab), (xEvent*)devEnterLeave, 1,
                                mask, filters[devEnterLeave->type], devgrab);
	else
	    (void)DeliverEventsToWindow(mouse, pWin, (xEvent*)devEnterLeave, 
                                        1, filters[devEnterLeave->type], 
                                        NullGrab, mouse->id);
    }

    if ((type == EnterNotify) && (mask & KeymapStateMask))
    {
	xKeymapEvent ke;
	ClientPtr client = grab ? rClient(grab)
				: clients[CLIENT_ID(pWin->drawable.id)];
	if (XaceHook(XACE_DEVICE_ACCESS, client, keybd, FALSE))
	    memmove((char *)&ke.map[0], (char *)&keybd->key->down[1], 31);
	else
	    bzero((char *)&ke.map[0], 31);

	ke.type = KeymapNotify;
	if (grab)
	    (void)TryClientEvents(rClient(grab), (xEvent *)&ke, 1, mask,
				  KeymapStateMask, grab);
	else
	    (void)DeliverEventsToWindow(mouse, pWin, (xEvent *)&ke, 1,
					KeymapStateMask, NullGrab, 0);
    }
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
                    child->drawable.id); }


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
static void
DoEnterLeaveEvents(DeviceIntPtr pDev, 
        WindowPtr fromWin, 
        WindowPtr toWin, 
        int mode) 
{
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

static void
FocusEvent(DeviceIntPtr dev, int type, int mode, int detail, WindowPtr pWin)
{
    xEvent event;
    int* numFoci; /* no of foci the window has already */
    Bool sendevent = FALSE;

    if (dev != inputInfo.keyboard)
	DeviceFocusEvent(dev, type, mode, detail, pWin);

    /*
     * Same procedure as for Enter/Leave events.
     *
     * Sending multiple core FocusIn/Out events to the same window may confuse
     * the client.  
     * We can send multiple events that have detail NotifyVirtual,
     * NotifyNonlinearVirtual, NotifyPointerRoot, NotifyDetailNone or
     * NotifyPointer however.
     *
     * For standard events (NotifyAncestor, NotifyInferior, NotifyNonlinear)
     * we only send an FocusIn event for the first kbd to set the focus. A
     * FocusOut event is sent for the last kbd to set the focus away from the
     * window.. 
     *
     * For events with Virtual detail, we send them only to a window that does
     * not have a focus from another keyboard.
     *
     * For a window tree in the form of 
     *
     * A -> Bf -> C -> D 
     *  \               (where B and E have focus)
     *    -> Ef         
     *    
     * If the focus changes from E into D, a FocusOut is sent to E, a
     * FocusIn is sent to D, a FocusIn with detail
     * NotifyNonlinearVirtual to C and nothing to B.
     */

    numFoci =
        &((FocusSemaphoresPtr)pWin->devPrivates[FocusPrivatesIndex].ptr)->focusinout;
    if (detail != NotifyVirtual && 
            detail != NotifyNonlinearVirtual && 
            detail != NotifyPointer &&
            detail != NotifyPointerRoot &&
            detail != NotifyDetailNone)
    {
        (type == FocusIn) ? (*numFoci)++ : (*numFoci)--;
        if (((*numFoci) == (FocusOut - type)))
            sendevent = TRUE;
    } else
    {
        if (!(*numFoci))
            sendevent = TRUE;
    }

    if (sendevent)
    {
        event.u.focus.mode = mode;
        event.u.u.type = type;
        event.u.u.detail = detail;
        event.u.focus.window = pWin->drawable.id;
        (void)DeliverEventsToWindow(dev, pWin, &event, 1, filters[type], NullGrab,
                                    0);
        if ((type == FocusIn) &&
                ((pWin->eventMask | wOtherEventMasks(pWin)) & KeymapStateMask))
        {
            xKeymapEvent ke;
            ClientPtr client = clients[CLIENT_ID(pWin->drawable.id)];
            if (XaceHook(XACE_DEVICE_ACCESS, client, dev, FALSE))
                memmove((char *)&ke.map[0], (char *)&dev->key->down[1], 31);
            else
                bzero((char *)&ke.map[0], 31);

            ke.type = KeymapNotify;
            (void)DeliverEventsToWindow(dev, pWin, (xEvent *)&ke, 1,
                                        KeymapStateMask, NullGrab, 0);
        }
    }
}

 /*
  * recursive because it is easier
  * no-op if child not descended from ancestor
  */
static Bool
FocusInEvents(
    DeviceIntPtr dev,
    WindowPtr ancestor, WindowPtr child, WindowPtr skipChild,
    int mode, int detail,
    Bool doAncestor)
{
    if (child == NullWindow)
	return ancestor == NullWindow;
    if (ancestor == child)
    {
	if (doAncestor)
	    FocusEvent(dev, FocusIn, mode, detail, child);
	return TRUE;
    }
    if (FocusInEvents(dev, ancestor, child->parent, skipChild, mode, detail,
		      doAncestor))
    {
	if (child != skipChild)
	    FocusEvent(dev, FocusIn, mode, detail, child);
	return TRUE;
    }
    return FALSE;
}

/* dies horribly if ancestor is not an ancestor of child */
static void
FocusOutEvents(
    DeviceIntPtr dev,
    WindowPtr child, WindowPtr ancestor,
    int mode, int detail,
    Bool doAncestor)
{
    WindowPtr  pWin;

    for (pWin = child; pWin != ancestor; pWin = pWin->parent)
	FocusEvent(dev, FocusOut, mode, detail, pWin);
    if (doAncestor)
	FocusEvent(dev, FocusOut, mode, detail, ancestor);
}

void
DoFocusEvents(DeviceIntPtr dev, WindowPtr fromWin, WindowPtr toWin, int mode)
{
    int     out, in;		       /* for holding details for to/from
				          PointerRoot/None */
    int     i;
    SpritePtr pSprite = dev->spriteInfo->sprite;

    if (fromWin == toWin)
	return;
    out = (fromWin == NoneWin) ? NotifyDetailNone : NotifyPointerRoot;
    in = (toWin == NoneWin) ? NotifyDetailNone : NotifyPointerRoot;
 /* wrong values if neither, but then not referenced */

    if ((toWin == NullWindow) || (toWin == PointerRootWin))
    {
	if ((fromWin == NullWindow) || (fromWin == PointerRootWin))
   	{
	    if (fromWin == PointerRootWin)
                FocusOutEvents(dev, pSprite->win, RootWindow(dev), mode,
                               NotifyPointer, TRUE);
	    /* Notify all the roots */
#ifdef PANORAMIX
 	    if ( !noPanoramiXExtension )
	        FocusEvent(dev, FocusOut, mode, out, WindowTable[0]);
	    else 
#endif
	        for (i=0; i<screenInfo.numScreens; i++)
	            FocusEvent(dev, FocusOut, mode, out, WindowTable[i]);
	}
	else
	{
	    if (IsParent(fromWin, pSprite->win))
	      FocusOutEvents(dev, pSprite->win, fromWin, mode, NotifyPointer,
			     FALSE);
	    FocusEvent(dev, FocusOut, mode, NotifyNonlinear, fromWin);
	    /* next call catches the root too, if the screen changed */
	    FocusOutEvents(dev, fromWin->parent, NullWindow, mode,
			   NotifyNonlinearVirtual, FALSE);
	}
	/* Notify all the roots */
#ifdef PANORAMIX
	if ( !noPanoramiXExtension )
	    FocusEvent(dev, FocusIn, mode, in, WindowTable[0]);
	else 
#endif
	    for (i=0; i<screenInfo.numScreens; i++)
	        FocusEvent(dev, FocusIn, mode, in, WindowTable[i]);
	if (toWin == PointerRootWin)
	    (void)FocusInEvents(dev, RootWindow(dev), pSprite->win,
				NullWindow, mode, NotifyPointer, TRUE);
    }
    else
    {
	if ((fromWin == NullWindow) || (fromWin == PointerRootWin))
	{
	    if (fromWin == PointerRootWin)
		FocusOutEvents(dev, pSprite->win, RootWindow(dev), mode,
			       NotifyPointer, TRUE);
#ifdef PANORAMIX
 	    if ( !noPanoramiXExtension )
	        FocusEvent(dev, FocusOut, mode, out, WindowTable[0]);
	    else 
#endif
	        for (i=0; i<screenInfo.numScreens; i++)
	            FocusEvent(dev, FocusOut, mode, out, WindowTable[i]);
	    if (toWin->parent != NullWindow)
	      (void)FocusInEvents(dev, RootWindow(dev), toWin, toWin, mode,
				  NotifyNonlinearVirtual, TRUE);
	    FocusEvent(dev, FocusIn, mode, NotifyNonlinear, toWin);
	    if (IsParent(toWin, pSprite->win))
    	       (void)FocusInEvents(dev, toWin, pSprite->win, NullWindow, mode,
				   NotifyPointer, FALSE);
	}
	else
	{
	    if (IsParent(toWin, fromWin))
	    {
		FocusEvent(dev, FocusOut, mode, NotifyAncestor, fromWin);
		FocusOutEvents(dev, fromWin->parent, toWin, mode,
			       NotifyVirtual, FALSE);
		FocusEvent(dev, FocusIn, mode, NotifyInferior, toWin);
		if ((IsParent(toWin, pSprite->win)) &&
			(pSprite->win != fromWin) &&
			(!IsParent(fromWin, pSprite->win)) &&
			(!IsParent(pSprite->win, fromWin)))
		    (void)FocusInEvents(dev, toWin, pSprite->win, NullWindow,
					mode, NotifyPointer, FALSE);
	    }
	    else
		if (IsParent(fromWin, toWin))
		{
		    if ((IsParent(fromWin, pSprite->win)) &&
			    (pSprite->win != fromWin) &&
			    (!IsParent(toWin, pSprite->win)) &&
			    (!IsParent(pSprite->win, toWin)))
			FocusOutEvents(dev, pSprite->win, fromWin, mode,
				       NotifyPointer, FALSE);
		    FocusEvent(dev, FocusOut, mode, NotifyInferior, fromWin);
		    (void)FocusInEvents(dev, fromWin, toWin, toWin, mode,
					NotifyVirtual, FALSE);
		    FocusEvent(dev, FocusIn, mode, NotifyAncestor, toWin);
		}
		else
		{
		/* neither fromWin or toWin is child of other */
		    WindowPtr common = CommonAncestor(toWin, fromWin);
		/* common == NullWindow ==> different screens */
		    if (IsParent(fromWin, pSprite->win))
			FocusOutEvents(dev, pSprite->win, fromWin, mode,
				       NotifyPointer, FALSE);
		    FocusEvent(dev, FocusOut, mode, NotifyNonlinear, fromWin);
		    if (fromWin->parent != NullWindow)
		      FocusOutEvents(dev, fromWin->parent, common, mode,
				     NotifyNonlinearVirtual, FALSE);
		    if (toWin->parent != NullWindow)
		      (void)FocusInEvents(dev, common, toWin, toWin, mode,
					  NotifyNonlinearVirtual, FALSE);
		    FocusEvent(dev, FocusIn, mode, NotifyNonlinear, toWin);
		    if (IsParent(toWin, pSprite->win))
			(void)FocusInEvents(dev, toWin, pSprite->win, NullWindow,
					    mode, NotifyPointer, FALSE);
		}
	}
    }
}

/**
 * Set the input focus to the given window. Subsequent keyboard events will be
 * delivered to the given window.
 * 
 * Usually called from ProcSetInputFocus as result of a client request. If so,
 * the device is the inputInfo.keyboard.
 * If called from ProcXSetInputFocus as result of a client xinput request, the
 * device is set to the device specified by the client.
 *
 * @param client Client that requested input focus change.
 * @param dev Focus device. 
 * @param focusID The window to obtain the focus. Can be PointerRoot or None.
 * @param revertTo Specifies where the focus reverts to when window becomes
 * unviewable.
 * @param ctime Specifies the time.
 * @param followOK True if pointer is allowed to follow the keyboard.
 */
int
SetInputFocus(
    ClientPtr client,
    DeviceIntPtr dev,
    Window focusID,
    CARD8 revertTo,
    Time ctime,
    Bool followOK)
{
    FocusClassPtr focus;
    WindowPtr focusWin;
    int mode, rc;
    TimeStamp time;
    DeviceIntPtr keybd; /* used for FollowKeyboard or FollowKeyboardWin */


    UpdateCurrentTime();
    if ((revertTo != RevertToParent) &&
	(revertTo != RevertToPointerRoot) &&
	(revertTo != RevertToNone) &&
	((revertTo != RevertToFollowKeyboard) || !followOK))
    {
	client->errorValue = revertTo;
	return BadValue;
    }
    time = ClientTimeToServerTime(ctime);

    if (IsKeyboardDevice(dev))
        keybd = dev;
    else
    {
        keybd = GetPairedKeyboard(dev);
        if (!keybd) 
            keybd = inputInfo.keyboard;
    }

    if ((focusID == None) || (focusID == PointerRoot))
	focusWin = (WindowPtr)(long)focusID;
    else if ((focusID == FollowKeyboard) && followOK)
    {
	focusWin = keybd->focus->win;
    }
    else {
	rc = dixLookupWindow(&focusWin, focusID, client, DixReadAccess);
	if (rc != Success)
	    return rc;
 	/* It is a match error to try to set the input focus to an 
	unviewable window. */
	if(!focusWin->realized)
	    return(BadMatch);
    }
    focus = dev->focus;
    if ((CompareTimeStamps(time, currentTime) == LATER) ||
	(CompareTimeStamps(time, focus->time) == EARLIER))
	return Success;
    mode = (dev->deviceGrab.grab) ? NotifyWhileGrabbed : NotifyNormal;
    if (focus->win == FollowKeyboardWin)
	DoFocusEvents(dev, keybd->focus->win, focusWin, mode);
    else
	DoFocusEvents(dev, focus->win, focusWin, mode);
    focus->time = time;
    focus->revert = revertTo;
    if (focusID == FollowKeyboard)
	focus->win = FollowKeyboardWin;
    else
	focus->win = focusWin;
    if ((focusWin == NoneWin) || (focusWin == PointerRootWin))
	focus->traceGood = 0;
    else
    {
        int depth = 0;
	WindowPtr pWin;

        for (pWin = focusWin; pWin; pWin = pWin->parent) depth++;
        if (depth > focus->traceSize)
        {
	    focus->traceSize = depth+1;
	    Must_have_memory = TRUE; /* XXX */
	    focus->trace = (WindowPtr *)xrealloc(focus->trace,
						 focus->traceSize *
						 sizeof(WindowPtr));
	    Must_have_memory = FALSE; /* XXX */
	}
	focus->traceGood = depth;
        for (pWin = focusWin, depth--; pWin; pWin = pWin->parent, depth--) 
	    focus->trace[depth] = pWin;
    }
    return Success;
}

/**
 * Server-side protocol handling for SetInputFocus request.
 *
 * Sets the input focus for the virtual core keyboard.
 */
int
ProcSetInputFocus(client)
    ClientPtr client;
{
    DeviceIntPtr kbd = PickKeyboard(client);
    REQUEST(xSetInputFocusReq);

    REQUEST_SIZE_MATCH(xSetInputFocusReq);

    if (!XaceHook(XACE_DEVICE_ACCESS, client, inputInfo.keyboard, TRUE))
	return Success;

    return SetInputFocus(client, kbd, stuff->focus,
			 stuff->revertTo, stuff->time, FALSE);
}

/**
 * Server-side protocol handling for GetInputFocus request.
 * 
 * Sends the current input focus for the client's keyboard back to the
 * client.
 */
int
ProcGetInputFocus(ClientPtr client)
{
    DeviceIntPtr kbd = PickKeyboard(client);
    xGetInputFocusReply rep;
    /* REQUEST(xReq); */
    FocusClassPtr focus = kbd->focus;

    REQUEST_SIZE_MATCH(xReq);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    if (focus->win == NoneWin)
	rep.focus = None;
    else if (focus->win == PointerRootWin)
	rep.focus = PointerRoot;
    else rep.focus = focus->win->drawable.id;
    rep.revertTo = focus->revert;
    WriteReplyToClient(client, sizeof(xGetInputFocusReply), &rep);
    return Success;
}

/**
 * Server-side protocol handling for Grabpointer request.
 *
 * Sets an active grab on the client's ClientPointer and returns success
 * status to client.
 */
int
ProcGrabPointer(ClientPtr client)
{
    xGrabPointerReply rep;
    DeviceIntPtr device = PickPointer(client);
    GrabPtr grab;
    WindowPtr pWin, confineTo;
    CursorPtr cursor, oldCursor;
    REQUEST(xGrabPointerReq);
    TimeStamp time;
    int rc;

    REQUEST_SIZE_MATCH(xGrabPointerReq);
    UpdateCurrentTime();
    if ((stuff->pointerMode != GrabModeSync) &&
	(stuff->pointerMode != GrabModeAsync))
    {
	client->errorValue = stuff->pointerMode;
        return BadValue;
    }
    if ((stuff->keyboardMode != GrabModeSync) &&
	(stuff->keyboardMode != GrabModeAsync))
    {
	client->errorValue = stuff->keyboardMode;
        return BadValue;
    }
    if ((stuff->ownerEvents != xFalse) && (stuff->ownerEvents != xTrue))
    {
	client->errorValue = stuff->ownerEvents;
        return BadValue;
    }
    if (stuff->eventMask & ~PointerGrabMask)
    {
	client->errorValue = stuff->eventMask;
        return BadValue;
    }
    rc = dixLookupWindow(&pWin, stuff->grabWindow, client, DixReadAccess);
    if (rc != Success)
	return rc;
    if (stuff->confineTo == None)
	confineTo = NullWindow;
    else 
    {
	rc = dixLookupWindow(&confineTo, stuff->confineTo, client,
			     DixReadAccess);
	if (rc != Success)
	    return rc;
    }
    if (stuff->cursor == None)
	cursor = NullCursor;
    else
    {
	cursor = (CursorPtr)SecurityLookupIDByType(client, stuff->cursor,
						RT_CURSOR, DixReadAccess);
	if (!cursor)
	{
	    client->errorValue = stuff->cursor;
	    return BadCursor;
	}
    }
	/* at this point, some sort of reply is guaranteed. */
    time = ClientTimeToServerTime(stuff->time);
    rep.type = X_Reply;
    rep.sequenceNumber = client->sequence;
    rep.length = 0;
    grab = device->deviceGrab.grab;
    if ((grab) && !SameClient(grab, client))
	rep.status = AlreadyGrabbed;
    else if ((!pWin->realized) ||
             (confineTo &&
                !(confineTo->realized 
                    && BorderSizeNotEmpty(device, confineTo))))
	rep.status = GrabNotViewable;
    else if (device->deviceGrab.sync.frozen &&
	     device->deviceGrab.sync.other && 
             !SameClient(device->deviceGrab.sync.other, client))
	rep.status = GrabFrozen;
    else if ((CompareTimeStamps(time, currentTime) == LATER) ||
	     (CompareTimeStamps(time, device->deviceGrab.grabTime) == EARLIER))
	rep.status = GrabInvalidTime;
    else
    {
	GrabRec tempGrab;

	oldCursor = NullCursor;
	if (grab)
 	{
	    if (grab->confineTo && !confineTo)
		ConfineCursorToWindow(device, RootWindow(device), FALSE, FALSE);
	    oldCursor = grab->cursor;
	}
        tempGrab.next = NULL;
	tempGrab.cursor = cursor;
	tempGrab.resource = client->clientAsMask;
	tempGrab.ownerEvents = stuff->ownerEvents;
	tempGrab.eventMask = stuff->eventMask;
	tempGrab.confineTo = confineTo;
	tempGrab.window = pWin;
	tempGrab.keyboardMode = stuff->keyboardMode;
	tempGrab.pointerMode = stuff->pointerMode;
	tempGrab.device = device;
        tempGrab.coreGrab = True;
        tempGrab.genericMasks = NULL;
	(*device->deviceGrab.ActivateGrab)(device, &tempGrab, time, FALSE);
	if (oldCursor)
	    FreeCursor (oldCursor, (Cursor)0);
	rep.status = GrabSuccess;

        RemoveOtherCoreGrabs(client, device);
    }
    WriteReplyToClient(client, sizeof(xGrabPointerReply), &rep);
    return Success;
}

/**
 * Server-side protocol handling for ChangeActivePointerGrab request.
 *
 * Changes properties of the grab hold by the client. If the client does not
 * hold an active grab on the device, nothing happens. 
 *
 * Works on the client's ClientPointer.
 */
int
ProcChangeActivePointerGrab(ClientPtr client)
{
    DeviceIntPtr device = PickPointer(client);
    GrabPtr      grab = device->deviceGrab.grab;
    CursorPtr newCursor, oldCursor;
    REQUEST(xChangeActivePointerGrabReq);
    TimeStamp time;

    REQUEST_SIZE_MATCH(xChangeActivePointerGrabReq);
    if (stuff->eventMask & ~PointerGrabMask)
    {
	client->errorValue = stuff->eventMask;
        return BadValue;
    }
    if (stuff->cursor == None)
	newCursor = NullCursor;
    else
    {
	newCursor = (CursorPtr)SecurityLookupIDByType(client, stuff->cursor,
						RT_CURSOR, DixReadAccess);
	if (!newCursor)
	{
	    client->errorValue = stuff->cursor;
	    return BadCursor;
	}
    }
    if (!grab)
	return Success;
    if (!SameClient(grab, client))
	return Success;
    time = ClientTimeToServerTime(stuff->time);
    if ((CompareTimeStamps(time, currentTime) == LATER) ||
	     (CompareTimeStamps(time, device->deviceGrab.grabTime) == EARLIER))
	return Success;
    oldCursor = grab->cursor;
    grab->cursor = newCursor;
    if (newCursor)
	newCursor->refcnt++;
    PostNewCursor(device);
    if (oldCursor)
	FreeCursor(oldCursor, (Cursor)0);
    grab->eventMask = stuff->eventMask;
    return Success;
}

/**
 * Server-side protocol handling for UngrabPointer request.
 *
 * Deletes the pointer grab on the client's ClientPointer device.
 */
int
ProcUngrabPointer(ClientPtr client)
{
    DeviceIntPtr device = PickPointer(client);
    GrabPtr grab;
    TimeStamp time;
    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    UpdateCurrentTime();
    grab = device->deviceGrab.grab;
    time = ClientTimeToServerTime(stuff->id);
    if ((CompareTimeStamps(time, currentTime) != LATER) &&
	    (CompareTimeStamps(time, device->deviceGrab.grabTime) != EARLIER) &&
	    (grab) && SameClient(grab, client))
	(*device->deviceGrab.DeactivateGrab)(device);
    return Success;
}

/**
 * Sets a grab on the given device.
 * 
 * Called from ProcGrabKeyboard to work on the client's keyboard.
 * Called from ProcXGrabDevice to work on the device specified by the client.
 * 
 * The parameters this_mode and other_mode represent the keyboard_mode and
 * pointer_mode parameters of XGrabKeyboard(). 
 * See man page for details on all the parameters
 * 
 * @param client Client that owns the grab.
 * @param dev The device to grab. 
 * @param this_mode GrabModeSync or GrabModeAsync
 * @param other_mode GrabModeSync or GrabModeAsync
 * @param status Return code to be returned to the caller.
 * 
 * @returns Success or BadValue.
 */
int
GrabDevice(ClientPtr client, DeviceIntPtr dev, 
           unsigned this_mode, unsigned other_mode, Window grabWindow, 
           unsigned ownerEvents, Time ctime, Mask mask, CARD8 *status,
           Bool coreGrab)
{
    WindowPtr pWin;
    GrabPtr grab;
    TimeStamp time;
    int rc;
    GrabInfoPtr grabInfo = &dev->deviceGrab;

    UpdateCurrentTime();
    if ((this_mode != GrabModeSync) && (this_mode != GrabModeAsync))
    {
	client->errorValue = this_mode;
        return BadValue;
    }
    if ((other_mode != GrabModeSync) && (other_mode != GrabModeAsync))
    {
	client->errorValue = other_mode;
        return BadValue;
    }
    if ((ownerEvents != xFalse) && (ownerEvents != xTrue))
    {
	client->errorValue = ownerEvents;
        return BadValue;
    }
    rc = dixLookupWindow(&pWin, grabWindow, client, DixReadAccess);
    if (rc != Success)
	return rc;
    time = ClientTimeToServerTime(ctime);
    grab = grabInfo->grab;
    if (grab && !SameClient(grab, client))
	*status = AlreadyGrabbed;
    else if (!pWin->realized)
	*status = GrabNotViewable;
    else if ((CompareTimeStamps(time, currentTime) == LATER) ||
	     (CompareTimeStamps(time, grabInfo->grabTime) == EARLIER))
	*status = GrabInvalidTime;
    else if (grabInfo->sync.frozen &&
	     grabInfo->sync.other && !SameClient(grabInfo->sync.other, client))
	*status = GrabFrozen;
    else
    {
	GrabRec tempGrab;

        /* Otherwise segfaults happen on grabbed MPX devices */
        memset(&tempGrab, 0, sizeof(GrabRec));

        tempGrab.next = NULL;
	tempGrab.window = pWin;
	tempGrab.resource = client->clientAsMask;
	tempGrab.ownerEvents = ownerEvents;
	tempGrab.keyboardMode = this_mode;
	tempGrab.pointerMode = other_mode;
	tempGrab.eventMask = mask;
	tempGrab.device = dev;
        tempGrab.cursor = NULL;
        tempGrab.coreGrab = coreGrab;
        tempGrab.genericMasks = NULL;

	(*grabInfo->ActivateGrab)(dev, &tempGrab, time, FALSE);
	*status = GrabSuccess;
    }
    return Success;
}

/**
 * Deactivate any core grabs on the given client except the given device.
 *
 * This fixes race conditions where clients deal with implicit passive grabs
 * on one device, but then actively grab their client pointer, which is
 * another device.
 *
 * Grabs are only removed if the other device matches the type of device. If
 * dev is a pointer device, only other pointer grabs are removed. Likewise, if
 * dev is a keyboard device, only keyboard grabs are removed.
 * 
 * If dev doesn't have a grab, do nothing and go for a beer.
 *
 * @param client The client that is to be limited.
 * @param dev The only device allowed to have a grab on the client.
 */

_X_EXPORT void
RemoveOtherCoreGrabs(ClientPtr client, DeviceIntPtr dev)
{
    if (!dev || !dev->deviceGrab.grab)
        return;

    DeviceIntPtr it = inputInfo.devices;
    for (; it; it = it->next)
    {
        if (it == dev)
            continue;
        /* check for IsPointer Device */

        if (it->deviceGrab.grab &&
                it->deviceGrab.grab->coreGrab &&
                SameClient(it->deviceGrab.grab, client))
        {
            if ((IsPointerDevice(dev) && IsPointerDevice(it)) ||
                    (IsKeyboardDevice(dev) && IsKeyboardDevice(it)))
                (*it->deviceGrab.DeactivateGrab)(it);
        }
    }
}

/**
 * Server-side protocol handling for GrabKeyboard request.
 *
 * Grabs the client's keyboard and returns success status to client.
 */
int
ProcGrabKeyboard(ClientPtr client)
{
    xGrabKeyboardReply rep;
    REQUEST(xGrabKeyboardReq);
    int result;
    DeviceIntPtr keyboard = PickKeyboard(client);

    REQUEST_SIZE_MATCH(xGrabKeyboardReq);

    if (XaceHook(XACE_DEVICE_ACCESS, client, keyboard, TRUE))
    {
	result = GrabDevice(client, keyboard, stuff->keyboardMode,
			    stuff->pointerMode, stuff->grabWindow,
			    stuff->ownerEvents, stuff->time,
			    KeyPressMask | KeyReleaseMask, &rep.status, TRUE);
        RemoveOtherCoreGrabs(client, keyboard);
    }
    else {
	result = Success;
	rep.status = AlreadyGrabbed;
    }

    if (result != Success)
	return result;
    rep.type = X_Reply;
    rep.sequenceNumber = client->sequence;
    rep.length = 0;
    WriteReplyToClient(client, sizeof(xGrabKeyboardReply), &rep);
    return Success;
}

/**
 * Server-side protocol handling for UngrabKeyboard request.
 *
 * Deletes a possible grab on the client's keyboard.
 */
int
ProcUngrabKeyboard(ClientPtr client)
{
    DeviceIntPtr device = PickKeyboard(client);
    GrabPtr grab;
    TimeStamp time;
    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    UpdateCurrentTime();
    grab = device->deviceGrab.grab;
    time = ClientTimeToServerTime(stuff->id);
    if ((CompareTimeStamps(time, currentTime) != LATER) &&
	(CompareTimeStamps(time, device->deviceGrab.grabTime) != EARLIER) &&
	(grab) && SameClient(grab, client) && grab->coreGrab)
	(*device->deviceGrab.DeactivateGrab)(device);
    return Success;
}

/**
 * Server-side protocol handling for QueryPointer request.
 *
 * Returns the current state and position of the client's ClientPointer to the
 * client. 
 */
int
ProcQueryPointer(ClientPtr client)
{
    xQueryPointerReply rep;
    WindowPtr pWin, t;
    DeviceIntPtr mouse = PickPointer(client);
    SpritePtr pSprite = mouse->spriteInfo->sprite;
    int rc;

    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    rc = dixLookupWindow(&pWin, stuff->id, client, DixReadAccess);
    if (rc != Success)
	return rc;
    if (mouse->valuator->motionHintWindow)
	MaybeStopHint(mouse, client);
    rep.type = X_Reply;
    rep.sequenceNumber = client->sequence;
    rep.mask = mouse->button->state | inputInfo.keyboard->key->state;
    rep.length = 0;
    rep.root = (RootWindow(mouse))->drawable.id;
    rep.rootX = pSprite->hot.x;
    rep.rootY = pSprite->hot.y;
    rep.child = None;
    if (pSprite->hot.pScreen == pWin->drawable.pScreen)
    {
	rep.sameScreen = xTrue;
	rep.winX = pSprite->hot.x - pWin->drawable.x;
	rep.winY = pSprite->hot.y - pWin->drawable.y;
	for (t = pSprite->win; t; t = t->parent)
	    if (t->parent == pWin)
	    {
		rep.child = t->drawable.id;
		break;
	    }
    }
    else
    {
	rep.sameScreen = xFalse;
	rep.winX = 0;
	rep.winY = 0;
    }

#ifdef PANORAMIX
    if(!noPanoramiXExtension) {
	rep.rootX += panoramiXdataPtr[0].x;
	rep.rootY += panoramiXdataPtr[0].y;
	if(stuff->id == rep.root) {
	    rep.winX += panoramiXdataPtr[0].x;
	    rep.winY += panoramiXdataPtr[0].y;
	}
    }
#endif

    WriteReplyToClient(client, sizeof(xQueryPointerReply), &rep);

    return(Success);    
}

/**
 * Initializes the device list and the DIX sprite to sane values. Allocates
 * trace memory used for quick window traversal.
 */
void
InitEvents(void)
{
    int i;

    inputInfo.numDevices = 0;
    inputInfo.devices = (DeviceIntPtr)NULL;
    inputInfo.off_devices = (DeviceIntPtr)NULL;
    inputInfo.keyboard = (DeviceIntPtr)NULL;
    inputInfo.pointer = (DeviceIntPtr)NULL;
    lastEventMask = OwnerGrabButtonMask;
    filters[MotionNotify] = PointerMotionMask;
#ifdef XEVIE
    xeviewin = NULL;
#endif

    syncEvents.replayDev = (DeviceIntPtr)NULL;
    syncEvents.replayWin = NullWindow;
    while (syncEvents.pending)
    {
	QdEventPtr next = syncEvents.pending->next;
	xfree(syncEvents.pending);
	syncEvents.pending = next;
    }
    syncEvents.pendtail = &syncEvents.pending;
    syncEvents.playingEvents = FALSE;
    syncEvents.time.months = 0;
    syncEvents.time.milliseconds = 0;	/* hardly matters */
    currentTime.months = 0;
    currentTime.milliseconds = GetTimeInMillis();
    lastDeviceEventTime = currentTime;
    for (i = 0; i < DNPMCOUNT; i++)
    {
	DontPropagateMasks[i] = 0;
	DontPropagateRefCnts[i] = 0;
    }
}

/**
 * This function is deprecated! It shouldn't be used anymore. It used to free
 * the spriteTraces, but now they are freed when the SpriteRec is freed.
 */
_X_DEPRECATED void
CloseDownEvents(void)
{

}

/**
 * Server-side protocol handling for SendEvent request.
 *
 * Locates the window to send the event to and forwards the event. 
 */
int
ProcSendEvent(ClientPtr client)
{
    WindowPtr pWin;
    WindowPtr effectiveFocus = NullWindow; /* only set if dest==InputFocus */
    SpritePtr pSprite = PickPointer(client)->spriteInfo->sprite;
    REQUEST(xSendEventReq);

    REQUEST_SIZE_MATCH(xSendEventReq);

    /* The client's event type must be a core event type or one defined by an
	extension. */

    if ( ! ((stuff->event.u.u.type > X_Reply &&
	     stuff->event.u.u.type < LASTEvent) || 
	    (stuff->event.u.u.type >= EXTENSION_EVENT_BASE &&
	     stuff->event.u.u.type < (unsigned)lastEvent)))
    {
	client->errorValue = stuff->event.u.u.type;
	return BadValue;
    }
    if (stuff->event.u.u.type == ClientMessage &&
	stuff->event.u.u.detail != 8 &&
	stuff->event.u.u.detail != 16 &&
	stuff->event.u.u.detail != 32)
    {
	client->errorValue = stuff->event.u.u.detail;
	return BadValue;
    }
    if (stuff->eventMask & ~AllEventMasks)
    {
	client->errorValue = stuff->eventMask;
	return BadValue;
    }

    if (stuff->destination == PointerWindow)
	pWin = pSprite->win;
    else if (stuff->destination == InputFocus)
    {
	WindowPtr inputFocus = inputInfo.keyboard->focus->win;

	if (inputFocus == NoneWin)
	    return Success;

	/* If the input focus is PointerRootWin, send the event to where
	the pointer is if possible, then perhaps propogate up to root. */
   	if (inputFocus == PointerRootWin)
	    inputFocus = pSprite->spriteTrace[0]; /* Root window! */

	if (IsParent(inputFocus, pSprite->win))
	{
	    effectiveFocus = inputFocus;
	    pWin = pSprite->win;
	}
	else
	    effectiveFocus = pWin = inputFocus;
    }
    else
	dixLookupWindow(&pWin, stuff->destination, client, DixReadAccess);

    if (!pWin)
	return BadWindow;
    if ((stuff->propagate != xFalse) && (stuff->propagate != xTrue))
    {
	client->errorValue = stuff->propagate;
	return BadValue;
    }
    stuff->event.u.u.type |= 0x80;
    if (stuff->propagate)
    {
	for (;pWin; pWin = pWin->parent)
	{
            if (DeliverEventsToWindow(PickPointer(client), pWin,
                        &stuff->event, 1, stuff->eventMask, NullGrab, 0))
		return Success;
	    if (pWin == effectiveFocus)
		return Success;
	    stuff->eventMask &= ~wDontPropagateMask(pWin);
	    if (!stuff->eventMask)
		break;
	}
    }
    else
        (void)DeliverEventsToWindow(PickPointer(client), pWin, &stuff->event,
                                    1, stuff->eventMask, NullGrab, 0);
    return Success;
}

/**
 * Server-side protocol handling for UngrabKey request.
 *
 * Deletes a passive grab for the given key. Works on the
 * client's keyboard.
 */
int
ProcUngrabKey(ClientPtr client)
{
    REQUEST(xUngrabKeyReq);
    WindowPtr pWin;
    GrabRec tempGrab;
    DeviceIntPtr keybd = PickKeyboard(client);
    int rc;

    REQUEST_SIZE_MATCH(xUngrabKeyReq);
    rc = dixLookupWindow(&pWin, stuff->grabWindow, client, DixReadAccess);
    if (rc != Success)
	return rc;

    if (((stuff->key > keybd->key->curKeySyms.maxKeyCode) ||
	 (stuff->key < keybd->key->curKeySyms.minKeyCode))
	&& (stuff->key != AnyKey))
    {
	client->errorValue = stuff->key;
        return BadValue;
    }
    if ((stuff->modifiers != AnyModifier) &&
	(stuff->modifiers & ~AllModifiersMask))
    {
	client->errorValue = stuff->modifiers;
	return BadValue;
    }
    tempGrab.resource = client->clientAsMask;
    tempGrab.device = keybd;
    tempGrab.window = pWin;
    tempGrab.modifiersDetail.exact = stuff->modifiers;
    tempGrab.modifiersDetail.pMask = NULL;
    tempGrab.modifierDevice = inputInfo.keyboard;
    tempGrab.type = KeyPress;
    tempGrab.detail.exact = stuff->key;
    tempGrab.detail.pMask = NULL;
    tempGrab.next = NULL;

    if (!DeletePassiveGrabFromList(&tempGrab))
	return(BadAlloc);
    return(Success);
}

/**
 * Server-side protocol handling for GrabKey request.
 *
 * Creates a grab for the client's keyboard and adds it to the list of passive
 * grabs. 
 */
int
ProcGrabKey(ClientPtr client)
{
    WindowPtr pWin;
    REQUEST(xGrabKeyReq);
    GrabPtr grab;
    DeviceIntPtr keybd = PickKeyboard(client);
    int rc;

    REQUEST_SIZE_MATCH(xGrabKeyReq);
    if ((stuff->ownerEvents != xTrue) && (stuff->ownerEvents != xFalse))
    {
	client->errorValue = stuff->ownerEvents;
	return(BadValue);
    }
    if ((stuff->pointerMode != GrabModeSync) &&
	(stuff->pointerMode != GrabModeAsync))
    {
	client->errorValue = stuff->pointerMode;
        return BadValue;
    }
    if ((stuff->keyboardMode != GrabModeSync) &&
	(stuff->keyboardMode != GrabModeAsync))
    {
	client->errorValue = stuff->keyboardMode;
        return BadValue;
    }
    if (((stuff->key > keybd->key->curKeySyms.maxKeyCode) ||
	 (stuff->key < keybd->key->curKeySyms.minKeyCode))
	&& (stuff->key != AnyKey))
    {
	client->errorValue = stuff->key;
        return BadValue;
    }
    if ((stuff->modifiers != AnyModifier) &&
	(stuff->modifiers & ~AllModifiersMask))
    {
	client->errorValue = stuff->modifiers;
	return BadValue;
    }
    rc = dixLookupWindow(&pWin, stuff->grabWindow, client, DixReadAccess);
    if (rc != Success)
	return rc;

    grab = CreateGrab(client->index, keybd, pWin, 
	(Mask)(KeyPressMask | KeyReleaseMask), (Bool)stuff->ownerEvents,
	(Bool)stuff->keyboardMode, (Bool)stuff->pointerMode,
	keybd, stuff->modifiers, KeyPress, stuff->key, 
	NullWindow, NullCursor);
    if (!grab)
	return BadAlloc;
    return AddPassiveGrabToList(grab);
}


/**
 * Server-side protocol handling for GrabButton request.
 *
 * Creates a grab for the client's ClientPointer and adds it as a passive grab
 * to the list.
 */
int
ProcGrabButton(ClientPtr client)
{
    WindowPtr pWin, confineTo;
    REQUEST(xGrabButtonReq);
    CursorPtr cursor;
    GrabPtr grab;
    DeviceIntPtr pointer, modifierDevice;
    int rc;

    REQUEST_SIZE_MATCH(xGrabButtonReq);
    if ((stuff->pointerMode != GrabModeSync) &&
	(stuff->pointerMode != GrabModeAsync))
    {
	client->errorValue = stuff->pointerMode;
        return BadValue;
    }
    if ((stuff->keyboardMode != GrabModeSync) &&
	(stuff->keyboardMode != GrabModeAsync))
    {
	client->errorValue = stuff->keyboardMode;
        return BadValue;
    }
    if ((stuff->modifiers != AnyModifier) &&
	(stuff->modifiers & ~AllModifiersMask))
    {
	client->errorValue = stuff->modifiers;
	return BadValue;
    }
    if ((stuff->ownerEvents != xFalse) && (stuff->ownerEvents != xTrue))
    {
	client->errorValue = stuff->ownerEvents;
	return BadValue;
    }
    if (stuff->eventMask & ~PointerGrabMask)
    {
	client->errorValue = stuff->eventMask;
        return BadValue;
    }
    rc = dixLookupWindow(&pWin, stuff->grabWindow, client, DixReadAccess);
    if (rc != Success)
	return rc;
    if (stuff->confineTo == None)
       confineTo = NullWindow;
    else {
	rc = dixLookupWindow(&confineTo, stuff->confineTo, client,
			     DixReadAccess);
	if (rc != Success)
	    return rc;
    }
    if (stuff->cursor == None)
	cursor = NullCursor;
    else
    {
	cursor = (CursorPtr)SecurityLookupIDByType(client, stuff->cursor,
						RT_CURSOR, DixReadAccess);
	if (!cursor)
	{
	    client->errorValue = stuff->cursor;
	    return BadCursor;
	}
    }

    pointer = PickPointer(client);
    modifierDevice = GetPairedKeyboard(pointer);
    if (!modifierDevice)
        modifierDevice = inputInfo.keyboard;

    grab = CreateGrab(client->index, pointer, pWin, 
        (Mask)stuff->eventMask, (Bool)stuff->ownerEvents,
        (Bool) stuff->keyboardMode, (Bool)stuff->pointerMode,
        modifierDevice, stuff->modifiers, ButtonPress,
        stuff->button, confineTo, cursor);
    if (!grab)
	return BadAlloc;
    return AddPassiveGrabToList(grab);
}

/**
 * Server-side protocol handling for UngrabButton request.
 *
 * Deletes a passive grab on the client's ClientPointer from the list.
 */
int
ProcUngrabButton(ClientPtr client)
{
    REQUEST(xUngrabButtonReq);
    WindowPtr pWin;
    GrabRec tempGrab;
    int rc;

    REQUEST_SIZE_MATCH(xUngrabButtonReq);
    if ((stuff->modifiers != AnyModifier) &&
	(stuff->modifiers & ~AllModifiersMask))
    {
	client->errorValue = stuff->modifiers;
	return BadValue;
    }
    rc = dixLookupWindow(&pWin, stuff->grabWindow, client, DixReadAccess);
    if (rc != Success)
	return rc;
    tempGrab.resource = client->clientAsMask;
    tempGrab.device = PickPointer(client);
    tempGrab.window = pWin;
    tempGrab.modifiersDetail.exact = stuff->modifiers;
    tempGrab.modifiersDetail.pMask = NULL;
    tempGrab.modifierDevice = inputInfo.keyboard;
    tempGrab.type = ButtonPress;
    tempGrab.detail.exact = stuff->button;
    tempGrab.detail.pMask = NULL;
    tempGrab.next = NULL;

    if (!DeletePassiveGrabFromList(&tempGrab))
	return(BadAlloc);
    return(Success);
}

/**
 * Deactivate any grab that may be on the window, remove the focus.
 * Delete any XInput extension events from the window too. Does not change the
 * window mask. Use just before the window is deleted.
 *
 * If freeResources is set, passive grabs on the window are deleted.
 *
 * @param pWin The window to delete events from.
 * @param freeResources True if resources associated with the window should be
 * deleted.
 */
void
DeleteWindowFromAnyEvents(WindowPtr pWin, Bool freeResources)
{
    WindowPtr		parent;
    DeviceIntPtr	mouse = inputInfo.pointer;
    DeviceIntPtr	keybd = inputInfo.keyboard;
    FocusClassPtr	focus;
    OtherClientsPtr	oc;
    GrabPtr		passive;
    GrabPtr             grab; 


    /* Deactivate any grabs performed on this window, before making any
	input focus changes. */
    grab = mouse->deviceGrab.grab;
    if (grab &&
	((grab->window == pWin) || (grab->confineTo == pWin)))
	(*mouse->deviceGrab.DeactivateGrab)(mouse);


    /* Deactivating a keyboard grab should cause focus events. */
    grab = keybd->deviceGrab.grab;
    if (grab && (grab->window == pWin))
	(*keybd->deviceGrab.DeactivateGrab)(keybd);

    /* And now the real devices */
    for (mouse = inputInfo.devices; mouse; mouse = mouse->next)
    {
        grab = mouse->deviceGrab.grab;
        if (grab && ((grab->window == pWin) || (grab->confineTo == pWin)))
            (*mouse->deviceGrab.DeactivateGrab)(mouse);
    }


    for (keybd = inputInfo.devices; keybd; keybd = keybd->next)
    {
        if (IsKeyboardDevice(keybd))
        {
            focus = keybd->focus;

            /* If the focus window is a root window (ie. has no parent) then don't 
               delete the focus from it. */

            if ((pWin == focus->win) && (pWin->parent != NullWindow))
            {
                int focusEventMode = NotifyNormal;

                /* If a grab is in progress, then alter the mode of focus events. */

                if (keybd->deviceGrab.grab)
                    focusEventMode = NotifyWhileGrabbed;

                switch (focus->revert)
                {
                    case RevertToNone:
                        DoFocusEvents(keybd, pWin, NoneWin, focusEventMode);
                        focus->win = NoneWin;
                        focus->traceGood = 0;
                        break;
                    case RevertToParent:
                        parent = pWin;
                        do
                        {
                            parent = parent->parent;
                            focus->traceGood--;
                        } while (!parent->realized
                                /* This would be a good protocol change -- windows being reparented
                                   during SaveSet processing would cause the focus to revert to the
                                   nearest enclosing window which will survive the death of the exiting
                                   client, instead of ending up reverting to a dying window and thence
                                   to None
                                 */
#ifdef NOTDEF
                                || clients[CLIENT_ID(parent->drawable.id)]->clientGone
#endif
                                );
                        DoFocusEvents(keybd, pWin, parent, focusEventMode);
                        focus->win = parent;
                        focus->revert = RevertToNone;
                        break;
                    case RevertToPointerRoot:
                        DoFocusEvents(keybd, pWin, PointerRootWin, focusEventMode);
                        focus->win = PointerRootWin;
                        focus->traceGood = 0;
                        break;
                }
            }
        }

        if (IsPointerDevice(keybd))
        {
            if (keybd->valuator->motionHintWindow == pWin)
                keybd->valuator->motionHintWindow = NullWindow;
        }
    }

    if (freeResources)
    {
	if (pWin->dontPropagate)
	    DontPropagateRefCnts[pWin->dontPropagate]--;
	while ( (oc = wOtherClients(pWin)) )
	    FreeResource(oc->resource, RT_NONE);
	while ( (passive = wPassiveGrabs(pWin)) )
	    FreeResource(passive->resource, RT_NONE);
     }
#ifdef XINPUT
    DeleteWindowFromAnyExtEvents(pWin, freeResources);
#endif
}

/**
 * Call this whenever some window at or below pWin has changed geometry. If
 * there is a grab on the window, the cursor will be re-confined into the
 * window.
 */
_X_EXPORT void
CheckCursorConfinement(WindowPtr pWin)
{
    GrabPtr grab;
    WindowPtr confineTo;
    DeviceIntPtr pDev;

#ifdef PANORAMIX
    if(!noPanoramiXExtension && pWin->drawable.pScreen->myNum) return;
#endif

    for (pDev = inputInfo.devices; pDev; pDev = pDev->next)
    {
        if (DevHasCursor(pDev))
        {
            grab = pDev->deviceGrab.grab;
            if (grab && (confineTo = grab->confineTo))
            {
                if (!BorderSizeNotEmpty(pDev, confineTo))
                    (*inputInfo.pointer->deviceGrab.DeactivateGrab)(pDev);
                else if ((pWin == confineTo) || IsParent(pWin, confineTo))
                    ConfineCursorToWindow(pDev, confineTo, TRUE, TRUE);
            }
        }
    }
}

Mask
EventMaskForClient(WindowPtr pWin, ClientPtr client)
{
    OtherClientsPtr	other;

    if (wClient (pWin) == client)
	return pWin->eventMask;
    for (other = wOtherClients(pWin); other; other = other->next)
    {
	if (SameClient(other, client))
	    return other->mask;
    }
    return 0;
}

/**
 * Server-side protocol handling for RecolorCursor request.
 */
int
ProcRecolorCursor(ClientPtr client)
{
    CursorPtr pCursor;
    int		nscr;
    ScreenPtr	pscr;
    Bool 	displayed;
    SpritePtr   pSprite = PickPointer(client)->spriteInfo->sprite;
    REQUEST(xRecolorCursorReq);

    REQUEST_SIZE_MATCH(xRecolorCursorReq);
    pCursor = (CursorPtr)SecurityLookupIDByType(client, stuff->cursor,
					RT_CURSOR, DixWriteAccess);
    if ( !pCursor) 
    {
	client->errorValue = stuff->cursor;
	return (BadCursor);
    }

    pCursor->foreRed = stuff->foreRed;
    pCursor->foreGreen = stuff->foreGreen;
    pCursor->foreBlue = stuff->foreBlue;

    pCursor->backRed = stuff->backRed;
    pCursor->backGreen = stuff->backGreen;
    pCursor->backBlue = stuff->backBlue;

    for (nscr = 0; nscr < screenInfo.numScreens; nscr++)
    {
	pscr = screenInfo.screens[nscr];
#ifdef PANORAMIX
	if(!noPanoramiXExtension)
	    displayed = (pscr == pSprite->screen);
	else
#endif
	    displayed = (pscr == pSprite->hotPhys.pScreen);
	( *pscr->RecolorCursor)(PickPointer(client), pscr, pCursor,
				(pCursor == pSprite->current) && displayed);
    }
    return (Success);
}

/**
 * Write the given events to a client, swapping the byte order if necessary.
 * To swap the byte ordering, a callback is called that has to be set up for
 * the given event type.
 *
 * In the case of DeviceMotionNotify trailed by DeviceValuators, the events
 * can be more than one. Usually it's just one event. 
 *
 * Do not modify the event structure passed in. See comment below.
 * 
 * @param pClient Client to send events to.
 * @param count Number of events.
 * @param events The event list.
 */
_X_EXPORT void
WriteEventsToClient(ClientPtr pClient, int count, xEvent *events)
{
#ifdef PANORAMIX
    xEvent    eventCopy;
#endif
    xEvent    *eventTo, *eventFrom;
    int       i,
              eventlength = sizeof(xEvent);

#ifdef XKB
    if ((!noXkbExtension)&&(!XkbFilterEvents(pClient, count, events)))
	return;
#endif

#ifdef PANORAMIX
    if(!noPanoramiXExtension && 
       (panoramiXdataPtr[0].x || panoramiXdataPtr[0].y)) 
    {
	switch(events->u.u.type) {
	case MotionNotify:
	case ButtonPress:
	case ButtonRelease:
	case KeyPress:
	case KeyRelease:
	case EnterNotify:
	case LeaveNotify:
	/* 
	   When multiple clients want the same event DeliverEventsToWindow
	   passes the same event structure multiple times so we can't 
	   modify the one passed to us 
        */
	    count = 1;  /* should always be 1 */
	    memcpy(&eventCopy, events, sizeof(xEvent));
	    eventCopy.u.keyButtonPointer.rootX += panoramiXdataPtr[0].x;
	    eventCopy.u.keyButtonPointer.rootY += panoramiXdataPtr[0].y;
	    if(eventCopy.u.keyButtonPointer.event == 
	       eventCopy.u.keyButtonPointer.root) 
	    {
		eventCopy.u.keyButtonPointer.eventX += panoramiXdataPtr[0].x;
		eventCopy.u.keyButtonPointer.eventY += panoramiXdataPtr[0].y;
	    }
	    events = &eventCopy;
	    break;
	default: break;
	}
    }
#endif

    if (EventCallback)
    {
	EventInfoRec eventinfo;
	eventinfo.client = pClient;
	eventinfo.events = events;
	eventinfo.count = count;
	CallCallbacks(&EventCallback, (pointer)&eventinfo);
    }
#ifdef XSERVER_DTRACE
    if (XSERVER_SEND_EVENT_ENABLED()) {
	for (i = 0; i < count; i++)
	{
	    XSERVER_SEND_EVENT(pClient->index, events[i].u.u.type, &events[i]);
	}
    }
#endif	
    /* Just a safety check to make sure we only have one GenericEvent, it just
     * makes things easier for me right now. (whot) */
    for (i = 1; i < count; i++)
    {
        if (events[i].u.u.type == GenericEvent)
        {
            ErrorF("TryClientEvents: Only one GenericEvent at a time.");
            return; 
        }
    }

    if (events->u.u.type == GenericEvent)
    {
        eventlength += ((xGenericEvent*)events)->length * 4;
        if (eventlength > swapEventLen)
        {
            swapEventLen = eventlength;
            swapEvent = Xrealloc(swapEvent, swapEventLen);
            if (!swapEvent)
            {
                FatalError("WriteEventsToClient: Out of memory.\n");
                return;
            }
        }
    }

    if(pClient->swapped)
    {
	for(i = 0; i < count; i++)
	{
	    eventFrom = &events[i];
            eventTo = swapEvent;

	    /* Remember to strip off the leading bit of type in case
	       this event was sent with "SendEvent." */
	    (*EventSwapVector[eventFrom->u.u.type & 0177])
		(eventFrom, eventTo);

	    (void)WriteToClient(pClient, eventlength, (char *)&eventTo);
	}
    }
    else
    {
        /* only one GenericEvent, remember? that means either count is 1 and
         * eventlength is arbitrary or eventlength is 32 and count doesn't
         * matter. And we're all set. Woohoo. */
	(void)WriteToClient(pClient, count * eventlength, (char *) events);
    }
}

/*
 * Set the client pointer for the given client. Second parameter setter could
 * be used in the future to determine access rights. Unused for now.
 *
 * A client can have exactly one ClientPointer. Each time a
 * request/reply/event is processed and the choice of devices is ambiguous
 * (e.g. QueryPointer request), the server will pick the ClientPointer (see
 * PickPointer()). 
 * If a keyboard is needed, the first keyboard paired with the CP is used.
 */
_X_EXPORT Bool
SetClientPointer(ClientPtr client, ClientPtr setter, DeviceIntPtr device)
{
    client->clientPtr = device;
    return TRUE;
}

/* PickPointer will pick an appropriate pointer for the given client.
 *
 * If a client pointer is set, it will pick the client pointer, otherwise the
 * first available pointer in the list. If no physical device is attached, it
 * will pick the core pointer, but will not store it on the client.
 */
_X_EXPORT DeviceIntPtr
PickPointer(ClientPtr client)
{
    if (!client->clientPtr)
    {
        /* look if there is a real device attached */
        DeviceIntPtr it = inputInfo.devices;
        while (it)
        {
            if (it != inputInfo.pointer && it->spriteInfo->spriteOwner)
            {
                client->clientPtr = it;
                break;
            }
            it = it->next;
        }

        if (!it)
        {
            ErrorF("Picking VCP\n");
            return inputInfo.pointer;
        }
    }
    return client->clientPtr;
}

/* PickKeyboard will pick an appropriate keyboard for the given client by
 * searching the list of devices for the keyboard device that is paired with
 * the client's pointer.
 * If no pointer is paired with the keyboard, the virtual core keyboard is
 * returned.
 */
_X_EXPORT DeviceIntPtr
PickKeyboard(ClientPtr client)
{
    DeviceIntPtr ptr = PickPointer(client);
    DeviceIntPtr kbd = inputInfo.devices;

    while(kbd)
    {
        if (ptr != kbd && 
            IsKeyboardDevice(kbd) && 
            ptr->spriteInfo->sprite == kbd->spriteInfo->sprite)
            return kbd;
        kbd = kbd->next;
    }

    return (kbd) ? kbd : inputInfo.keyboard;
}

/* A client that has one or more core grabs does not get core events from
 * devices it does not have a grab on. Legacy applications behave bad
 * otherwise because they are not used to it and the events interfere.
 * Only applies for core events.
 *
 * Return true if a core event from the device would interfere and should not
 * be delivered.
 */
Bool 
IsInterferingGrab(ClientPtr client, DeviceIntPtr dev, xEvent* event)
{
    DeviceIntPtr it = inputInfo.devices;

    if (dev->deviceGrab.grab && SameClient(dev->deviceGrab.grab, client))
        return FALSE;

    switch(event->u.u.type)
    {
        case KeyPress:
        case KeyRelease:
        case ButtonPress:
        case ButtonRelease:
        case MotionNotify:
        case EnterNotify:
        case LeaveNotify:
            break;
        default:
            return FALSE;
    }

    while(it)
    {
        if (it != dev)
        {
            if (it->deviceGrab.grab && SameClient(it->deviceGrab.grab, client)
                        && !it->deviceGrab.fromPassiveGrab)
            {
                return TRUE;
            }
        }
        it = it->next;
    }

    return FALSE;
}

/**
 * Set the filters for a extension. 
 * The filters array needs to contain the Masks that are applicable for each
 * event type for the given extension.
 * e.g. if generic event type 2 should be let through for windows with
 * MyExampleMask set, make sure that filters[2] == MyExampleMask.
 */
_X_EXPORT void 
SetGenericFilter(int extension, Mask* filters)
{
    generic_filters[extension & 0x7f] = filters;
}


/**
 * Grab a device for XI events and XGE events.
 * grabmode is used to ungrab a device.
 */
_X_EXPORT int
ExtGrabDevice(ClientPtr client, 
              DeviceIntPtr dev, 
              int device_mode,
              WindowPtr grabWindow, 
              WindowPtr confineTo, 
              TimeStamp ctime, 
              Bool ownerEvents, 
              CursorPtr cursor, 
              Mask xi_mask, 
              GenericMaskPtr ge_masks)
{
    GrabInfoPtr grabinfo;
    GrabRec     newGrab;

    UpdateCurrentTime();

    grabinfo = &dev->deviceGrab;

    if (grabinfo->grab && !SameClient(grabinfo->grab, client))
        return AlreadyGrabbed;

    if (!grabWindow->realized)
        return GrabNotViewable;

    if ((CompareTimeStamps(ctime, currentTime) == LATER) ||
            (CompareTimeStamps(ctime, grabinfo->grabTime) == EARLIER))
        return GrabInvalidTime;

    if (grabinfo->sync.frozen && grabinfo->sync.other &&
            !SameClient(grabinfo->sync.other, client))
        return GrabFrozen;

    memset(&newGrab, 0, sizeof(GrabRec));
    newGrab.window         = grabWindow;
    newGrab.resource       = client->clientAsMask;
    newGrab.ownerEvents    = ownerEvents;
    newGrab.device         = dev;
    newGrab.cursor         = cursor;
    newGrab.confineTo      = confineTo;
    newGrab.eventMask      = xi_mask;
    newGrab.genericMasks   = NULL;
    newGrab.next           = NULL;

    if (ge_masks)
    {
        newGrab.genericMasks  = xcalloc(1, sizeof(GenericMaskRec));
        *newGrab.genericMasks = *ge_masks;
        newGrab.genericMasks->next = NULL;
    }

    if (IsPointerDevice(dev))
    {
        newGrab.keyboardMode = GrabModeAsync;
        newGrab.pointerMode  = device_mode;
    } else
    {
        newGrab.keyboardMode = device_mode;
        newGrab.pointerMode  = GrabModeAsync;
    }

    (*grabinfo->ActivateGrab)(dev, &newGrab, ctime, FALSE);
    return GrabSuccess;
}


_X_EXPORT int
ExtUngrabDevice(ClientPtr client, DeviceIntPtr dev)
{
    GrabInfoPtr grabinfo = &dev->deviceGrab;
    if (grabinfo->grab && SameClient(grabinfo->grab, client))
        (*grabinfo->DeactivateGrab)(dev);
    return GrabSuccess;
}



