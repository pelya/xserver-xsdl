/* $XFree86$ */

#include "X.h"
#include "Xmd.h"
#include "Xproto.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "regionstr.h"
#define PSZ 8
#include "cfb.h"
#undef PSZ
#include "cfb32.h"
#include "cfb8_32.h"
#include "mi.h"


RegionPtr 
cfb8_32CopyPlane(
    DrawablePtr pSrc,
    DrawablePtr	pDst,
    GCPtr pGC,
    int srcx, int srcy,
    int width, int height,
    int dstx, int dsty,
    unsigned long bitPlane
){
   /* There's actually much more to it than this */

   if((pDst->bitsPerPixel == 8) && (pSrc->bitsPerPixel != 32)){
	return(cfbCopyPlane(pSrc, pDst,
            pGC, srcx, srcy, width, height, dstx, dsty, bitPlane));
   } 


   return(miCopyPlane (pSrc, pDst,
            pGC, srcx, srcy, width, height, dstx, dsty, bitPlane));
}
