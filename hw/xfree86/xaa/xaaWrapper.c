#include "X.h"
#include "Xproto.h"
#include "scrnintstr.h"
#include "gcstruct.h"
#include "glyphstr.h"
#include "window.h"
#include "windowstr.h"
#include "picture.h"
#include "picturestr.h"
#include "colormapst.h"
#include "xaa.h"
#include "xaalocal.h"
#include "xaaWrapper.h"

void XAASync(ScreenPtr pScreen);

/* #include "render.h" */

#if 0
#define COND(pDraw) \
        ((pDraw)->depth \
                      != (xaaWrapperGetScrPriv(((DrawablePtr)(pDraw))->pScreen))->depth)
#endif
#define COND(pDraw) 1

#if 0
static Bool xaaWrapperPreCreateGC(GCPtr pGC);
#endif
static Bool xaaWrapperCreateGC(GCPtr pGC);
static void xaaWrapperValidateGC(GCPtr pGC, unsigned long changes, DrawablePtr pDraw);
static void xaaWrapperDestroyGC(GCPtr pGC);
static void xaaWrapperChangeGC (GCPtr pGC, unsigned long   mask);
static void xaaWrapperCopyGC (GCPtr pGCSrc, unsigned long   mask, GCPtr pGCDst);
static void xaaWrapperChangeClip (GCPtr pGC, int type, pointer pvalue, int nrects);

static void xaaWrapperCopyClip(GCPtr pgcDst, GCPtr pgcSrc);
static void xaaWrapperDestroyClip(GCPtr pGC);

#if 0
static void xaaWrapperFillSpans(DrawablePtr pDraw, GC *pGC, int nInit,
			DDXPointPtr pptInit, int *pwidthInit, int fSorted);
static void xaaWrapperSetSpans(DrawablePtr pDraw, GCPtr	pGC, char *pcharsrc,
		       DDXPointPtr pptInit, int	*pwidthInit, int nspans,
		       int fSorted);
static void xaaWrapperPutImage(DrawablePtr pDraw, GCPtr	pGC, int depth, int x, int y,
		       int w, int h,int	leftPad, int format, char *pImage);
static RegionPtr xaaWrapperCopyPlane(DrawablePtr pSrc,
			     DrawablePtr pDst, GCPtr pGC,int srcx, int srcy,
			     int width, int height, int	dstx, int dsty,
			     unsigned long bitPlane);
static void xaaWrapperPolyPoint(DrawablePtr pDraw, GCPtr pGC, int mode, int npt,
			xPoint *pptInit);
static void xaaWrapperPolylines(DrawablePtr pDraw, GCPtr pGC, int mode,
			int npt, DDXPointPtr pptInit);
static void xaaWrapperPolySegment(DrawablePtr pDraw, GCPtr pGC, int nseg,
			  xSegment *pSeg);
static void xaaWrapperPolyRectangle(DrawablePtr  pDraw, GCPtr pGC, int nRects,
			    xRectangle  *pRects);
static void xaaWrapperPolyArc( DrawablePtr pDraw, GCPtr	pGC, int narcs, xArc *parcs);
static void xaaWrapperFillPolygon(DrawablePtr pDraw, GCPtr pGC, int shape,
			  int mode, int count, DDXPointPtr pptInit);
static void xaaWrapperPolyFillRect(DrawablePtr pDraw, GCPtr pGC, int nRectsInit, 
			   xRectangle *pRectsInit);
static RegionPtr xaaWrapperCopyArea(DrawablePtr pSrc, DrawablePtr pDst, GC *pGC,
			    int srcx, int srcy, int width, int height,
			    int dstx, int dsty);
static void xaaWrapperPolyFillArc(DrawablePtr pDraw, GCPtr pGC, int narcs,
			  xArc *parcs);
static int xaaWrapperPolyText8(DrawablePtr pDraw, GCPtr	pGC, int x, int	y, int count,
		       char *chars);
static int xaaWrapperPolyText16(DrawablePtr pDraw, GCPtr pGC, int x, int y,
			int count, unsigned short *chars);
static void xaaWrapperImageText8(DrawablePtr pDraw, GCPtr pGC, int x, 
			 int y, int count, char	*chars);
static void xaaWrapperImageText16(DrawablePtr pDraw, GCPtr pGC, int x, int y,
			  int count, unsigned short *chars);
static void xaaWrapperImageGlyphBlt(DrawablePtr pDraw, GCPtr pGC, int x, int y,
			    unsigned int nglyph, CharInfoPtr *ppci,
			    pointer pglyphBase);
static void xaaWrapperPolyGlyphBlt(DrawablePtr pDraw, GCPtr pGC, int x, int y,
			   unsigned int nglyph, CharInfoPtr *ppci,
			   pointer pglyphBase);
static void xaaWrapperPushPixels(GCPtr pGC, PixmapPtr pBitMap, DrawablePtr pDraw,
			 int	dx, int dy, int xOrg, int yOrg);
