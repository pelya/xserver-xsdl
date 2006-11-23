/*
 * miPointer->c
 */


/*

Copyright 1989, 1998  The Open Group

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
*/
#ifdef MPX
 /* 
  * MPX additions:
  * Copyright © 2006 Peter Hutterer
  * License see above.
  * Author: Peter Hutterer <peter@cs.unisa.edu.au>
  *
  */
#endif

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

# define NEED_EVENTS
# include   <X11/X.h>
# include   <X11/Xmd.h>
# include   <X11/Xproto.h>
# include   "misc.h"
# include   "windowstr.h"
# include   "pixmapstr.h"
# include   "mi.h"
# include   "scrnintstr.h"
# include   "mipointrst.h"
# include   "cursorstr.h"
# include   "dixstruct.h"
# include   "inputstr.h"

_X_EXPORT int miPointerScreenIndex;
static unsigned long miPointerGeneration = 0;

#define GetScreenPrivate(s) ((miPointerScreenPtr) ((s)->devPrivates[miPointerScreenIndex].ptr))
#define SetupScreen(s)	miPointerScreenPtr  pScreenPriv = GetScreenPrivate(s)

/*
 * until more than one pointer device exists.
 */

static miPointerPtr miPointer;

#ifdef MPX
/* Multipointers 
 * ID of a device == index in this array.
 */
static miPointerRec miMPPointers[MAX_DEVICES];


/* Check if the given device is a MP device. */
_X_EXPORT Bool 
IsMPDev(DeviceIntPtr pDev) 
{
    return (pDev && pDev->isMPDev && pDev->id < MAX_DEVICES);
}
#endif

static Bool miPointerRealizeCursor(DeviceIntPtr pDev, ScreenPtr pScreen, 
                                   CursorPtr pCursor);
static Bool miPointerUnrealizeCursor(DeviceIntPtr pDev, ScreenPtr pScreen, 
                                     CursorPtr pCursor);
static Bool miPointerDisplayCursor(DeviceIntPtr pDev, ScreenPtr pScreen, 
                                   CursorPtr pCursor);
static void miPointerConstrainCursor(DeviceIntPtr pDev, ScreenPtr pScreen,
                                     BoxPtr pBox); 
static void miPointerPointerNonInterestBox(DeviceIntPtr pDev, 
                                           ScreenPtr pScreen, BoxPtr pBox);
static void miPointerCursorLimits(DeviceIntPtr pDev, ScreenPtr pScreen,
                                  CursorPtr pCursor, BoxPtr pHotBox, 
                                  BoxPtr pTopLeftBox);
static Bool miPointerSetCursorPosition(DeviceIntPtr pDev, ScreenPtr pScreen, 
                                       int x, int y,
				       Bool generateEvent);
static Bool miPointerCloseScreen(int index, ScreenPtr pScreen);
static void miPointerMove(DeviceIntPtr pDev, ScreenPtr pScreen, 
                          int x, int y,
                          unsigned long time);

