/* $XFree86: xc/programs/Xserver/hw/xfree86/ramdac/xf86Cursor.c,v 1.20 2003/02/24 20:43:54 tsi Exp $ */

#include "xf86.h"
#include "xf86_ansic.h"
#include "xf86CursorPriv.h"
#include "colormapst.h"
#include "cursorstr.h"

int xf86CursorScreenIndex = -1;
static unsigned long xf86CursorGeneration = 0;

/* sprite functions */

static Bool xf86CursorRealizeCursor(ScreenPtr, CursorPtr);
static Bool xf86CursorUnrealizeCursor(ScreenPtr, CursorPtr);
static void xf86CursorSetCursor(ScreenPtr, CursorPtr, int, int);
static void xf86CursorMoveCursor(ScreenPtr, int, int);

static miPointerSpriteFuncRec xf86CursorSpriteFuncs = {
   xf86CursorRealizeCursor,
   xf86CursorUnrealizeCursor,
   xf86CursorSetCursor,
   xf86CursorMoveCursor
};

/* Screen functions */

static void xf86CursorInstallColormap(ColormapPtr);
static void xf86CursorRecolorCursor(ScreenPtr, CursorPtr, Bool);
static Bool xf86CursorCloseScreen(int, ScreenPtr);
static void xf86CursorQueryBestSize(int, unsigned short*, unsigned short*,
				    ScreenPtr);

/* ScrnInfoRec functions */

static Bool xf86CursorSwitchMode(int, DisplayModePtr,int);
static Bool xf86CursorEnterVT(int, int);
static void xf86CursorLeaveVT(int, int);
static int  xf86CursorSetDGAMode(int, int, DGADevicePtr);

Bool
xf86InitCursor(
   ScreenPtr pScreen,
   xf86CursorInfoPtr infoPtr
)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    xf86CursorScreenPtr ScreenPriv;
    miPointerScreenPtr PointPriv;

    if (xf86CursorGeneration != serverGeneration) {
	if ((xf86CursorScreenIndex = AllocateScreenPrivateIndex()) < 0)
	    return FALSE;
	xf86CursorGeneration = serverGeneration;
    }

    if (!xf86InitHardwareCursor(pScreen, infoPtr))
	return FALSE;

    ScreenPriv = xcalloc(1, sizeof(xf86CursorScreenRec));
    if (!ScreenPriv)
	return FALSE;

    pScreen->devPrivates[xf86CursorScreenIndex].ptr = ScreenPriv;

    ScreenPriv->SWCursor = TRUE;
    ScreenPriv->isUp = FALSE;
    ScreenPriv->CurrentCursor = NULL;
    ScreenPriv->CursorInfoPtr = infoPtr;
    ScreenPriv->PalettedCursor = FALSE;
    ScreenPriv->pInstalledMap = NULL;

    ScreenPriv->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = xf86CursorCloseScreen;
    ScreenPriv->QueryBestSize = pScreen->QueryBestSize;
    pScreen->QueryBestSize = xf86CursorQueryBestSize;
    ScreenPriv->RecolorCursor = pScreen->RecolorCursor;
    pScreen->RecolorCursor = xf86CursorRecolorCursor;

    if ((infoPtr->pScrn->bitsPerPixel == 8) &&
	!(infoPtr->Flags & HARDWARE_CURSOR_TRUECOLOR_AT_8BPP)) {
	ScreenPriv->InstallColormap = pScreen->InstallColormap;
	pScreen->InstallColormap = xf86CursorInstallColormap;
	ScreenPriv->PalettedCursor = TRUE;
    }

    PointPriv = pScreen->devPrivates[miPointerScreenIndex].ptr;

    ScreenPriv->showTransparent = PointPriv->showTransparent;
    if (infoPtr->Flags & HARDWARE_CURSOR_SHOW_TRANSPARENT)
	PointPriv->showTransparent = TRUE;
    else
	PointPriv->showTransparent = FALSE;
    ScreenPriv->spriteFuncs = PointPriv->spriteFuncs;
    PointPriv->spriteFuncs = &xf86CursorSpriteFuncs;

    ScreenPriv->SwitchMode = pScrn->SwitchMode;
    ScreenPriv->EnterVT = pScrn->EnterVT;
    ScreenPriv->LeaveVT = pScrn->LeaveVT;
    ScreenPriv->SetDGAMode = pScrn->SetDGAMode;
    
    ScreenPriv->ForceHWCursorCount = 0;
    ScreenPriv->HWCursorForced = FALSE;

    if (pScrn->SwitchMode)
	pScrn->SwitchMode = xf86CursorSwitchMode;
    pScrn->EnterVT = xf86CursorEnterVT;
    pScrn->LeaveVT = xf86CursorLeaveVT;
    pScrn->SetDGAMode = xf86CursorSetDGAMode;

    return TRUE;
}