#endif
static void
xaaWrapperComposite (CARD8 op, PicturePtr pSrc, PicturePtr pMask, PicturePtr pDst,
	     INT16 xSrc, INT16 ySrc, INT16 xMask, INT16 yMask,
	     INT16 xDst, INT16 yDst, CARD16 width, CARD16 height);
static void
xaaWrapperGlyphs (CARD8 op, PicturePtr pSrc, PicturePtr pDst,
	  PictFormatPtr maskFormat, INT16 xSrc, INT16 ySrc, int nlist,
	  GlyphListPtr list, GlyphPtr *glyphs);


typedef struct {
    CloseScreenProcPtr		CloseScreen;
    CreateScreenResourcesProcPtr CreateScreenResources;
    CreateWindowProcPtr		CreateWindow;
    CopyWindowProcPtr		CopyWindow;
    PaintWindowProcPtr		PaintWindowBackground;
    PaintWindowProcPtr		PaintWindowBorder;
    WindowExposuresProcPtr	WindowExposures;
    CreateGCProcPtr		CreateGC;
    CreateColormapProcPtr	CreateColormap;
    DestroyColormapProcPtr	DestroyColormap;
    InstallColormapProcPtr	InstallColormap;
    UninstallColormapProcPtr	UninstallColormap;
    ListInstalledColormapsProcPtr ListInstalledColormaps;
    StoreColorsProcPtr		StoreColors;
#ifdef RENDER
    CompositeProcPtr		Composite;
    GlyphsProcPtr		Glyphs;
#endif    

    CloseScreenProcPtr		wrapCloseScreen;
    CreateScreenResourcesProcPtr wrapCreateScreenResources;
    CreateWindowProcPtr		wrapCreateWindow;
    CopyWindowProcPtr		wrapCopyWindow;
    PaintWindowProcPtr		wrapPaintWindowBackground;
    PaintWindowProcPtr		wrapPaintWindowBorder;
    WindowExposuresProcPtr	wrapWindowExposures;
    CreateGCProcPtr		wrapCreateGC;
    CreateColormapProcPtr	wrapCreateColormap;
    DestroyColormapProcPtr	wrapDestroyColormap;
    InstallColormapProcPtr	wrapInstallColormap;
    UninstallColormapProcPtr	wrapUninstallColormap;
    ListInstalledColormapsProcPtr wrapListInstalledColormaps;
    StoreColorsProcPtr		wrapStoreColors;
#ifdef RENDER
    CompositeProcPtr		wrapComposite;
    GlyphsProcPtr		wrapGlyphs;
#endif    
    int depth;
} xaaWrapperScrPrivRec, *xaaWrapperScrPrivPtr;

#define xaaWrapperGetScrPriv(s)	((xaaWrapperScrPrivPtr)( \
				 (xaaWrapperScrPrivateIndex != -1) \
                          ? (s)->devPrivates[xaaWrapperScrPrivateIndex].ptr\
				: NULL))
#define xaaWrapperScrPriv(s)     xaaWrapperScrPrivPtr pScrPriv = xaaWrapperGetScrPriv(s)

#define wrap(priv,real,mem,func) {\
    priv->mem = real->mem; \
    real->mem = func; \
}

#if 0
#define wrap_pre(priv,real,real_func,mem,func) {\
    priv->mem = real->real_func; \
    real->real_func = func; \
}
#endif

#define get(priv,real,func,wrap) \
    priv->wrap = real->func;

#define unwrap(priv,real,mem) {\
    real->mem = priv->mem; \
}

#if 0
#define unwrap_pre(priv,real,real_func,mem) {\
    real->real_func = priv->mem; \
}
#endif

typedef struct _xaaWrapperGCPriv {
    GCOps   *ops;
    Bool  wrap;
    GCFuncs *funcs;
    GCOps   *wrapops;
} xaaWrapperGCPrivRec, *xaaWrapperGCPrivPtr;

#define xaaWrapperGetGCPriv(pGC) ((xaaWrapperGCPrivPtr) \
				      (pGC)->devPrivates[xaaWrapperGCPrivateIndex].ptr)
#define xaaWrapperGCPriv(pGC)   xaaWrapperGCPrivPtr  pGCPriv = xaaWrapperGetGCPriv(pGC)


static int xaaWrapperScrPrivateIndex = -1;
static int xaaWrapperGCPrivateIndex = -1;
static int xaaWrapperGeneration = -1;

static Bool
xaaWrapperCreateScreenResources(ScreenPtr pScreen)
{
    xaaWrapperScrPriv(pScreen);
    Bool		ret;
    
    unwrap (pScrPriv,pScreen, CreateScreenResources);
    ret = pScreen->CreateScreenResources(pScreen);
    wrap(pScrPriv,pScreen,CreateScreenResources,xaaWrapperCreateScreenResources);
    return ret;
}

