/* $XFree86$ */

#include "xf86RamDac.h"
#include "xf86cmap.h"

void RamDacGetRecPrivate(void);
Bool RamDacGetRec(ScrnInfoPtr pScrn);
int  RamDacGetScreenIndex(void);
void RamDacLoadPalette(ScrnInfoPtr pScrn, int numColors, int *indices,
    			LOCO *colors, VisualPtr pVisual);