/***** Screen functions *****/

static Bool
xf86CursorCloseScreen(int i, ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    miPointerScreenPtr PointPriv =
	pScreen->devPrivates[miPointerScreenIndex].ptr;
    xf86CursorScreenPtr ScreenPriv =
	pScreen->devPrivates[xf86CursorScreenIndex].ptr;

    if (ScreenPriv->isUp && pScrn->vtSema)
	xf86SetCursor(pScreen, NullCursor, ScreenPriv->x, ScreenPriv->y);

    pScreen->CloseScreen = ScreenPriv->CloseScreen;
    pScreen->QueryBestSize = ScreenPriv->QueryBestSize;
    pScreen->RecolorCursor = ScreenPriv->RecolorCursor;
    if (ScreenPriv->InstallColormap)
	pScreen->InstallColormap = ScreenPriv->InstallColormap;

    PointPriv->spriteFuncs = ScreenPriv->spriteFuncs;
    PointPriv->showTransparent = ScreenPriv->showTransparent;

    pScrn->SwitchMode = ScreenPriv->SwitchMode;
    pScrn->EnterVT = ScreenPriv->EnterVT;
    pScrn->LeaveVT = ScreenPriv->LeaveVT;
    pScrn->SetDGAMode = ScreenPriv->SetDGAMode;

    xfree(ScreenPriv->transparentData);
    xfree(ScreenPriv);

    return (*pScreen->CloseScreen)(i, pScreen);
}

static void
xf86CursorQueryBestSize(
   int class,
   unsigned short *width,
   unsigned short *height,
   ScreenPtr pScreen)
{
    xf86CursorScreenPtr ScreenPriv =
	pScreen->devPrivates[xf86CursorScreenIndex].ptr;

    if (class == CursorShape) {
	if(*width > ScreenPriv->CursorInfoPtr->MaxWidth)
	   *width = ScreenPriv->CursorInfoPtr->MaxWidth;
	if(*height > ScreenPriv->CursorInfoPtr->MaxHeight)
	   *height = ScreenPriv->CursorInfoPtr->MaxHeight;
    } else
	(*ScreenPriv->QueryBestSize)(class, width, height, pScreen);
}

static void
xf86CursorInstallColormap(ColormapPtr pMap)
{
    xf86CursorScreenPtr ScreenPriv =
	pMap->pScreen->devPrivates[xf86CursorScreenIndex].ptr;

    ScreenPriv->pInstalledMap = pMap;

    (*ScreenPriv->InstallColormap)(pMap);
}

static void
xf86CursorRecolorCursor(
    ScreenPtr pScreen,
    CursorPtr pCurs,
    Bool displayed)
{
    xf86CursorScreenPtr ScreenPriv =
	pScreen->devPrivates[xf86CursorScreenIndex].ptr;

    if (!displayed)
	return;

    if (ScreenPriv->SWCursor)
	(*ScreenPriv->RecolorCursor)(pScreen, pCurs, displayed);
    else
	xf86RecolorCursor(pScreen, pCurs, displayed);
}

/***** ScrnInfoRec functions *********/

static Bool
xf86CursorSwitchMode(int index, DisplayModePtr mode, int flags)
{
    Bool ret;
    ScreenPtr pScreen = screenInfo.screens[index];
    xf86CursorScreenPtr ScreenPriv =
	pScreen->devPrivates[xf86CursorScreenIndex].ptr;
    miPointerScreenPtr PointPriv =
	pScreen->devPrivates[miPointerScreenIndex].ptr;

    if (ScreenPriv->isUp) {
	xf86SetCursor(pScreen, NullCursor, ScreenPriv->x, ScreenPriv->y);
	ScreenPriv->isUp = FALSE;
    }

    ret = (*ScreenPriv->SwitchMode)(index, mode, flags);

    /*
     * Cannot restore cursor here because the new frame[XY][01] haven't been
     * calculated yet.  However, because the hardware cursor was removed above,
     * ensure the cursor is repainted by miPointerWarpCursor().
     */
    ScreenPriv->CursorToRestore = ScreenPriv->CurrentCursor;
    PointPriv->waitForUpdate = FALSE;	/* Force cursor repaint */

    return ret;
}