static Bool
xaaWrapperCloseScreen (int iScreen, ScreenPtr pScreen)
{
    xaaWrapperScrPriv(pScreen);
    Bool		ret;

    unwrap (pScrPriv,pScreen, CloseScreen);
    ret = pScreen->CloseScreen(iScreen,pScreen);
    return TRUE;
}

static Bool
xaaWrapperCreateWindow(WindowPtr pWin)
{
    xaaWrapperScrPriv(pWin->drawable.pScreen);
    Bool ret;
    
    unwrap (pScrPriv, pWin->drawable.pScreen, CreateWindow);
    if (COND(&pWin->drawable))
	pWin->drawable.pScreen->CreateWindow
	    = pScrPriv->wrapCreateWindow;
    ret = pWin->drawable.pScreen->CreateWindow(pWin);
    wrap(pScrPriv, pWin->drawable.pScreen, CreateWindow, xaaWrapperCreateWindow);

    return ret;
}

static void
xaaWrapperCopyWindow(WindowPtr	pWin,
	     DDXPointRec	ptOldOrg,
	     RegionPtr	prgnSrc)
{
    ScreenPtr		pScreen = pWin->drawable.pScreen;
    xaaWrapperScrPriv(pScreen);
    
    unwrap (pScrPriv, pScreen, CopyWindow);
#if 0
    if (COND(&pWin->drawable))
	pWin->drawable.pScreen->CopyWindow = pScrPriv->wrapCopyWindow;
#endif
    pScreen->CopyWindow(pWin, ptOldOrg, prgnSrc);
    wrap(pScrPriv, pScreen, CopyWindow, xaaWrapperCopyWindow);
}

static void
xaaWrapperWindowExposures (WindowPtr	pWin,
			  RegionPtr	prgn,
			  RegionPtr	other_exposed)
{
    xaaWrapperScrPriv(pWin->drawable.pScreen);

    unwrap (pScrPriv, pWin->drawable.pScreen, WindowExposures);
    if (COND(&pWin->drawable))
	pWin->drawable.pScreen->WindowExposures = pScrPriv->wrapWindowExposures;
    pWin->drawable.pScreen->WindowExposures(pWin, prgn, other_exposed);
    wrap(pScrPriv, pWin->drawable.pScreen, WindowExposures, xaaWrapperWindowExposures);
}

static void
xaaWrapperPaintWindow(WindowPtr pWin, RegionPtr pRegion, int what)
{
    xaaWrapperScrPriv(pWin->drawable.pScreen);

    switch (what) {
	case PW_BORDER:
	    unwrap (pScrPriv, pWin->drawable.pScreen, PaintWindowBorder);
	    if (COND(&pWin->drawable)) {
		pWin->drawable.pScreen->PaintWindowBorder
		    = pScrPriv->wrapPaintWindowBorder;
				XAASync(pWin->drawable.pScreen);
	    }
	    pWin->drawable.pScreen->PaintWindowBorder (pWin, pRegion, what);
	    wrap(pScrPriv, pWin->drawable.pScreen, PaintWindowBorder,
		 xaaWrapperPaintWindow);
	    break;
	case PW_BACKGROUND:
	    unwrap (pScrPriv, pWin->drawable.pScreen, PaintWindowBackground);
	    if (COND(&pWin->drawable)) {
		pWin->drawable.pScreen->PaintWindowBackground
		    = pScrPriv->wrapPaintWindowBackground;
		XAASync(pWin->drawable.pScreen);
	    }
	    pWin->drawable.pScreen->PaintWindowBackground (pWin, pRegion, what);
	    wrap(pScrPriv, pWin->drawable.pScreen, PaintWindowBackground,
		 xaaWrapperPaintWindow);
	    break;
    }

}

static Bool
xaaWrapperCreateColormap(ColormapPtr pmap)
{
    xaaWrapperScrPriv(pmap->pScreen);
    Bool		ret;
    unwrap(pScrPriv,pmap->pScreen, CreateColormap);
    ret = pmap->pScreen->CreateColormap(pmap);
    wrap(pScrPriv,pmap->pScreen,CreateColormap,xaaWrapperCreateColormap);
    
    return ret;
}

static void
xaaWrapperDestroyColormap(ColormapPtr pmap)
{
    xaaWrapperScrPriv(pmap->pScreen);
    unwrap(pScrPriv,pmap->pScreen, DestroyColormap);
    pmap->pScreen->DestroyColormap(pmap);
    wrap(pScrPriv,pmap->pScreen,DestroyColormap,xaaWrapperDestroyColormap);
}

static void
xaaWrapperStoreColors(ColormapPtr pmap, int nColors, xColorItem *pColors)
{
    xaaWrapperScrPriv(pmap->pScreen);
    unwrap(pScrPriv,pmap->pScreen, StoreColors);
    pmap->pScreen->StoreColors(pmap,nColors,pColors);
    wrap(pScrPriv,pmap->pScreen,StoreColors,xaaWrapperStoreColors);
}

