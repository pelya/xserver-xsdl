/*
 * mipointer.c
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
 /* 
  * MPX additions:
  * Copyright © 2006 Peter Hutterer
  * License see above.
  * Author: Peter Hutterer <peter@cs.unisa.edu.au>
  *
  */

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

static miPointerPtr miCorePointer;

/* Multipointers 
 * ID of a device == index in this array.
 */
static miPointerRec miPointers[MAX_DEVICES];
#define MIPOINTER(dev) \
    (MPHasCursor((dev))) ? &miPointers[(dev)->id] : miCorePointer

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
    miPointerPtr        pPointer;
    int                 ptrIdx;

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
     * virtual core pointer ID is always 1, so we let it point to the matching
     * index in the array.
     */
    miCorePointer = &miPointers[1];
    for(ptrIdx = 0; ptrIdx < MAX_DEVICES; ptrIdx++)
    {
            pPointer = &miPointers[ptrIdx];
            pPointer->pScreen = NULL;
            pPointer->pSpriteScreen = NULL;
            pPointer->pCursor = NULL;
            pPointer->pSpriteCursor = NULL;
            pPointer->limits.x1 = 0;
            pPointer->limits.x2 = 32767;
            pPointer->limits.y1 = 0;
            pPointer->limits.y2 = 32767;
            pPointer->confined = FALSE;
            pPointer->x = 0;
            pPointer->y = 0;
    }

    return TRUE;
}

static Bool
miPointerCloseScreen (index, pScreen)
    int		index;
    ScreenPtr	pScreen;
{
    miPointerPtr pPointer;
    int ptrIdx;

    SetupScreen(pScreen);

    for(ptrIdx = 0; ptrIdx < MAX_DEVICES; ptrIdx++)
    {
        pPointer = &miPointers[ptrIdx];

        if (pScreen == pPointer->pScreen)
            pPointer->pScreen = 0;
        if (pScreen == pPointer->pSpriteScreen)
            pPointer->pSpriteScreen = 0;
    }

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
    miPointerPtr pPointer = MIPOINTER(pDev);

    pPointer->pCursor = pCursor;
    pPointer->pScreen = pScreen;
    miPointerUpdateSprite(pDev);
    return TRUE;
}

static void
miPointerConstrainCursor (pDev, pScreen, pBox)
    DeviceIntPtr pDev;
    ScreenPtr	pScreen;
    BoxPtr	pBox;
{
    miPointerPtr pPointer = MIPOINTER(pDev);

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
    miPointerPtr pPointer = MIPOINTER(pDev);
    SetupScreen (pScreen);

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
    miPointerPtr        pPointer;

    if (!pDev || 
            !(pDev->coreEvents || pDev == inputInfo.pointer || pDev->isMPDev))
        return;

    pPointer = MIPOINTER(pDev);

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
    int x = miCorePointer->x + dx, y = miCorePointer->y + dy;

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
        miPointerPtr pPointer = MIPOINTER(pDev);

	pScreen = screenInfo.screens[screen_no];
	pScreenPriv = GetScreenPrivate (pScreen);
	(*pScreenPriv->screenFuncs->NewEventScreen) (pScreen, FALSE);
	NewCurrentScreen (pDev, pScreen, x, y);

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
    miPointerPtr pPointer = MIPOINTER(pDev);
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

    miPointerPtr        pPointer = MIPOINTER(pDev);

    pScreen = pPointer->pScreen;
    if (!pScreen)
	return;	    /* called before ready */

    if (!pDev || 
            !(pDev->coreEvents || pDev == inputInfo.pointer || pDev->isMPDev))
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
    miPointerPtr pPointer = MIPOINTER(pDev);
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
    miPointerPtr pPointer = MIPOINTER(pDev);
    SetupScreen(pScreen);


    if (pDev && (pDev->coreEvents || pDev == inputInfo.pointer || pDev->isMPDev)
        && !pScreenPriv->waitForUpdate && pScreen == pPointer->pSpriteScreen)
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
