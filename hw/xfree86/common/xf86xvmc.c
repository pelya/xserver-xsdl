/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86xvmc.c,v 1.3 2001/04/01 14:00:08 tsi Exp $ */

#include "misc.h"
#include "xf86.h"
#include "xf86_OSproc.h"

#include "X.h"
#include "Xproto.h"
#include "scrnintstr.h"
#include "resource.h"
#include "dixstruct.h"

#ifdef XFree86LOADER
#include "xvmodproc.h"
#endif

#include "xf86xvmc.h"

#ifdef XFree86LOADER
int (*XvMCScreenInitProc)(ScreenPtr, int, XvMCAdaptorPtr) = NULL;
#else
int (*XvMCScreenInitProc)(ScreenPtr, int, XvMCAdaptorPtr) = XvMCScreenInit;
#endif


typedef struct {
  CloseScreenProcPtr CloseScreen; 
  int num_adaptors;
  XF86MCAdaptorPtr *adaptors;
  XvMCAdaptorPtr dixinfo;
} xf86XvMCScreenRec, *xf86XvMCScreenPtr;

static unsigned long XF86XvMCGeneration = 0;
static int XF86XvMCScreenIndex = -1;

#define XF86XVMC_GET_PRIVATE(pScreen) \
   (xf86XvMCScreenPtr)((pScreen)->devPrivates[XF86XvMCScreenIndex].ptr)


static int 
xf86XvMCCreateContext (
  XvPortPtr pPort,
  XvMCContextPtr pContext,
  int *num_priv,
  CARD32 **priv
)
{
    xf86XvMCScreenPtr pScreenPriv = XF86XVMC_GET_PRIVATE(pContext->pScreen);
    ScrnInfoPtr pScrn = xf86Screens[pContext->pScreen->myNum];

    pContext->port_priv = (XvPortRecPrivatePtr)(pPort->devPriv.ptr);

    return (*pScreenPriv->adaptors[pContext->adapt_num]->CreateContext)(
		pScrn, pContext, num_priv, priv);
}

static void 
xf86XvMCDestroyContext ( XvMCContextPtr pContext)
{
    xf86XvMCScreenPtr pScreenPriv = XF86XVMC_GET_PRIVATE(pContext->pScreen);
    ScrnInfoPtr pScrn = xf86Screens[pContext->pScreen->myNum];

    (*pScreenPriv->adaptors[pContext->adapt_num]->DestroyContext)(
                				pScrn, pContext);
}

static int 
xf86XvMCCreateSurface (
  XvMCSurfacePtr pSurface,
  int *num_priv,
  CARD32 **priv
)
{
    XvMCContextPtr pContext = pSurface->context;
    xf86XvMCScreenPtr pScreenPriv = XF86XVMC_GET_PRIVATE(pContext->pScreen);
    ScrnInfoPtr pScrn = xf86Screens[pContext->pScreen->myNum];

    return (*pScreenPriv->adaptors[pContext->adapt_num]->CreateSurface)(
                pScrn, pSurface, num_priv, priv);
}

static void 
xf86XvMCDestroySurface (XvMCSurfacePtr pSurface)
{
    XvMCContextPtr pContext = pSurface->context;
    xf86XvMCScreenPtr pScreenPriv = XF86XVMC_GET_PRIVATE(pContext->pScreen);
    ScrnInfoPtr pScrn = xf86Screens[pContext->pScreen->myNum];

    (*pScreenPriv->adaptors[pContext->adapt_num]->DestroySurface)(
                                                pScrn, pSurface);
}

static int 
xf86XvMCCreateSubpicture (
  XvMCSubpicturePtr pSubpicture,
  int *num_priv,
  CARD32 **priv
)
{
    XvMCContextPtr pContext = pSubpicture->context;
    xf86XvMCScreenPtr pScreenPriv = XF86XVMC_GET_PRIVATE(pContext->pScreen);
    ScrnInfoPtr pScrn = xf86Screens[pContext->pScreen->myNum];

    return (*pScreenPriv->adaptors[pContext->adapt_num]->CreateSubpicture)(
                                  pScrn, pSubpicture, num_priv, priv);
}