static void
xaaWrapperInstallColormap(ColormapPtr pmap)
{
    xaaWrapperScrPriv(pmap->pScreen);
    
    unwrap(pScrPriv,pmap->pScreen, InstallColormap);
    pmap->pScreen->InstallColormap(pmap);
    wrap(pScrPriv,pmap->pScreen,InstallColormap,xaaWrapperInstallColormap);
}

static void
xaaWrapperUninstallColormap(ColormapPtr pmap)
{
    xaaWrapperScrPriv(pmap->pScreen);

    unwrap(pScrPriv,pmap->pScreen, UninstallColormap);
    pmap->pScreen->UninstallColormap(pmap);
    wrap(pScrPriv,pmap->pScreen,UninstallColormap,xaaWrapperUninstallColormap);
}

static int
xaaWrapperListInstalledColormaps(ScreenPtr pScreen, Colormap *pCmapIds)
{
    int			n;
    xaaWrapperScrPriv(pScreen);

    unwrap(pScrPriv,pScreen, ListInstalledColormaps);
    n = pScreen->ListInstalledColormaps(pScreen, pCmapIds);
    wrap (pScrPriv,pScreen,ListInstalledColormaps,xaaWrapperListInstalledColormaps);
    return n;
}

Bool
xaaSetupWrapper(ScreenPtr pScreen, XAAInfoRecPtr infoPtr, int depth, SyncFunc *func)
{
    Bool ret;
    xaaWrapperScrPrivPtr pScrPriv;
#ifdef RENDER
    PictureScreenPtr	ps = GetPictureScreenIfSet(pScreen);
#endif
    if (xaaWrapperGeneration != serverGeneration) {
	xaaWrapperScrPrivateIndex = AllocateScreenPrivateIndex ();
	if (xaaWrapperScrPrivateIndex == -1)
	    return FALSE;
	xaaWrapperGCPrivateIndex = AllocateGCPrivateIndex ();
	if (xaaWrapperGCPrivateIndex == -1)
	    return FALSE;
	xaaWrapperGeneration = serverGeneration;
    }

    if (!AllocateGCPrivate (pScreen, xaaWrapperGCPrivateIndex,
			    sizeof (xaaWrapperGCPrivRec)))
	return FALSE;

    pScrPriv = (xaaWrapperScrPrivPtr) xalloc (sizeof (xaaWrapperScrPrivRec));
    if (!pScrPriv)
	return FALSE;

    get (pScrPriv, pScreen, CloseScreen, wrapCloseScreen);
    get (pScrPriv, pScreen, CreateScreenResources, wrapCreateScreenResources);
    get (pScrPriv, pScreen, CreateWindow, wrapCreateWindow);
    get (pScrPriv, pScreen, CopyWindow, wrapCopyWindow);
    get (pScrPriv, pScreen, PaintWindowBorder, wrapPaintWindowBorder);
    get (pScrPriv, pScreen, PaintWindowBackground, wrapPaintWindowBackground);
    get (pScrPriv, pScreen, WindowExposures, wrapWindowExposures);
#if 0
    wrap_pre (pScrPriv, pScreen, CreateGC, wrapCreateGC, xaaWrapperPreCreateGC);
#else
    get (pScrPriv, pScreen, CreateGC, wrapCreateGC);
#endif
    get (pScrPriv, pScreen, CreateColormap, wrapCreateColormap);
    get (pScrPriv, pScreen, DestroyColormap, wrapDestroyColormap);
    get (pScrPriv, pScreen, InstallColormap, wrapInstallColormap);
    get (pScrPriv, pScreen, UninstallColormap, wrapUninstallColormap);
    get (pScrPriv, pScreen, ListInstalledColormaps, wrapListInstalledColormaps);
    get (pScrPriv, pScreen, StoreColors, wrapStoreColors);
#ifdef RENDER
    if (ps) {
	get (pScrPriv, ps, Glyphs, wrapGlyphs);
	get (pScrPriv, ps, Composite, wrapComposite);
    }
#endif
    if (!(ret = XAAInit(pScreen,infoPtr)))
	return FALSE;
    
    wrap (pScrPriv, pScreen, CloseScreen, xaaWrapperCloseScreen);
    wrap (pScrPriv, pScreen, CreateScreenResources,
	  xaaWrapperCreateScreenResources);
    wrap (pScrPriv, pScreen, CreateWindow, xaaWrapperCreateWindow);
    wrap (pScrPriv, pScreen, CopyWindow, xaaWrapperCopyWindow);
    wrap (pScrPriv, pScreen, PaintWindowBorder, xaaWrapperPaintWindow);
    wrap (pScrPriv, pScreen, PaintWindowBackground, xaaWrapperPaintWindow);
    wrap (pScrPriv, pScreen, WindowExposures, xaaWrapperWindowExposures);
    wrap (pScrPriv, pScreen, CreateGC, xaaWrapperCreateGC);
    wrap (pScrPriv, pScreen, CreateColormap, xaaWrapperCreateColormap);
    wrap (pScrPriv, pScreen, DestroyColormap, xaaWrapperDestroyColormap);
    wrap (pScrPriv, pScreen, InstallColormap, xaaWrapperInstallColormap);
    wrap (pScrPriv, pScreen, UninstallColormap, xaaWrapperUninstallColormap);
    wrap (pScrPriv, pScreen, ListInstalledColormaps,
	  xaaWrapperListInstalledColormaps);
    wrap (pScrPriv, pScreen, StoreColors, xaaWrapperStoreColors);

#ifdef RENDER
    if (ps) {
	wrap (pScrPriv, ps, Glyphs, xaaWrapperGlyphs);
	wrap (pScrPriv, ps, Composite, xaaWrapperComposite);
    }
#endif
    pScrPriv->depth = depth;
    pScreen->devPrivates[xaaWrapperScrPrivateIndex].ptr = (pointer) pScrPriv;

    *func = XAASync;
    
    return ret;
}