_X_EXPORT Bool
miPointerInitialize (pScreen, spriteFuncs, screenFuncs, waitForUpdate)
    ScreenPtr		    pScreen;
    miPointerSpriteFuncPtr  spriteFuncs;
    miPointerScreenFuncPtr  screenFuncs;
    Bool		    waitForUpdate;
{
    miPointerScreenPtr	pScreenPriv;

    if (miPointerGeneration != serverGeneration)
    {
	miPointerScreenIndex = AllocateScreenPrivateIndex();
	if (miPointerScreenIndex < 0)
	    return FALSE;
	miPointerGeneration = serverGeneration;
    }
    pScreenPriv = (miPointerScreenPtr) xalloc (sizeof (miPointerScreenRec));
    if (!pScreenPriv)
	return FALSE;
    pScreenPriv->spriteFuncs = spriteFuncs;
    pScreenPriv->screenFuncs = screenFuncs;
    /*
     * check for uninitialized methods
     */
    if (!screenFuncs->EnqueueEvent)
	screenFuncs->EnqueueEvent = mieqEnqueue;
    if (!screenFuncs->NewEventScreen)
	screenFuncs->NewEventScreen = mieqSwitchScreen;
    pScreenPriv->waitForUpdate = waitForUpdate;
    pScreenPriv->showTransparent = FALSE;
    pScreenPriv->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = miPointerCloseScreen;
    pScreen->devPrivates[miPointerScreenIndex].ptr = (pointer) pScreenPriv;
    /*
     * set up screen cursor method table
     */
    pScreen->ConstrainCursor = miPointerConstrainCursor;
    pScreen->CursorLimits = miPointerCursorLimits;
    pScreen->DisplayCursor = miPointerDisplayCursor;
    pScreen->RealizeCursor = miPointerRealizeCursor;
    pScreen->UnrealizeCursor = miPointerUnrealizeCursor;
    pScreen->SetCursorPosition = miPointerSetCursorPosition;
    pScreen->RecolorCursor = miRecolorCursor;
    pScreen->PointerNonInterestBox = miPointerPointerNonInterestBox;
    /*
     * set up the pointer object
     */
    miPointer = (miPointerPtr)xalloc(sizeof(miPointerRec));
    if (!miPointer)
    {
        xfree((pointer)pScreenPriv);
        return FALSE;
    }
    miPointer->pScreen = NULL;
    miPointer->pSpriteScreen = NULL;
    miPointer->pCursor = NULL;
    miPointer->pSpriteCursor = NULL;
    miPointer->limits.x1 = 0;
    miPointer->limits.x2 = 32767;
    miPointer->limits.y1 = 0;
    miPointer->limits.y2 = 32767;
    miPointer->confined = FALSE;
    miPointer->x = 0;
    miPointer->y = 0;

#ifdef MPX
    xfree(miPointer);
    miPointer = &miMPPointers[1];
    {
        int mpPtrIdx = 0; /* loop counter */
        /*
         * Set up pointer objects for multipointer devices.
         */
        while(mpPtrIdx < MAX_DEVICES)
        {
            miMPPointers[mpPtrIdx].pScreen = NULL;
            miMPPointers[mpPtrIdx].pSpriteScreen = NULL;
            miMPPointers[mpPtrIdx].pCursor = NULL;
            miMPPointers[mpPtrIdx].pSpriteCursor = NULL;
            miMPPointers[mpPtrIdx].limits.x1 = 0;
            miMPPointers[mpPtrIdx].limits.x2 = 32767;
            miMPPointers[mpPtrIdx].limits.y1 = 0;
            miMPPointers[mpPtrIdx].limits.y2 = 32767;
            miMPPointers[mpPtrIdx].confined = FALSE;
            miMPPointers[mpPtrIdx].x = 0;
            miMPPointers[mpPtrIdx].y = 0;
            mpPtrIdx++;
        }
    }
#endif

    return TRUE;
}

static Bool
miPointerCloseScreen (index, pScreen)
    int		index;
    ScreenPtr	pScreen;
{
    int mpPointerIdx = 0;
    SetupScreen(pScreen);

#ifdef MPX
    while(mpPointerIdx < MAX_DEVICES)
    {
        if (pScreen == miMPPointers[mpPointerIdx].pScreen) 
            miMPPointers[mpPointerIdx].pScreen = 0;
        if (pScreen == miMPPointers[mpPointerIdx].pSpriteScreen) 
            miMPPointers[mpPointerIdx].pSpriteScreen = 0;
        mpPointerIdx++;
    }
#else
    if (pScreen == miPointer->pScreen)
	miPointer->pScreen = 0;
    if (pScreen == miPointer->pSpriteScreen)
	miPointer->pSpriteScreen = 0;
    xfree((pointer)miPointer);
#endif
    pScreen->CloseScreen = pScreenPriv->CloseScreen;
    xfree ((pointer) pScreenPriv);
    return (*pScreen->CloseScreen) (index, pScreen);
}

/*
 * DIX/DDX interface routines
 */

static Bool
miPointerRealizeCursor (pDev, pScreen, pCursor)
    DeviceIntPtr pDev;
    ScreenPtr	 pScreen;
    CursorPtr	 pCursor;
{
    SetupScreen(pScreen);
    return (*pScreenPriv->spriteFuncs->RealizeCursor) (pDev, pScreen, pCursor);
}

static Bool
miPointerUnrealizeCursor (pDev, pScreen, pCursor)
    DeviceIntPtr pDev;
    ScreenPtr	 pScreen;
    CursorPtr	 pCursor;
{
    SetupScreen(pScreen);
    return (*pScreenPriv->spriteFuncs->UnrealizeCursor) (pDev, pScreen, pCursor);
}

