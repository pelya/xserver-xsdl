/*
 * Generic card driving functions.
 * Essentially, cascades calls to fbdev and vesa to initialize cards that
 * do not have hardware-specific routines yet. Copied from the ati driver.
 *
 * Copyright (c) 2004 Brent Cook <busterb@mail.utexas.edu>
 *
 * This code is licensed under the GPL.  See the file COPYING in the root
 * directory of the sources for details.
 */

#ifndef _BACKEND_H_
#define _BACKEND_H_
#include "kdrive.h"

#ifdef KDRIVEFBDEV
#include <fbdev.h>
#endif
#ifdef KDRIVEVESA
#include <vesa.h>
#endif

typedef enum _BackendType {VESA, FBDEV} BackendType;

typedef struct _BackendInfo {
    // backend info
    BackendType type;

    // backend internal structures
    union {
#ifdef KDRIVEFBDEV
        FbdevPriv fbdev;
#endif
#ifdef KDRIVEVESA
        VesaCardPrivRec vesa;
#endif
    } priv;

    // pointers to helper functions for this backend
    void (*cardfini)(KdCardInfo *);
    void (*scrfini)(KdScreenInfo *);
    Bool (*initScreen)(ScreenPtr);
    Bool (*finishInitScreen)(ScreenPtr pScreen);
    Bool (*createRes)(ScreenPtr);
    void (*preserve)(KdCardInfo *);
    void (*restore)(KdCardInfo *);
    Bool (*dpms)(ScreenPtr, int);
    Bool (*enable)(ScreenPtr);
    void (*disable)(ScreenPtr);
    void (*getColors)(ScreenPtr, int, int, xColorItem *);
    void (*putColors)(ScreenPtr, int, int, xColorItem *);
} BackendInfo;

typedef union _BackendScreen {
#ifdef KDRIVEFBDEV
    FbdevScrPriv fbdev;
#endif
#ifdef KDRIVEVESA
    VesaScreenPrivRec vesa;
#endif
} BackendScreen;

Bool
backendInitialize(KdCardInfo *card, BackendInfo *backend);

Bool
backendScreenInitialize(KdScreenInfo *screen, BackendScreen *backendScreen,
                        BackendInfo *backendCard);

#endif