GCFuncs xaaWrapperGCFuncs = {
    xaaWrapperValidateGC, xaaWrapperChangeGC, xaaWrapperCopyGC,
    xaaWrapperDestroyGC, xaaWrapperChangeClip, xaaWrapperDestroyClip,
    xaaWrapperCopyClip
};

#if 0
GCOps xaaWrapperGCOps = {
    xaaWrapperFillSpans, xaaWrapperSetSpans, 
    xaaWrapperPutImage, xaaWrapperCopyArea, 
    xaaWrapperCopyPlane, xaaWrapperPolyPoint, 
    xaaWrapperPolylines, xaaWrapperPolySegment, 
    xaaWrapperPolyRectangle, xaaWrapperPolyArc, 
    xaaWrapperFillPolygon, xaaWrapperPolyFillRect, 
    xaaWrapperPolyFillArc, xaaWrapperPolyText8, 
    xaaWrapperPolyText16, xaaWrapperImageText8, 
    xaaWrapperImageText16, xaaWrapperImageGlyphBlt, 
    xaaWrapperPolyGlyphBlt, xaaWrapperPushPixels,
#ifdef NEED_LINEHELPER
    NULL,
#endif
    {NULL}		/* devPrivate */
};
#endif

#define XAAWRAPPER_GC_FUNC_PROLOGUE(pGC) \
    xaaWrapperGCPriv(pGC); \
    unwrap(pGCPriv, pGC, funcs); \
    if (pGCPriv->wrap) unwrap(pGCPriv, pGC, ops)

#define XAAWRAPPER_GC_FUNC_EPILOGUE(pGC) \
    wrap(pGCPriv, pGC, funcs, &xaaWrapperGCFuncs);  \
    if (pGCPriv->wrap) wrap(pGCPriv, pGC, ops, pGCPriv->wrapops)

#if 0
static Bool
xaaWrapperPreCreateGC(GCPtr pGC)
{
    ScreenPtr		pScreen = pGC->pScreen;
    xaaWrapperScrPriv(pScreen);
    xaaWrapperGCPriv(pGC);
    Bool ret;

    unwrap_pre (pScrPriv, pScreen, CreateGC, wrapCreateGC);
    ret = (*pScreen->CreateGC) (pGC);
    wrap_pre (pScrPriv, pScreen, CreateGC, wrapCreateGC, xaaWrapperPreCreateGC);

    return ret;
}
#endif

static Bool
xaaWrapperCreateGC(GCPtr pGC)
{
    ScreenPtr		pScreen = pGC->pScreen;
    xaaWrapperScrPriv(pScreen);
    xaaWrapperGCPriv(pGC);
    Bool ret;

    unwrap (pScrPriv, pScreen, CreateGC);
    if((ret = (*pScreen->CreateGC) (pGC))) {
	pGCPriv->wrap = FALSE;
	pGCPriv->funcs = pGC->funcs;
	pGCPriv->wrapops = pGC->ops;
	pGC->funcs = &xaaWrapperGCFuncs;
    }
    wrap (pScrPriv, pScreen, CreateGC, xaaWrapperCreateGC);

    return ret;
}

static void
xaaWrapperValidateGC(
   GCPtr         pGC,
   unsigned long changes,
   DrawablePtr   pDraw 
){
    XAAWRAPPER_GC_FUNC_PROLOGUE (pGC);
    (*pGC->funcs->ValidateGC)(pGC, changes, pDraw);

    if(COND(pDraw)) 
	pGCPriv->wrap = TRUE;
    
    XAAWRAPPER_GC_FUNC_EPILOGUE (pGC);
}

static void
xaaWrapperDestroyGC(GCPtr pGC)
{
    XAAWRAPPER_GC_FUNC_PROLOGUE (pGC);
    (*pGC->funcs->DestroyGC)(pGC);
    XAAWRAPPER_GC_FUNC_EPILOGUE (pGC);
}

