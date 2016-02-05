
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

extern Bool xf86VidModeExtensionInit(ScreenPtr pScreen);

extern Bool xf86VidModeGetCurrentModeline(ScreenPtr pScreen, DisplayModePtr *mode,
                                          int *dotClock);
extern Bool xf86VidModeGetFirstModeline(ScreenPtr pScreen, DisplayModePtr *mode,
                                        int *dotClock);
extern Bool xf86VidModeGetNextModeline(ScreenPtr pScreen, DisplayModePtr *mode,
                                       int *dotClock);
extern Bool xf86VidModeDeleteModeline(ScreenPtr pScreen, DisplayModePtr mode);
extern Bool xf86VidModeZoomViewport(ScreenPtr pScreen, int zoom);
extern Bool xf86VidModeGetViewPort(ScreenPtr pScreen, int *x, int *y);
extern Bool xf86VidModeSetViewPort(ScreenPtr pScreen, int x, int y);
extern Bool xf86VidModeSwitchMode(ScreenPtr pScreen, DisplayModePtr mode);
extern Bool xf86VidModeLockZoom(ScreenPtr pScreen, Bool lock);
extern int xf86VidModeGetNumOfClocks(ScreenPtr pScreen, Bool *progClock);
extern Bool xf86VidModeGetClocks(ScreenPtr pScreen, int *Clocks);
extern ModeStatus xf86VidModeCheckModeForMonitor(ScreenPtr pScreen,
                                                 DisplayModePtr mode);
extern ModeStatus xf86VidModeCheckModeForDriver(ScreenPtr pScreen,
                                                DisplayModePtr mode);
extern void xf86VidModeSetCrtcForMode(ScreenPtr pScreen, DisplayModePtr mode);
extern Bool xf86VidModeAddModeline(ScreenPtr pScreen, DisplayModePtr mode);
extern int xf86VidModeGetDotClock(ScreenPtr pScreen, int Clock);
extern int xf86VidModeGetNumOfModes(ScreenPtr pScreen);
extern Bool xf86VidModeSetGamma(ScreenPtr pScreen, float red, float green,
                                float blue);
extern Bool xf86VidModeGetGamma(ScreenPtr pScreen, float *red, float *green,
                                float *blue);
extern vidMonitorValue xf86VidModeGetMonitorValue(ScreenPtr pScreen,
                                                  int valtyp, int indx);
extern Bool xf86VidModeSetGammaRamp(ScreenPtr, int, CARD16 *, CARD16 *,
                                    CARD16 *);
extern Bool xf86VidModeGetGammaRamp(ScreenPtr, int, CARD16 *, CARD16 *,
                                    CARD16 *);
extern int xf86VidModeGetGammaRampSize(ScreenPtr pScreen);

#endif