static Bool
xf86CursorEnterVT(int index, int flags)
{
    Bool ret;
    ScreenPtr pScreen = screenInfo.screens[index];
    xf86CursorScreenPtr ScreenPriv =
	pScreen->devPrivates[xf86CursorScreenIndex].ptr;

    ret = (*ScreenPriv->EnterVT)(index, flags);

    if (ScreenPriv->CurrentCursor)
	xf86CursorSetCursor(pScreen, ScreenPriv->CurrentCursor,
			ScreenPriv->x, ScreenPriv->y);
    return ret;
}

static void
xf86CursorLeaveVT(int index, int flags)
{
    ScreenPtr pScreen = screenInfo.screens[index];
    xf86CursorScreenPtr ScreenPriv =
	pScreen->devPrivates[xf86CursorScreenIndex].ptr;

    if (ScreenPriv->isUp) {
	xf86SetCursor(pScreen, NullCursor, ScreenPriv->x, ScreenPriv->y);
	ScreenPriv->isUp = FALSE;
    }
    ScreenPriv->SWCursor = TRUE;

    (*ScreenPriv->LeaveVT)(index, flags);
}

static int
xf86CursorSetDGAMode(int index, int num, DGADevicePtr devRet)
{
    ScreenPtr pScreen = screenInfo.screens[index];
    xf86CursorScreenPtr ScreenPriv =
	pScreen->devPrivates[xf86CursorScreenIndex].ptr;
    int ret;

    if (num && ScreenPriv->isUp) {
	xf86SetCursor(pScreen, NullCursor, ScreenPriv->x, ScreenPriv->y);
	ScreenPriv->isUp = FALSE;
	ScreenPriv->SWCursor = TRUE;
    }

    ret = (*ScreenPriv->SetDGAMode)(index, num, devRet);

    if (ScreenPriv->CurrentCursor && (!num || (ret != Success))) {
	xf86CursorSetCursor(pScreen, ScreenPriv->CurrentCursor,
			ScreenPriv->x, ScreenPriv->y);
    }

    return ret;
}

/****** miPointerSpriteFunctions *******/

static Bool
xf86CursorRealizeCursor(ScreenPtr pScreen, CursorPtr pCurs)
{
    xf86CursorScreenPtr ScreenPriv =
	pScreen->devPrivates[xf86CursorScreenIndex].ptr;

    if (pCurs->refcnt <= 1)
	pCurs->devPriv[pScreen->myNum] = NULL;

    return (*ScreenPriv->spriteFuncs->RealizeCursor)(pScreen, pCurs);
}

static Bool
xf86CursorUnrealizeCursor(ScreenPtr pScreen, CursorPtr pCurs)
{
    xf86CursorScreenPtr ScreenPriv =
	pScreen->devPrivates[xf86CursorScreenIndex].ptr;

    if (pCurs->refcnt <= 1) {
	xfree(pCurs->devPriv[pScreen->myNum]);
	pCurs->devPriv[pScreen->myNum] = NULL;
    }

    return (*ScreenPriv->spriteFuncs->UnrealizeCursor)(pScreen, pCurs);
}