static void
xf86XvMCDestroySubpicture (XvMCSubpicturePtr pSubpicture)
{
    XvMCContextPtr pContext = pSubpicture->context;
    xf86XvMCScreenPtr pScreenPriv = XF86XVMC_GET_PRIVATE(pContext->pScreen);
    ScrnInfoPtr pScrn = xf86Screens[pContext->pScreen->myNum];

    (*pScreenPriv->adaptors[pContext->adapt_num]->DestroySubpicture)(
                                                pScrn, pSubpicture);
}


static Bool
xf86XvMCCloseScreen (int i, ScreenPtr pScreen)
{
    xf86XvMCScreenPtr pScreenPriv = XF86XVMC_GET_PRIVATE(pScreen);

    pScreen->CloseScreen = pScreenPriv->CloseScreen;

    xfree(pScreenPriv->dixinfo);
    xfree(pScreenPriv);

    return (*pScreen->CloseScreen)(i, pScreen);
}

Bool xf86XvMCScreenInit(
   ScreenPtr pScreen, 
   int num_adaptors,
   XF86MCAdaptorPtr *adaptors
)
{
   XvMCAdaptorPtr pAdapt;
   xf86XvMCScreenPtr pScreenPriv;
   XvScreenPtr pxvs = 
	(XvScreenPtr)(pScreen->devPrivates[XF86XvScreenIndex].ptr);

   int i, j;

   if(!XvMCScreenInitProc) return FALSE;

   if(XF86XvMCGeneration != serverGeneration) {
	if((XF86XvMCScreenIndex = AllocateScreenPrivateIndex()) < 0)
	   return FALSE;
	XF86XvMCGeneration = serverGeneration;
   }

   if(!(pAdapt = xalloc(sizeof(XvMCAdaptorRec) * num_adaptors)))
	return FALSE;

   if(!(pScreenPriv = xalloc(sizeof(xf86XvMCScreenRec)))) {
	xfree(pAdapt);
	return FALSE;
   }

   pScreen->devPrivates[XF86XvMCScreenIndex].ptr = (pointer)pScreenPriv; 

   pScreenPriv->CloseScreen = pScreen->CloseScreen;
   pScreen->CloseScreen = xf86XvMCCloseScreen;

   pScreenPriv->num_adaptors = num_adaptors;
   pScreenPriv->adaptors = adaptors;
   pScreenPriv->dixinfo = pAdapt;

   for(i = 0; i < num_adaptors; i++) {
	pAdapt[i].xv_adaptor = NULL;
	for(j = 0; j < pxvs->nAdaptors; j++) {
	   if(!strcmp((*adaptors)->name, pxvs->pAdaptors[j].name)) {
		pAdapt[i].xv_adaptor = &(pxvs->pAdaptors[j]); 
		break;
	   }
	}
	if(!pAdapt[i].xv_adaptor) {
	    /* no adaptor by that name */
	    xfree(pAdapt);
	    return FALSE;
	}
	pAdapt[i].num_surfaces = (*adaptors)->num_surfaces;
	pAdapt[i].surfaces = (XvMCSurfaceInfoPtr*)((*adaptors)->surfaces);
	pAdapt[i].num_subpictures = (*adaptors)->num_subpictures;
	pAdapt[i].subpictures = (XvImagePtr*)((*adaptors)->subpictures);
	pAdapt[i].CreateContext = xf86XvMCCreateContext;
	pAdapt[i].DestroyContext = xf86XvMCDestroyContext;
	pAdapt[i].CreateSurface = xf86XvMCCreateSurface;
	pAdapt[i].DestroySurface = xf86XvMCDestroySurface;
	pAdapt[i].CreateSubpicture = xf86XvMCCreateSubpicture;
	pAdapt[i].DestroySubpicture = xf86XvMCDestroySubpicture;
	adaptors++;
   }

   if(Success != (*XvMCScreenInitProc)(pScreen, num_adaptors, pAdapt))
	return FALSE;

   return TRUE;
}
