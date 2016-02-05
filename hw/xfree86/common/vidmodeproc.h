
/* Prototypes for DGA functions that the DDX must provide */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _VIDMODEPROC_H_
#define _VIDMODEPROC_H_

typedef enum {
    VIDMODE_H_DISPLAY,
    VIDMODE_H_SYNCSTART,
    VIDMODE_H_SYNCEND,
    VIDMODE_H_TOTAL,
    VIDMODE_H_SKEW,
    VIDMODE_V_DISPLAY,
    VIDMODE_V_SYNCSTART,
    VIDMODE_V_SYNCEND,
    VIDMODE_V_TOTAL,
    VIDMODE_FLAGS,
    VIDMODE_CLOCK
} VidModeSelectMode;

typedef enum {
    VIDMODE_MON_VENDOR,
    VIDMODE_MON_MODEL,
    VIDMODE_MON_NHSYNC,
    VIDMODE_MON_NVREFRESH,
    VIDMODE_MON_HSYNC_LO,
    VIDMODE_MON_HSYNC_HI,
    VIDMODE_MON_VREFRESH_LO,
    VIDMODE_MON_VREFRESH_HI
} VidModeSelectMonitor;

typedef union {
    const void *ptr;
    int i;
    float f;
} vidMonitorValue;

extern Bool VidModeExtensionInit(ScreenPtr pScreen);

extern Bool VidModeGetCurrentModeline(ScreenPtr pScreen, DisplayModePtr *mode,
                                      int *dotClock);
extern Bool VidModeGetFirstModeline(ScreenPtr pScreen, DisplayModePtr *mode,
                                    int *dotClock);
extern Bool VidModeGetNextModeline(ScreenPtr pScreen, DisplayModePtr *mode,
                                   int *dotClock);
extern Bool VidModeDeleteModeline(ScreenPtr pScreen, DisplayModePtr mode);
extern Bool VidModeZoomViewport(ScreenPtr pScreen, int zoom);
extern Bool VidModeGetViewPort(ScreenPtr pScreen, int *x, int *y);
extern Bool VidModeSetViewPort(ScreenPtr pScreen, int x, int y);
extern Bool VidModeSwitchMode(ScreenPtr pScreen, DisplayModePtr mode);
extern Bool VidModeLockZoom(ScreenPtr pScreen, Bool lock);
extern int VidModeGetNumOfClocks(ScreenPtr pScreen, Bool *progClock);
extern Bool VidModeGetClocks(ScreenPtr pScreen, int *Clocks);
extern ModeStatus VidModeCheckModeForMonitor(ScreenPtr pScreen,
                                             DisplayModePtr mode);
extern ModeStatus VidModeCheckModeForDriver(ScreenPtr pScreen,
                                            DisplayModePtr mode);
extern void VidModeSetCrtcForMode(ScreenPtr pScreen, DisplayModePtr mode);
extern Bool VidModeAddModeline(ScreenPtr pScreen, DisplayModePtr mode);
extern int VidModeGetDotClock(ScreenPtr pScreen, int Clock);
extern int VidModeGetNumOfModes(ScreenPtr pScreen);
extern Bool VidModeSetGamma(ScreenPtr pScreen, float red, float green,
                            float blue);
extern Bool VidModeGetGamma(ScreenPtr pScreen, float *red, float *green,
                            float *blue);
extern vidMonitorValue VidModeGetMonitorValue(ScreenPtr pScreen,
                                              int valtyp, int indx);
extern Bool VidModeSetGammaRamp(ScreenPtr, int, CARD16 *, CARD16 *,
                                CARD16 *);
extern Bool VidModeGetGammaRamp(ScreenPtr, int, CARD16 *, CARD16 *,
                                CARD16 *);
extern int VidModeGetGammaRampSize(ScreenPtr pScreen);

#endif