static void
xaaWrapperChangeGC (
    GCPtr	    pGC,
    unsigned long   mask
){
    XAAWRAPPER_GC_FUNC_PROLOGUE (pGC);
    (*pGC->funcs->ChangeGC) (pGC, mask);
    XAAWRAPPER_GC_FUNC_EPILOGUE (pGC);
}

static void
xaaWrapperCopyGC (
    GCPtr	    pGCSrc, 
    unsigned long   mask,
    GCPtr	    pGCDst
){
    XAAWRAPPER_GC_FUNC_PROLOGUE (pGCDst);
    (*pGCDst->funcs->CopyGC) (pGCSrc, mask, pGCDst);
    XAAWRAPPER_GC_FUNC_EPILOGUE (pGCDst);
}

static void
xaaWrapperChangeClip (
    GCPtr   pGC,
    int		type,
    pointer	pvalue,
    int		nrects 
){
    XAAWRAPPER_GC_FUNC_PROLOGUE (pGC);
    (*pGC->funcs->ChangeClip) (pGC, type, pvalue, nrects);
    XAAWRAPPER_GC_FUNC_EPILOGUE (pGC);
}

static void
xaaWrapperCopyClip(GCPtr pgcDst, GCPtr pgcSrc)
{
    XAAWRAPPER_GC_FUNC_PROLOGUE (pgcDst);
    (* pgcDst->funcs->CopyClip)(pgcDst, pgcSrc);
    XAAWRAPPER_GC_FUNC_EPILOGUE (pgcDst);
}

static void
xaaWrapperDestroyClip(GCPtr pGC)
{
    XAAWRAPPER_GC_FUNC_PROLOGUE (pGC);
    (* pGC->funcs->DestroyClip)(pGC);
    XAAWRAPPER_GC_FUNC_EPILOGUE (pGC);
}

#if 0
#define XAAWRAPPER_GC_OP_PROLOGUE(pGC,pDraw) \
/*     xaaWrapperScrPriv(pDraw->pScreen); */\
    xaaWrapperGCPriv(pGC);  \
    GCFuncs *oldFuncs = pGC->funcs; \
    unwrap(pGCPriv, pGC, funcs);  \
    unwrap(pGCPriv, pGC, ops); \
	
#define XAAWRAPPER_GC_OP_EPILOGUE(pGC,pDraw) \
    wrap(pGCPriv, pGC, funcs, oldFuncs); \
    wrap(pGCPriv, pGC, ops, &xaaWrapperGCOps)

static void
xaaWrapperFillSpans(
    DrawablePtr pDraw,
    GC		*pGC,
    int		nInit,	
    DDXPointPtr pptInit,	
    int 	*pwidthInit,		
    int 	fSorted 
){
    XAAWRAPPER_GC_OP_PROLOGUE(pGC, pDraw);
    (*pGC->ops->FillSpans)(pDraw, pGC, nInit, pptInit, pwidthInit, fSorted);
    XAAWRAPPER_GC_OP_EPILOGUE(pGC, pDraw);
}

static void
xaaWrapperSetSpans(
    DrawablePtr		pDraw,
    GCPtr		pGC,
    char		*pcharsrc,
    DDXPointPtr 	pptInit,
    int			*pwidthInit,
    int			nspans,
    int			fSorted 
){
    XAAWRAPPER_GC_OP_PROLOGUE(pGC, pDraw);

    (*pGC->ops->SetSpans)(pDraw, pGC, pcharsrc, pptInit, 
			  pwidthInit, nspans, fSorted);
    XAAWRAPPER_GC_OP_EPILOGUE(pGC, pDraw);
}


static void
xaaWrapperPutImage(
    DrawablePtr pDraw,
    GCPtr	pGC,
    int		depth, 
    int x, int y, int w, int h,
    int		leftPad,
    int		format,
    char 	*pImage 
){
    XAAWRAPPER_GC_OP_PROLOGUE(pGC, pDraw);
    (*pGC->ops->PutImage)(pDraw, pGC, depth, x, y, w, h, 
		leftPad, format, pImage);
    XAAWRAPPER_GC_OP_EPILOGUE(pGC, pDraw);
}

static RegionPtr
xaaWrapperCopyArea(
    DrawablePtr pSrc,
    DrawablePtr pDst,
    GC *pGC,
    int srcx, int srcy,
    int width, int height,
    int dstx, int dsty 
){
    RegionPtr ret;
    XAAWRAPPER_GC_OP_PROLOGUE(pGC, pDst);
    ret = (*pGC->ops->CopyArea)(pSrc, pDst,
            pGC, srcx, srcy, width, height, dstx, dsty);
    XAAWRAPPER_GC_OP_EPILOGUE(pGC, pDst);

    return ret;
}


