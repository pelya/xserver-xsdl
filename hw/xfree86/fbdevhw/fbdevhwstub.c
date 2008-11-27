#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "xf86.h"
#include "xf86cmap.h"
#include "fbdevhw.h"

/* Stubs for the static server on platforms that don't support fbdev */


Bool
fbdevHWGetRec(ScrnInfoPtr pScrn)
{
	return FALSE;
}

void
fbdevHWFreeRec(ScrnInfoPtr pScrn)
{
}


_X_EXPORT Bool
fbdevHWProbe(struct pci_device *pPci, char *device, char **namep)
{
	return FALSE;
}

_X_EXPORT Bool
fbdevHWInit(ScrnInfoPtr pScrn, struct pci_device *pPci, char *device)
{
	xf86Msg(X_ERROR, "fbdevhw is not available on this platform\n");
	return FALSE;
}

_X_EXPORT char*
fbdevHWGetName(ScrnInfoPtr pScrn)
{
	return NULL;
}

_X_EXPORT int
fbdevHWGetDepth(ScrnInfoPtr pScrn, int *fbbpp)
{
	return -1;
}

_X_EXPORT int
fbdevHWGetLineLength(ScrnInfoPtr pScrn)
{
	return -1;	/* Should cause something spectacular... */
}

_X_EXPORT int
fbdevHWGetType(ScrnInfoPtr pScrn)
{
	return -1;
}

_X_EXPORT int
fbdevHWGetVidmem(ScrnInfoPtr pScrn)
{
	return -1;
}

_X_EXPORT void
fbdevHWSetVideoModes(ScrnInfoPtr pScrn)
{
}

DisplayModePtr
fbdevHWGetBuildinMode(ScrnInfoPtr pScrn)
{
	return NULL;
}

_X_EXPORT void
fbdevHWUseBuildinMode(ScrnInfoPtr pScrn)
{
}

_X_EXPORT void*
fbdevHWMapVidmem(ScrnInfoPtr pScrn)
{
	return NULL;
}

_X_EXPORT int
fbdevHWLinearOffset(ScrnInfoPtr pScrn)
{
	return 0;
}

_X_EXPORT Bool
fbdevHWUnmapVidmem(ScrnInfoPtr pScrn)
{
	return FALSE;
}

void*
fbdevHWMapMMIO(ScrnInfoPtr pScrn)
{
	return NULL;
}

Bool
fbdevHWUnmapMMIO(ScrnInfoPtr pScrn)
{
	return FALSE;
}

_X_EXPORT Bool
fbdevHWModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{	
	return FALSE;
}

_X_EXPORT void
fbdevHWSave(ScrnInfoPtr pScrn)
{
}

_X_EXPORT void
fbdevHWRestore(ScrnInfoPtr pScrn)
{
}

void
fbdevHWLoadPalette(ScrnInfoPtr pScrn, int numColors, int *indices,
		 LOCO *colors, VisualPtr pVisual)
{
}

ModeStatus
fbdevHWValidMode(int scrnIndex, DisplayModePtr mode, Bool verbose, int flags)
{
	return MODE_ERROR;
}

Bool
fbdevHWSwitchMode(int scrnIndex, DisplayModePtr mode, int flags)
{
	return FALSE;
}

_X_EXPORT void
fbdevHWAdjustFrame(int scrnIndex, int x, int y, int flags)
{
}

Bool
fbdevHWEnterVT(int scrnIndex, int flags)
{
	return FALSE;
}

void
fbdevHWLeaveVT(int scrnIndex, int flags)
{
}

void
fbdevHWDPMSSet(ScrnInfoPtr pScrn, int mode, int flags)
{
}

_X_EXPORT Bool
fbdevHWSaveScreen(ScreenPtr pScreen, int mode)
{
	return FALSE;
}

_X_EXPORT xf86SwitchModeProc *
fbdevHWSwitchModeWeak(void) { return fbdevHWSwitchMode; }

_X_EXPORT xf86AdjustFrameProc *
fbdevHWAdjustFrameWeak(void) { return fbdevHWAdjustFrame; }

_X_EXPORT xf86EnterVTProc *
fbdevHWEnterVTWeak(void) { return fbdevHWEnterVT; }

_X_EXPORT xf86LeaveVTProc *
fbdevHWLeaveVTWeak(void) { return fbdevHWLeaveVT; }

_X_EXPORT xf86ValidModeProc *
fbdevHWValidModeWeak(void) { return fbdevHWValidMode; }

_X_EXPORT xf86DPMSSetProc *
fbdevHWDPMSSetWeak(void) { return fbdevHWDPMSSet; }

_X_EXPORT xf86LoadPaletteProc *
fbdevHWLoadPaletteWeak(void) { return fbdevHWLoadPalette; }

_X_EXPORT SaveScreenProcPtr
fbdevHWSaveScreenWeak(void) { return fbdevHWSaveScreen; }