static void
xf86CursorSetCursor(ScreenPtr pScreen, CursorPtr pCurs, int x, int y)
{
    xf86CursorScreenPtr ScreenPriv =
	pScreen->devPrivates[xf86CursorScreenIndex].ptr;
    xf86CursorInfoPtr infoPtr = ScreenPriv->CursorInfoPtr;
    miPointerScreenPtr PointPriv;

    ScreenPriv->CurrentCursor = pCurs;
    ScreenPriv->x = x;
    ScreenPriv->y = y;
    ScreenPriv->CursorToRestore = NULL;

    if (pCurs == NullCursor) {	/* means we're supposed to remove the cursor */
	if (ScreenPriv->SWCursor)
	    (*ScreenPriv->spriteFuncs->SetCursor)(pScreen, NullCursor, x, y);
	else if (ScreenPriv->isUp) {
	    xf86SetCursor(pScreen, NullCursor, x, y);
	    ScreenPriv->isUp = FALSE;
	}
	return;
    }

    ScreenPriv->HotX = pCurs->bits->xhot;
    ScreenPriv->HotY = pCurs->bits->yhot;

    PointPriv = pScreen->devPrivates[miPointerScreenIndex].ptr;

    if (infoPtr->pScrn->vtSema && (ScreenPriv->ForceHWCursorCount || ((
#ifdef ARGB_CURSOR
	pCurs->bits->argb && infoPtr->UseHWCursorARGB &&
	 (*infoPtr->UseHWCursorARGB) (pScreen, pCurs) ) || (
	pCurs->bits->argb == 0 &&
#endif
	(pCurs->bits->height <= infoPtr->MaxHeight) &&
	(pCurs->bits->width <= infoPtr->MaxWidth) &&
	(!infoPtr->UseHWCursor || (*infoPtr->UseHWCursor)(pScreen, pCurs))))))
    {

	if (ScreenPriv->SWCursor)	/* remove the SW cursor */
	      (*ScreenPriv->spriteFuncs->SetCursor)(pScreen, NullCursor, x, y);

	xf86SetCursor(pScreen, pCurs, x, y);
	ScreenPriv->SWCursor = FALSE;
	ScreenPriv->isUp = TRUE;
	PointPriv->waitForUpdate = !infoPtr->pScrn->silkenMouse;
	return;
    }

    PointPriv->waitForUpdate = TRUE;

    if (ScreenPriv->isUp) {
	/* Remove the HW cursor, or make it transparent */
	if (infoPtr->Flags & HARDWARE_CURSOR_SHOW_TRANSPARENT) {
	    xf86SetTransparentCursor(pScreen);
	} else {
	    xf86SetCursor(pScreen, NullCursor, x, y);
	    ScreenPriv->isUp = FALSE;
	}
    }

    ScreenPriv->SWCursor = TRUE;

    if (pCurs->bits->emptyMask && !ScreenPriv->showTransparent)
	pCurs = NullCursor;
    (*ScreenPriv->spriteFuncs->SetCursor)(pScreen, pCurs, x, y);
}

static void
xf86CursorMoveCursor(ScreenPtr pScreen, int x, int y)
{
    xf86CursorScreenPtr ScreenPriv =
	pScreen->devPrivates[xf86CursorScreenIndex].ptr;

    ScreenPriv->x = x;
    ScreenPriv->y = y;

    if (ScreenPriv->CursorToRestore)
	xf86CursorSetCursor(pScreen, ScreenPriv->CursorToRestore,
			    ScreenPriv->x, ScreenPriv->y);
    else if (ScreenPriv->SWCursor)
	(*ScreenPriv->spriteFuncs->MoveCursor)(pScreen, x, y);
    else if (ScreenPriv->isUp)
	xf86MoveCursor(pScreen, x, y);
}

void
xf86ForceHWCursor (ScreenPtr pScreen, Bool on)
{
    xf86CursorScreenPtr ScreenPriv =
	pScreen->devPrivates[xf86CursorScreenIndex].ptr;

    if (on)
    {
	if (ScreenPriv->ForceHWCursorCount++ == 0)
	{
	    if (ScreenPriv->SWCursor && ScreenPriv->CurrentCursor)
	    {
		ScreenPriv->HWCursorForced = TRUE;
		xf86CursorSetCursor (pScreen, ScreenPriv->CurrentCursor,
				     ScreenPriv->x, ScreenPriv->y);
	    }
	    else
		ScreenPriv->HWCursorForced = FALSE;
	}
    }
    else
    {
	if (--ScreenPriv->ForceHWCursorCount == 0)
	{
	    if (ScreenPriv->HWCursorForced && ScreenPriv->CurrentCursor)
		xf86CursorSetCursor (pScreen, ScreenPriv->CurrentCursor,
				     ScreenPriv->x, ScreenPriv->y);
	}
    }
}

xf86CursorInfoPtr
xf86CreateCursorInfoRec(void)
{
    return xcalloc(1, sizeof(xf86CursorInfoRec));
}

void
xf86DestroyCursorInfoRec(xf86CursorInfoPtr infoPtr)
{
    xfree(infoPtr);
}