static Bool
miPointerDisplayCursor (pDev, pScreen, pCursor)
    DeviceIntPtr pDev;
    ScreenPtr 	 pScreen;
    CursorPtr	 pCursor;
{
#ifdef MPX
    /* use core pointer for non MPX devices */
    if (!IsMPDev(pDev))
        pDev = inputInfo.pointer;

    miMPPointers[pDev->id].pCursor = pCursor;
    miMPPointers[pDev->id].pScreen = pScreen;
    miPointerUpdateSprite(pDev);
#else
    miPointer->pCursor = pCursor;
    miPointer->pScreen = pScreen;
    miPointerUpdateSprite(inputInfo.pointer);
#endif
    return TRUE;
}

static void
miPointerConstrainCursor (pDev, pScreen, pBox)
    DeviceIntPtr pDev;
    ScreenPtr	pScreen;
    BoxPtr	pBox;
{
    miPointerPtr pPointer = miPointer;
#ifdef MPX
    if (IsMPDev(pDev))
        pPointer = &miMPPointers[pDev->id];
#endif
    pPointer->limits = *pBox;
    pPointer->confined = PointerConfinedToScreen(pDev);
}

/*ARGSUSED*/
static void
miPointerPointerNonInterestBox (pDev, pScreen, pBox)
    DeviceIntPtr pDev;
    ScreenPtr	 pScreen;
    BoxPtr	 pBox;
{
    /* until DIX uses this, this will remain a stub */
}

/*ARGSUSED*/
static void
miPointerCursorLimits(pDev, pScreen, pCursor, pHotBox, pTopLeftBox)
    DeviceIntPtr pDev;
    ScreenPtr	 pScreen;
    CursorPtr	 pCursor;
    BoxPtr	 pHotBox;
    BoxPtr	 pTopLeftBox;
{
    *pTopLeftBox = *pHotBox;
}

static Bool GenerateEvent;

static Bool
miPointerSetCursorPosition(pDev, pScreen, x, y, generateEvent)
    DeviceIntPtr pDev;
    ScreenPtr    pScreen;
    int          x, y;
    Bool         generateEvent;
{
    SetupScreen (pScreen);

    GenerateEvent = generateEvent;
    /* device dependent - must pend signal and call miPointerWarpCursor */
    (*pScreenPriv->screenFuncs->WarpCursor) (pDev, pScreen, x, y);
    if (!generateEvent)
	miPointerUpdateSprite(pDev);
    return TRUE;
}

/* Once signals are ignored, the WarpCursor function can call this */

_X_EXPORT void
miPointerWarpCursor (pDev, pScreen, x, y)
    DeviceIntPtr pDev;
    ScreenPtr	 pScreen;
    int	   	 x, y;
{
    miPointerPtr pPointer = miPointer;
    SetupScreen (pScreen);

#ifdef MPX
    if (IsMPDev(pDev))
        pPointer = &miMPPointers[pDev->id];
#endif


    if (pPointer->pScreen != pScreen)
	(*pScreenPriv->screenFuncs->NewEventScreen) (pScreen, TRUE);

    if (GenerateEvent)
    {
	miPointerMoved (pDev, pScreen, x, y, GetTimeInMillis()); 
    }
    else
    {
	/* everything from miPointerMove except the event and history */

    	if (!pScreenPriv->waitForUpdate && pScreen == pPointer->pSpriteScreen)
    	{
	    pPointer->devx = x;
	    pPointer->devy = y;
	    if(!pPointer->pCursor->bits->emptyMask)
		(*pScreenPriv->spriteFuncs->MoveCursor) (pDev, pScreen, x, y);
    	}
	pPointer->x = x;
	pPointer->y = y;
	pPointer->pScreen = pScreen;
    }
}

/*
 * Pointer/CursorDisplay interface routines
 */

/*
 * miPointerUpdate
 *
 * Syncronize the sprite with the cursor - called from ProcessInputEvents
 */

void
miPointerUpdate ()
{
    miPointerUpdateSprite(inputInfo.pointer);
}

