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

#define VESA_TEXT_SAVE	(64*1024)

typedef struct _VesaMode {
    int mode;
    VbeModeInfoBlock vmib;
} VesaModeRec, *VesaModePtr;

typedef struct _VesaCardPriv {
    VbeInfoPtr vi;
    VbeInfoBlock *vib;
    VesaModePtr modes;
    int	nmode;
    char text[VESA_TEXT_SAVE];
} VesaCardPrivRec, *VesaCardPrivPtr;

#define VESA_LINEAR	0
#define VESA_WINDOWED	1
#define VESA_PLANAR	2
typedef struct _VesaScreenPriv {
    VesaModePtr mode;
    Bool	shadow;
    int		mapping;
    void *fb;
} VesaScreenPrivRec, *VesaScreenPrivPtr;

extern int vesa_video_mode;
extern Bool vesa_force_mode;

Bool vesaListModes(void);
Bool vesaInitialize(KdCardInfo *card, VesaCardPrivPtr priv);
Bool vesaCardInit(KdCardInfo *card);
Bool vesaInitialize (KdCardInfo *card, VesaCardPrivPtr priv);
Bool vesaScreenInitialize (KdScreenInfo *screen, VesaScreenPrivPtr pscr);
Bool vesaScreenInit(KdScreenInfo *screen);    
Bool vesaInitScreen(ScreenPtr pScreen);
Bool vesaEnable(ScreenPtr pScreen);
void vesaDisable(ScreenPtr pScreen);
void vesaPreserve(KdCardInfo *card);
void vesaRestore(KdCardInfo *card);
void vesaCardFini(KdCardInfo *card);
void vesaScreenFini(KdScreenInfo *screen);
void vesaPutColors (ScreenPtr pScreen, int fb, int n, xColorItem *pdefs);
void vesaGetColors (ScreenPtr pScreen, int fb, int n, xColorItem *pdefs);
int vesaProcessArgument (int argc, char **argv, int i);

#endif _VESA_H_