static RegionPtr
xaaWrapperCopyPlane(
    DrawablePtr	pSrc,
    DrawablePtr	pDst,
    GCPtr pGC,
    int	srcx, int srcy,
    int	width, int height,
    int	dstx, int dsty,
    unsigned long bitPlane 
){
    RegionPtr ret;
    XAAWRAPPER_GC_OP_PROLOGUE(pGC, pDst);
    ret = (*pGC->ops->CopyPlane)(pSrc, pDst,
	       pGC, srcx, srcy, width, height, dstx, dsty, bitPlane);
    XAAWRAPPER_GC_OP_EPILOGUE(pGC, pDst);
    return ret;
}

static void
xaaWrapperPolyPoint(
    DrawablePtr pDraw,
    GCPtr pGC,
    int mode,
    int npt,
    xPoint *pptInit 
){
    XAAWRAPPER_GC_OP_PROLOGUE(pGC, pDraw);
    (*pGC->ops->PolyPoint)(pDraw, pGC, mode, npt, pptInit);
    XAAWRAPPER_GC_OP_EPILOGUE(pGC, pDraw);
}

static void
xaaWrapperPolylines(
    DrawablePtr pDraw,
    GCPtr	pGC,
    int		mode,		
    int		npt,		
    DDXPointPtr pptInit 
){
    XAAWRAPPER_GC_OP_PROLOGUE(pGC, pDraw);
    (*pGC->ops->Polylines)(pDraw, pGC, mode, npt, pptInit);
    XAAWRAPPER_GC_OP_EPILOGUE(pGC, pDraw);
}

static void 
xaaWrapperPolySegment(
    DrawablePtr	pDraw,
    GCPtr	pGC,
    int		nseg,
    xSegment	*pSeg
    ){
    XAAWRAPPER_GC_OP_PROLOGUE(pGC, pDraw);
    (*pGC->ops->PolySegment)(pDraw, pGC, nseg, pSeg);
    XAAWRAPPER_GC_OP_EPILOGUE(pGC, pDraw);
}

static void
xaaWrapperPolyRectangle(
    DrawablePtr  pDraw,
    GCPtr        pGC,
    int	         nRects,
    xRectangle  *pRects 
){
    XAAWRAPPER_GC_OP_PROLOGUE(pGC, pDraw);
    (*pGC->ops->PolyRectangle)(pDraw, pGC, nRects, pRects);
    XAAWRAPPER_GC_OP_EPILOGUE(pGC, pDraw);
}

static void
xaaWrapperPolyArc(
    DrawablePtr	pDraw,
    GCPtr	pGC,
    int		narcs,
    xArc	*parcs 
){
    XAAWRAPPER_GC_OP_PROLOGUE(pGC, pDraw);
    (*pGC->ops->PolyArc)(pDraw, pGC, narcs, parcs);
    XAAWRAPPER_GC_OP_EPILOGUE(pGC, pDraw);
}

static void
xaaWrapperFillPolygon(
    DrawablePtr	pDraw,
    GCPtr	pGC,
    int		shape,
    int		mode,
    int		count,
    DDXPointPtr	pptInit 
){
    XAAWRAPPER_GC_OP_PROLOGUE(pGC, pDraw);
    (*pGC->ops->FillPolygon)(pDraw, pGC, shape, mode, count, pptInit);
    XAAWRAPPER_GC_OP_EPILOGUE(pGC, pDraw);
}

static void 
xaaWrapperPolyFillRect(
    DrawablePtr	pDraw,
    GCPtr	pGC,
    int		nRectsInit, 
    xRectangle	*pRectsInit 
){
    XAAWRAPPER_GC_OP_PROLOGUE(pGC, pDraw);
    (*pGC->ops->PolyFillRect)(pDraw, pGC, nRectsInit, pRectsInit);
    XAAWRAPPER_GC_OP_EPILOGUE(pGC, pDraw);
}

static void
xaaWrapperPolyFillArc(
    DrawablePtr	pDraw,
    GCPtr	pGC,
    int		narcs,
    xArc	*parcs 
){
    XAAWRAPPER_GC_OP_PROLOGUE(pGC, pDraw);
    (*pGC->ops->PolyFillArc)(pDraw, pGC, narcs, parcs);
    XAAWRAPPER_GC_OP_EPILOGUE(pGC, pDraw);
}

static int
xaaWrapperPolyText8(
    DrawablePtr pDraw,
    GCPtr	pGC,
    int		x, 
    int 	y,
    int 	count,
    char	*chars 
){
    int width;
    
    XAAWRAPPER_GC_OP_PROLOGUE(pGC, pDraw);
    width = (*pGC->ops->PolyText8)(pDraw, pGC, x, y, count, chars);
    XAAWRAPPER_GC_OP_EPILOGUE(pGC, pDraw);

    return width;
}