void
miPointerUpdateSprite (DeviceIntPtr pDev)
{
    ScreenPtr		pScreen;
    miPointerScreenPtr	pScreenPriv;
    CursorPtr		pCursor;
    int			x, y, devx, devy;
    miPointerPtr        pPointer = miPointer;

#ifdef MPX
    if (!pDev || 
            !(pDev->coreEvents || pDev == inputInfo.pointer || pDev->isMPDev))
#else
    if (!pDev || !(pDev->coreEvents || pDev == inputInfo.pointer))
#endif
        return;

#ifdef MPX
    if (IsMPDev(pDev))
        pPointer = &miMPPointers[pDev->id];
#endif

    pScreen = pPointer->pScreen;
    if (!pScreen)
	return;

    x = pPointer->x;
    y = pPointer->y;
    devx = pPointer->devx;
    devy = pPointer->devy;

    pScreenPriv = GetScreenPrivate (pScreen);
    /*
     * if the cursor has switched screens, disable the sprite
     * on the old screen
     */
    if (pScreen != pPointer->pSpriteScreen)
    {
	if (pPointer->pSpriteScreen)
	{
	    miPointerScreenPtr  pOldPriv;
    	
	    pOldPriv = GetScreenPrivate (pPointer->pSpriteScreen);
	    if (pPointer->pCursor)
	    {
	    	(*pOldPriv->spriteFuncs->SetCursor)
			    	(pDev, pPointer->pSpriteScreen, NullCursor, 0, 0);
	    }
	    (*pOldPriv->screenFuncs->CrossScreen) (pPointer->pSpriteScreen, FALSE);
	}
	(*pScreenPriv->screenFuncs->CrossScreen) (pScreen, TRUE);
	(*pScreenPriv->spriteFuncs->SetCursor)
				(pDev, pScreen, pPointer->pCursor, x, y);
	pPointer->devx = x;
	pPointer->devy = y;
	pPointer->pSpriteCursor = pPointer->pCursor;
	pPointer->pSpriteScreen = pScreen;
    }
    /*
     * if the cursor has changed, display the new one
     */
    else if (pPointer->pCursor != pPointer->pSpriteCursor)
    {
	pCursor = pPointer->pCursor;
	if (pCursor->bits->emptyMask && !pScreenPriv->showTransparent)
	    pCursor = NullCursor;
	(*pScreenPriv->spriteFuncs->SetCursor) (pDev, pScreen, pCursor, x, y);

	pPointer->devx = x;
	pPointer->devy = y;
	pPointer->pSpriteCursor = pPointer->pCursor;
    }
    else if (x != devx || y != devy)
    {
	pPointer->devx = x;
	pPointer->devy = y;
	if(!pPointer->pCursor->bits->emptyMask)
	    (*pScreenPriv->spriteFuncs->MoveCursor) (pDev, pScreen, x, y);
    }
}

/*
 * miPointerDeltaCursor.  The pointer has moved dx,dy from it's previous
 * position.
 */

void
miPointerDeltaCursor (int dx, int dy, unsigned long time)
{
    int x = miPointer->x + dx, y = miPointer->y + dy;

    miPointerSetPosition(inputInfo.pointer, &x, &y, time);
}

void
miPointerSetNewScreen(int screen_no, int x, int y)
{
    miPointerSetScreen(inputInfo.pointer, screen_no, x, y);
}

void
miPointerSetScreen(DeviceIntPtr pDev, int screen_no, int x, int y)
{
	miPointerScreenPtr pScreenPriv;
	ScreenPtr pScreen;
        miPointerPtr pPointer = miPointer;

	pScreen = screenInfo.screens[screen_no];
	pScreenPriv = GetScreenPrivate (pScreen);
	(*pScreenPriv->screenFuncs->NewEventScreen) (pScreen, FALSE);
	NewCurrentScreen (pDev, pScreen, x, y);
#ifdef MPX
        if (IsMPDev(pDev))
            pPointer = &miMPPointers[pDev->id];
#endif
        pPointer->limits.x2 = pScreen->width;
        pPointer->limits.y2 = pScreen->height;
}

_X_EXPORT ScreenPtr
miPointerCurrentScreen ()
{
    return miPointerGetScreen(inputInfo.pointer);
}

_X_EXPORT ScreenPtr
miPointerGetScreen(DeviceIntPtr pDev)
{
    miPointerPtr pPointer;
#ifdef MPX
    if (IsMPDev(pDev))
        pPointer = &miMPPointers[pDev->id];
#endif
    return pPointer->pScreen;
}

