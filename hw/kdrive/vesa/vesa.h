/* 
Copyright (c) 2000 by Juliusz Chroboczek
 
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
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
/* $XFree86$ */

#ifndef _VESA_H_
#define _VESA_H_

#include "kdrive.h"
#include <sys/vm86.h>
#include "vbe.h"

typedef struct _VesaPriv {
    int mode;
    VbeInfoPtr vi;
    VbeInfoBlock *vib;
    VbeModeInfoBlock *vmib;
    void *fb;
} VesaPrivRec, *VesaPrivPtr;

extern int vesa_video_mode;
extern Bool vesa_force_mode;

Bool vesaListModes(void);
Bool vesaInitialize(KdCardInfo *card, VesaPrivPtr priv);
Bool vesaCardInit(KdCardInfo *card);
Bool vesaScreenInit(KdScreenInfo *screen);
Bool vesaInitScreen(ScreenPtr pScreen);
void vesaEnable(ScreenPtr pScreen);
void vesaDisable(ScreenPtr pScreen);
void vesaPreserve(KdCardInfo *card);
void vesaRestore(KdCardInfo *card);
void vesaCardFini(KdCardInfo *card);
void vesaScreenFini(KdScreenInfo *screen);
void vesaPutColors (ScreenPtr pScreen, int fb, int n, xColorItem *pdefs);
void vesaGetColors (ScreenPtr pScreen, int fb, int n, xColorItem *pdefs);

#endif _VESA_H_