static int
xaaWrapperPolyText16(
    DrawablePtr pDraw,
    GCPtr	pGC,
    int		x,
    int		y,
    int 	count,
    unsigned short *chars 
){
    int width;
    
    XAAWRAPPER_GC_OP_PROLOGUE(pGC, pDraw);
    width = (*pGC->ops->PolyText16)(pDraw, pGC, x, y, count, chars);
    XAAWRAPPER_GC_OP_EPILOGUE(pGC, pDraw);

    return width;
}

static void
xaaWrapperImageText8(
    DrawablePtr pDraw,
    GCPtr	pGC,
    int		x, 
    int		y,
    int 	count,
    char	*chars 
){
    XAAWRAPPER_GC_OP_PROLOGUE(pGC, pDraw);
    (*pGC->ops->ImageText8)(pDraw, pGC, x, y, count, chars);
    XAAWRAPPER_GC_OP_EPILOGUE(pGC, pDraw);
}

static void
xaaWrapperImageText16(
    DrawablePtr pDraw,
    GCPtr	pGC,
    int		x,
    int		y,
    int 	count,
    unsigned short *chars 
){
    XAAWRAPPER_GC_OP_PROLOGUE(pGC, pDraw);
    (*pGC->ops->ImageText16)(pDraw, pGC, x, y, count, chars);
    XAAWRAPPER_GC_OP_EPILOGUE(pGC, pDraw);
}

static void
xaaWrapperImageGlyphBlt(
    DrawablePtr pDraw,
    GCPtr pGC,
    int x, int y,
    unsigned int nglyph,
    CharInfoPtr *ppci,
    pointer pglyphBase 
){
    XAAWRAPPER_GC_OP_PROLOGUE(pGC, pDraw);
    (*pGC->ops->ImageGlyphBlt)(pDraw, pGC, x, y, nglyph, 
					ppci, pglyphBase);
    XAAWRAPPER_GC_OP_EPILOGUE(pGC, pDraw);
}

static void
xaaWrapperPolyGlyphBlt(
    DrawablePtr pDraw,
    GCPtr pGC,
    int x, int y,
    unsigned int nglyph,
    CharInfoPtr *ppci,
    pointer pglyphBase 
){
    XAAWRAPPER_GC_OP_PROLOGUE(pGC, pDraw);
    (*pGC->ops->PolyGlyphBlt)(pDraw, pGC, x, y, nglyph, 
				ppci, pglyphBase);
    XAAWRAPPER_GC_OP_EPILOGUE(pGC, pDraw);
}

static void
xaaWrapperPushPixels(
    GCPtr	pGC,
    PixmapPtr	pBitMap,
    DrawablePtr pDraw,
    int	dx, int dy, int xOrg, int yOrg 
){
    XAAWRAPPER_GC_OP_PROLOGUE(pGC, pDraw);
    (*pGC->ops->PushPixels)(pGC, pBitMap, pDraw, dx, dy, xOrg, yOrg);
    XAAWRAPPER_GC_OP_EPILOGUE(pGC, pDraw);
}
#endif

#ifdef RENDER
static void
xaaWrapperComposite (CARD8 op, PicturePtr pSrc, PicturePtr pMask, PicturePtr pDst,
	     INT16 xSrc, INT16 ySrc, INT16 xMask, INT16 yMask,
    INT16 xDst, INT16 yDst, CARD16 width, CARD16 height)
{
    ScreenPtr		pScreen = pDst->pDrawable->pScreen;
    PictureScreenPtr	ps = GetPictureScreen(pScreen);
    xaaWrapperScrPriv(pScreen);

    unwrap (pScrPriv, ps, Composite);
    (*ps->Composite) (op, pSrc, pMask, pDst, xSrc, ySrc, xMask, yMask,
		      xDst, yDst, width, height);
    wrap (pScrPriv, ps, Composite, xaaWrapperComposite);
}


static void
xaaWrapperGlyphs (CARD8 op, PicturePtr pSrc, PicturePtr pDst,
	  PictFormatPtr maskFormat, INT16 xSrc, INT16 ySrc, int nlist,
	  GlyphListPtr list, GlyphPtr *glyphs)
{
    ScreenPtr		pScreen = pDst->pDrawable->pScreen;
    PictureScreenPtr	ps = GetPictureScreen(pScreen);
    xaaWrapperScrPriv(pScreen);

    unwrap (pScrPriv, ps, Glyphs);
    (*ps->Glyphs) (op, pSrc, pDst, maskFormat, xSrc, ySrc,
		   nlist, list, glyphs);
    wrap (pScrPriv, ps, Glyphs, xaaWrapperGlyphs);

}
#endif

void
XAASync(ScreenPtr pScreen)
{
    XAAScreenPtr pScreenPriv = 
	(XAAScreenPtr) pScreen->devPrivates[XAAScreenIndex].ptr;
    XAAInfoRecPtr infoRec = pScreenPriv->AccelInfoRec;

    if(infoRec->NeedToSync) {
        (*infoRec->Sync)(infoRec->pScrn);
        infoRec->NeedToSync = FALSE;
    }
}
