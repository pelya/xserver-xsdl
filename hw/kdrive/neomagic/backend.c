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

#ifdef HAVE_CONFIG_H
#include <kdrive-config.h>
#endif
#include "backend.h"

Bool
backendInitialize(KdCardInfo *card, BackendInfo *backend)
{
    Bool success = FALSE;

#ifdef KDRIVEVESA
    if (!success && vesaInitialize(card, &backend->priv.vesa)) {
        success = TRUE;
        backend->type = VESA;
        backend->cardfini = vesaCardFini;
        backend->scrfini = vesaScreenFini;
        backend->initScreen = vesaInitScreen;
        backend->finishInitScreen = vesaFinishInitScreen;
        backend->createRes = vesaCreateResources;
        backend->preserve = vesaPreserve;
        backend->restore = vesaRestore;
        backend->dpms = vesaDPMS;
        backend->enable = vesaEnable;
        backend->disable = vesaDisable;
        backend->getColors = vesaGetColors;
        backend->putColors = vesaPutColors;
    }
#endif
#ifdef KDRIVEFBDEV
    if (!success && fbdevInitialize(card, &backend->priv.fbdev)) {
        success = TRUE;
        backend->type = FBDEV;
        backend->cardfini = fbdevCardFini;
        backend->scrfini = fbdevScreenFini;
        backend->initScreen = fbdevInitScreen;
        backend->finishInitScreen = fbdevFinishInitScreen;
        backend->createRes = fbdevCreateResources;
        backend->preserve = fbdevPreserve;
        backend->restore = fbdevRestore;
        backend->dpms = fbdevDPMS;
        backend->enable = fbdevEnable;
        backend->disable = fbdevDisable;
        backend->getColors = fbdevGetColors;
        backend->putColors = fbdevPutColors;
    }
#endif
    return success;
}

Bool
backendScreenInitialize(KdScreenInfo *screen, BackendScreen *backendScreen,
                        BackendInfo *backendCard)
{
    Bool success = FALSE;

#ifdef KDRIVEFBDEV
    if (backendCard->type == FBDEV) {
        screen->card->driver = &backendCard->priv.fbdev;
        success = fbdevScreenInitialize(screen, &backendScreen->fbdev);
        screen->memory_size = backendCard->priv.fbdev.fix.smem_len;
        screen->off_screen_base = backendCard->priv.fbdev.var.yres_virtual
                                * screen->fb[0].byteStride;
    }
#endif
#ifdef KDRIVEVESA
    if (backendCard->type == VESA) {
		screen->card->driver = &backendCard->priv.vesa;
        if (screen->fb[0].depth == 0) {
            screen->fb[0].depth = 16;
        }
        success = vesaScreenInitialize(screen, &backendScreen->vesa);
    }
#endif
    return success;
}