/* Move the pointer to x, y on the current screen, update the sprite, and
 * the motion history.  Generates no events.  Does not return changed x
 * and y if they are clipped; use miPointerSetPosition instead. */
_X_EXPORT void
miPointerAbsoluteCursor (int x, int y, unsigned long time)
{
    miPointerSetPosition(inputInfo.pointer, &x, &y, time);
}

_X_EXPORT void
miPointerSetPosition(DeviceIntPtr pDev, int *x, int *y, unsigned long time)
{
    miPointerScreenPtr	pScreenPriv;
    ScreenPtr		pScreen;
    ScreenPtr		newScreen;

    miPointerPtr        pPointer = miPointer;
#ifdef MPX
    if (IsMPDev(pDev))
        pPointer = &(miMPPointers[pDev->id]);
#endif

    pScreen = pPointer->pScreen;
    if (!pScreen)
	return;	    /* called before ready */

#ifdef MPX
    if (!pDev || 
            !(pDev->coreEvents || pDev == inputInfo.pointer || pDev->isMPDev))
#else
    if (!pDev || !(pDev->coreEvents || pDev == inputInfo.pointer))
#endif
        return;

    if (*x < 0 || *x >= pScreen->width || *y < 0 || *y >= pScreen->height)
    {
	pScreenPriv = GetScreenPrivate (pScreen);
	if (!pPointer->confined)
	{
	    newScreen = pScreen;
	    (*pScreenPriv->screenFuncs->CursorOffScreen) (&newScreen, x, y);
	    if (newScreen != pScreen)
	    {
		pScreen = newScreen;
		(*pScreenPriv->screenFuncs->NewEventScreen) (pScreen, FALSE);
		pScreenPriv = GetScreenPrivate (pScreen);
	    	/* Smash the confine to the new screen */
                pPointer->limits.x2 = pScreen->width;
                pPointer->limits.y2 = pScreen->height;
	    }
	}
    }
    /* Constrain the sprite to the current limits. */
    if (*x < pPointer->limits.x1)
	*x = pPointer->limits.x1;
    if (*x >= pPointer->limits.x2)
	*x = pPointer->limits.x2 - 1;
    if (*y < pPointer->limits.y1)
	*y = pPointer->limits.y1;
    if (*y >= pPointer->limits.y2)
	*y = pPointer->limits.y2 - 1;

    if (pPointer->x == *x && pPointer->y == *y && 
            pPointer->pScreen == pScreen) 
        return;

    miPointerMoved(pDev, pScreen, *x, *y, time);
}

_X_EXPORT void
miPointerPosition (int *x, int *y)
{
    miPointerGetPosition(inputInfo.pointer, x, y);
}

_X_EXPORT void
miPointerGetPosition(DeviceIntPtr pDev, int *x, int *y)
{
    miPointerPtr pPointer = miPointer;
#ifdef MPX
    if (IsMPDev(pDev))
        pPointer = &miMPPointers[pDev->id];
#endif
    *x = pPointer->x;
    *y = pPointer->y;
}

void
miPointerMove (DeviceIntPtr pDev, ScreenPtr pScreen, int x, int y, unsigned long time)
{
    miPointerMoved(pDev, pScreen, x, y, time);
}

/* Move the pointer on the current screen,  and update the sprite. */
void
miPointerMoved (DeviceIntPtr pDev, ScreenPtr pScreen, int x, int y,
                     unsigned long time)
{
    miPointerPtr pPointer = miPointer;
    SetupScreen(pScreen);

#ifdef MPX
    if (IsMPDev(pDev))
        pPointer = &miMPPointers[pDev->id];
#endif

    if (pDev && (pDev->coreEvents || pDev == inputInfo.pointer
#ifdef MPX
                || pDev->isMPDev
#endif
                ) &&
        !pScreenPriv->waitForUpdate && pScreen == pPointer->pSpriteScreen)
    {
	pPointer->devx = x;
	pPointer->devy = y;
	if(!pPointer->pCursor->bits->emptyMask)
	    (*pScreenPriv->spriteFuncs->MoveCursor) (pDev, pScreen, x, y);
    }

    pPointer->x = x;
    pPointer->y = y;
    pPointer->pScreen = pScreen;
}
