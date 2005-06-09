/*
 * Id: kdrive.h,v 1.1 1999/11/02 03:54:46 keithp Exp $
 *
 * Copyright © 1999 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
/* $RCSId: xc/programs/Xserver/hw/kdrive/kdrive.h,v 1.29 2002/11/13 16:37:39 keithp Exp $ */

#ifndef _KDRIVE_H_
#define _KDRIVE_H_

#include <stdio.h>
#include <X11/X.h>
#define NEED_EVENTS
#include <X11/Xproto.h>
#include <X11/Xos.h>
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "servermd.h"
#include "mibstore.h"
#include "colormapst.h"
#include "gcstruct.h"
#include "input.h"
#include "mipointer.h"
#include "mi.h"
#include "dix.h"
#include "fb.h"
#include "fboverlay.h"
#include "shadow.h"
#include "randrstr.h"

extern WindowPtr    *WindowTable;

#define KD_DPMS_NORMAL	    0
#define KD_DPMS_STANDBY	    1
#define KD_DPMS_SUSPEND	    2
#define KD_DPMS_POWERDOWN   3
#define KD_DPMS_MAX	    KD_DPMS_POWERDOWN

#ifndef KD_MAX_FB
#define KD_MAX_FB   FB_OVERLAY_MAX
#endif

#ifndef KD_MAX_CARD_ADDRESS
#define KD_MAX_CARD_ADDRESS 8
#endif

/*
 * Configuration information per video card
 */

typedef struct _KdCardAttr {
    CARD32  io;
    CARD32  address[KD_MAX_CARD_ADDRESS];
    int	    naddr;

    /* PCI bus info */
    CARD16  vendorID;
    CARD16  deviceID;
    CARD8   domain;
    CARD8   bus;
    CARD8   slot;
    CARD8   func;
} KdCardAttr;

typedef struct _KdCardInfo {
    struct _KdCardFuncs	    *cfuncs;
    void		    *closure;
    KdCardAttr		    attr;
    void		    *driver;
    struct _KdScreenInfo    *screenList;
    int			    selected;
    struct _KdCardInfo	    *next;

    Bool		    needSync;
    int			    lastMarker;
} KdCardInfo;

extern KdCardInfo	*kdCardInfo;

/*
 * Configuration information per X screen
 */
typedef struct _KdFrameBuffer {
    CARD8	*frameBuffer;
    int		depth;
    int		bitsPerPixel;
    int		pixelStride;
    int		byteStride;
    Bool	shadow;
    unsigned long   visuals;
    Pixel       redMask, greenMask, blueMask;
    void	*closure;
} KdFrameBuffer;

typedef struct _KdOffscreenArea KdOffscreenArea;

typedef void (*KdOffscreenSaveProc) (ScreenPtr pScreen, KdOffscreenArea *area);

typedef enum _KdOffscreenState {
    KdOffscreenAvail,
    KdOffscreenRemovable,
    KdOffscreenLocked,
} KdOffscreenState;

struct _KdOffscreenArea {
    int			offset;
    int			save_offset;
    int			size;
    int			score;
    pointer		privData;
    
    KdOffscreenSaveProc save;

    KdOffscreenState	state;
    
   KdOffscreenArea	*next;
};

#define RR_Rotate_All	(RR_Rotate_0|RR_Rotate_90|RR_Rotate_180|RR_Rotate_270)
#define RR_Reflect_All	(RR_Reflect_X|RR_Reflect_Y)

typedef struct _KdScreenInfo {
    struct _KdScreenInfo    *next;
    KdCardInfo	*card;
    ScreenPtr	pScreen;
    void	*driver;
    Rotation	randr;	/* rotation and reflection */
    int		width;
    int		height;
    int		rate;
    int		width_mm;
    int		height_mm;
    int		subpixel_order;
    Bool        dumb;
    Bool        softCursor;
    int		mynum;
    DDXPointRec	origin;
    KdFrameBuffer   fb[KD_MAX_FB];
    CARD8	*memory_base;
    unsigned long   memory_size;
    unsigned long   off_screen_base;
} KdScreenInfo;

typedef struct _KdCardFuncs {
    Bool	(*cardinit) (KdCardInfo *); /* detect and map device */
    Bool	(*scrinit) (KdScreenInfo *);/* initialize screen information */
    Bool	(*initScreen) (ScreenPtr);  /* initialize ScreenRec */
    Bool	(*finishInitScreen) (ScreenPtr pScreen);
    Bool	(*createRes) (ScreenPtr);   /* create screen resources */
    void	(*preserve) (KdCardInfo *); /* save graphics card state */
    Bool        (*enable) (ScreenPtr);      /* set up for rendering */
    Bool	(*dpms) (ScreenPtr, int);   /* set DPMS screen saver */
    void        (*disable) (ScreenPtr);     /* turn off rendering */
    void	(*restore) (KdCardInfo *);  /* restore graphics card state */
    void	(*scrfini) (KdScreenInfo *);/* close down screen */
    void        (*cardfini) (KdCardInfo *); /* close down */

    Bool        (*initCursor) (ScreenPtr);      /* detect and map cursor */
    void        (*enableCursor) (ScreenPtr);    /* enable cursor */
    void        (*disableCursor) (ScreenPtr);   /* disable cursor */
    void        (*finiCursor) (ScreenPtr);      /* close down */
    void        (*recolorCursor) (ScreenPtr, int, xColorItem *);

    Bool        (*initAccel) (ScreenPtr);
    void        (*enableAccel) (ScreenPtr);
    void        (*disableAccel) (ScreenPtr);
    void        (*finiAccel) (ScreenPtr);

    void        (*getColors) (ScreenPtr, int, int, xColorItem *);
    void        (*putColors) (ScreenPtr, int, int, xColorItem *);

} KdCardFuncs;

#define KD_MAX_PSEUDO_DEPTH 8
#define KD_MAX_PSEUDO_SIZE	    (1 << KD_MAX_PSEUDO_DEPTH)

typedef struct {
    KdScreenInfo    *screen;
    KdCardInfo	    *card;

    Bool	    enabled;
    Bool	    closed;
    int		    bytesPerPixel[KD_MAX_FB];

    int		    dpmsState;
    
    KdOffscreenArea *off_screen_areas;

    ColormapPtr     pInstalledmap[KD_MAX_FB];         /* current colormap */
    xColorItem      systemPalette[KD_MAX_PSEUDO_SIZE];/* saved windows colors */

    CreateScreenResourcesProcPtr    CreateScreenResources;
    CloseScreenProcPtr  CloseScreen;
#ifdef FB_OLD_SCREEN
    miBSFuncRec	    BackingStoreFuncs;
#endif
} KdPrivScreenRec, *KdPrivScreenPtr;

typedef enum _kdMouseState {
    start,
    button_1_pend,
    button_1_down,
    button_2_down,
    button_3_pend,
    button_3_down,
    synth_2_down_13,
    synth_2_down_3,
    synth_2_down_1,
    num_input_states
} KdMouseState;

#define KD_MAX_BUTTON  7

typedef struct _KdMouseInfo {
    struct _KdMouseInfo	*next;
    void		*driver;
    void		*closure;
    char		*name;
    char		*prot;
    char		map[KD_MAX_BUTTON];
    int			nbutton;
    Bool		emulateMiddleButton;
    unsigned long	emulationTimeout;
    Bool		timeoutPending;
    KdMouseState	mouseState;
    Bool		eventHeld;
    xEvent		heldEvent;
    unsigned char	buttonState;
    int			emulationDx, emulationDy;
    int			inputType;
    Bool		transformCoordinates;
} KdMouseInfo;

extern KdMouseInfo	*kdMouseInfo;

extern int KdCurScreen;

KdMouseInfo *KdMouseInfoAdd (void);
void	    KdMouseInfoDispose (KdMouseInfo *mi);
void	    KdParseMouse (char *);

typedef struct _KdMouseFuncs {
    Bool    	    (*Init) (void);
    void	    (*Fini) (void);
} KdMouseFuncs;

typedef struct _KdKeyboardFuncs {
    void	    (*Load) (void);
    int		    (*Init) (void);
    void	    (*Leds) (int);
    void	    (*Bell) (int, int, int);
    void	    (*Fini) (void);
    int		    LockLed;
} KdKeyboardFuncs;

typedef struct _KdOsFuncs {
    int		    (*Init) (void);
    void	    (*Enable) (void);
    Bool	    (*SpecialKey) (KeySym);
    void	    (*Disable) (void);
    void	    (*Fini) (void);
    void	    (*pollEvents) (void);
} KdOsFuncs;

typedef enum _KdSyncPolarity {
    KdSyncNegative, KdSyncPositive
} KdSyncPolarity;

typedef struct _KdMonitorTiming {
    /* label */
    int		    horizontal;
    int		    vertical;
    int		    rate;
    /* pixel clock */
    int		    clock;  /* in KHz */
    /* horizontal timing */
    int		    hfp;    /* front porch */
    int		    hbp;    /* back porch */
    int		    hblank; /* blanking */
    KdSyncPolarity  hpol;   /* polarity */
    /* vertical timing */
    int		    vfp;    /* front porch */
    int		    vbp;    /* back porch */
    int		    vblank; /* blanking */
    KdSyncPolarity  vpol;   /* polarity */
} KdMonitorTiming;

extern const KdMonitorTiming	kdMonitorTimings[];
extern const int		kdNumMonitorTimings;

typedef struct _KdMouseMatrix {
    int	    matrix[2][3];
} KdMouseMatrix;

typedef struct _KaaTrapezoid {
    float tl, tr, ty;
    float bl, br, by;
} KaaTrapezoid;

typedef struct _KaaScreenInfo {
    int	        offsetAlign;
    int         pitchAlign;
    int		flags;

    int		(*markSync) (ScreenPtr pScreen);
    void	(*waitMarker) (ScreenPtr pScreen, int marker);

    Bool	(*PrepareSolid) (PixmapPtr	pPixmap,
				 int		alu,
				 Pixel		planemask,
				 Pixel		fg);
    void	(*Solid) (int x1, int y1, int x2, int y2);
    void	(*DoneSolid) (void);

    Bool	(*PrepareCopy) (PixmapPtr	pSrcPixmap,
				PixmapPtr	pDstPixmap,
				Bool		upsidedown,
				Bool		reverse,
				int		alu,
				Pixel		planemask);
    void	(*Copy) (int	srcX,
			 int	srcY,
			 int	dstX,
			 int	dstY,
			 int	width,
			 int	height);
    void	(*DoneCopy) (void);

    Bool        (*PrepareBlend) (int		op,
				 PicturePtr	pSrcPicture,
				 PicturePtr	pDstPicture,
				 PixmapPtr	pSrc,
				 PixmapPtr	pDst);
    void        (*Blend) (int	srcX,
			  int	srcY,
			  int	dstX,
			  int	dstY,
			  int	width,
			  int	height);
    void	(*DoneBlend) (void);

    Bool        (*CheckComposite) (int		op,
				   PicturePtr	pSrcPicture,
				   PicturePtr	pMaskPicture,
				   PicturePtr	pDstPicture);
    Bool        (*PrepareComposite) (int		op,
				     PicturePtr		pSrcPicture,
				     PicturePtr		pMaskPicture,
				     PicturePtr		pDstPicture,
				     PixmapPtr		pSrc,
				     PixmapPtr		pMask,
				     PixmapPtr		pDst);
    void        (*Composite) (int	srcX,
			     int	srcY,
			     int	maskX,
			     int	maskY,
			     int	dstX,
			     int	dstY,
			     int	width,
			     int	height);
    void	(*DoneComposite) (void);

    Bool	(*PrepareTrapezoids) (PicturePtr pDstPicture,
				      PixmapPtr pDst);
    void	(*Trapezoids) (KaaTrapezoid	 *traps,
			       int		 ntraps);
    void	(*DoneTrapezoids) (void);

    Bool        (*UploadToScreen) (PixmapPtr		pDst,
				   char			*src,
				   int			src_pitch);
    Bool        (*UploadToScratch) (PixmapPtr		pSrc,
				   PixmapPtr		pDst);
} KaaScreenInfoRec, *KaaScreenInfoPtr;

#define KAA_OFFSCREEN_PIXMAPS		(1 << 0)
#define KAA_OFFSCREEN_ALIGN_POT		(1 << 1)

/*
 * This is the only completely portable way to
 * compute this info.
 */

#ifndef BitsPerPixel
#define BitsPerPixel(d) (\
    PixmapWidthPaddingInfo[d].notPower2 ? \
    (PixmapWidthPaddingInfo[d].bytesPerPixel * 8) : \
    ((1 << PixmapWidthPaddingInfo[d].padBytesLog2) * 8 / \
    (PixmapWidthPaddingInfo[d].padRoundUp+1)))
#endif

extern int		kdScreenPrivateIndex;
extern unsigned long	kdGeneration;
extern Bool		kdEnabled;
extern Bool		kdSwitchPending;
extern Bool		kdEmulateMiddleButton;
extern Bool		kdDisableZaphod;
extern Bool		kdDontZap;
extern int		kdVirtualTerminal;
extern char		*kdSwitchCmd;
extern KdOsFuncs	*kdOsFuncs;

#define KdGetScreenPriv(pScreen) ((KdPrivScreenPtr) \
				  (pScreen)->devPrivates[kdScreenPrivateIndex].ptr)
#define KdSetScreenPriv(pScreen,v) ((pScreen)->devPrivates[kdScreenPrivateIndex].ptr = \
				    (pointer) v)
#define KdScreenPriv(pScreen) KdPrivScreenPtr pScreenPriv = KdGetScreenPriv(pScreen)

/* kaa.c */
Bool
kaaDrawInit (ScreenPtr	        pScreen,
	     KaaScreenInfoPtr   pScreenInfo);

void
kaaDrawFini (ScreenPtr	        pScreen);

void
kaaWrapGC (GCPtr pGC);

void
kaaUnwrapGC (GCPtr pGC);

/* kasync.c */
void
KdCheckFillSpans  (DrawablePtr pDrawable, GCPtr pGC, int nspans,
		   DDXPointPtr ppt, int *pwidth, int fSorted);

void
KdCheckSetSpans (DrawablePtr pDrawable, GCPtr pGC, char *psrc,
		 DDXPointPtr ppt, int *pwidth, int nspans, int fSorted);

void
KdCheckPutImage (DrawablePtr pDrawable, GCPtr pGC, int depth,
		 int x, int y, int w, int h, int leftPad, int format,
		 char *bits);

RegionPtr
KdCheckCopyArea (DrawablePtr pSrc, DrawablePtr pDst, GCPtr pGC,
		 int srcx, int srcy, int w, int h, int dstx, int dsty);

RegionPtr
KdCheckCopyPlane (DrawablePtr pSrc, DrawablePtr pDst, GCPtr pGC,
		  int srcx, int srcy, int w, int h, int dstx, int dsty,
		  unsigned long bitPlane);

void
KdCheckPolyPoint (DrawablePtr pDrawable, GCPtr pGC, int mode, int npt,
		  DDXPointPtr pptInit);

void
KdCheckPolylines (DrawablePtr pDrawable, GCPtr pGC,
		  int mode, int npt, DDXPointPtr ppt);

void
KdCheckPolySegment (DrawablePtr pDrawable, GCPtr pGC,
		    int nsegInit, xSegment *pSegInit);

void
KdCheckPolyRectangle (DrawablePtr pDrawable, GCPtr pGC, 
		      int nrects, xRectangle *prect);

void
KdCheckPolyArc (DrawablePtr pDrawable, GCPtr pGC, 
		int narcs, xArc *pArcs);

#define KdCheckFillPolygon	miFillPolygon

void
KdCheckPolyFillRect (DrawablePtr pDrawable, GCPtr pGC,
		     int nrect, xRectangle *prect);

void
KdCheckPolyFillArc (DrawablePtr pDrawable, GCPtr pGC, 
		    int narcs, xArc *pArcs);

void
KdCheckImageGlyphBlt (DrawablePtr pDrawable, GCPtr pGC,
		      int x, int y, unsigned int nglyph,
		      CharInfoPtr *ppci, pointer pglyphBase);

void
KdCheckPolyGlyphBlt (DrawablePtr pDrawable, GCPtr pGC,
		     int x, int y, unsigned int nglyph,
		     CharInfoPtr *ppci, pointer pglyphBase);

void
KdCheckPushPixels (GCPtr pGC, PixmapPtr pBitmap,
		   DrawablePtr pDrawable,
		   int w, int h, int x, int y);

void
KdCheckGetImage (DrawablePtr pDrawable,
		 int x, int y, int w, int h,
		 unsigned int format, unsigned long planeMask,
		 char *d);

void
KdCheckGetSpans (DrawablePtr pDrawable,
		 int wMax,
		 DDXPointPtr ppt,
		 int *pwidth,
		 int nspans,
		 char *pdstStart);

void
KdCheckSaveAreas (PixmapPtr	pPixmap,
		  RegionPtr	prgnSave,
		  int		xorg,
		  int		yorg,
		  WindowPtr	pWin);

void
KdCheckRestoreAreas (PixmapPtr	pPixmap,
		     RegionPtr	prgnSave,
		     int	xorg,
		     int    	yorg,
		     WindowPtr	pWin);

void
KdCheckPaintWindow (WindowPtr pWin, RegionPtr pRegion, int what);

void
KdCheckCopyWindow (WindowPtr pWin, DDXPointRec ptOldOrg, RegionPtr prgnSrc);

void
KdCheckPaintKey(DrawablePtr  pDrawable,
		RegionPtr    pRegion,
		CARD32       pixel,
		int          layer);

void
KdCheckOverlayCopyWindow  (WindowPtr pWin, DDXPointRec ptOldOrg, RegionPtr prgnSrc);

void
KdScreenInitAsync (ScreenPtr pScreen);
    
extern const GCOps	kdAsyncPixmapGCOps;

/* knoop.c */
extern GCOps		kdNoopOps;

/* kcmap.c */
void
KdSetColormap (ScreenPtr pScreen, int fb);

void
KdEnableColormap (ScreenPtr pScreen);

void
KdDisableColormap (ScreenPtr pScreen);

void
KdInstallColormap (ColormapPtr pCmap);

void
KdUninstallColormap (ColormapPtr pCmap);

int
KdListInstalledColormaps (ScreenPtr pScreen, Colormap *pCmaps);

void
KdStoreColors (ColormapPtr pCmap, int ndef, xColorItem *pdefs);

/* kcurscol.c */
void
KdAllocateCursorPixels (ScreenPtr	pScreen,
			int		fb,
			CursorPtr	pCursor, 
			Pixel		*source,
			Pixel		*mask);

/* kdrive.c */
extern miPointerScreenFuncRec kdPointerScreenFuncs;

void
KdSetRootClip (ScreenPtr pScreen, BOOL enable);

void
KdDisableScreen (ScreenPtr pScreen);

void
KdDisableScreens (void);

Bool
KdEnableScreen (ScreenPtr pScreen);

void
KdEnableScreens (void);

void
KdSuspend (void);

void
KdResume (void);

void
KdProcessSwitch (void);

Rotation
KdAddRotation (Rotation a, Rotation b);

Rotation
KdSubRotation (Rotation a, Rotation b);

void
KdParseScreen (KdScreenInfo *screen,
	       char	    *arg);

char *
KdSaveString (char *str);

void
KdParseMouse (char *arg);

void
KdParseRgba (char *rgba);

void
KdUseMsg (void);

int
KdProcessArgument (int argc, char **argv, int i);

void
KdOsInit (KdOsFuncs *pOsFuncs);

Bool
KdAllocatePrivates (ScreenPtr pScreen);

Bool
KdCreateScreenResources (ScreenPtr pScreen);

Bool
KdCloseScreen (int index, ScreenPtr pScreen);

Bool
KdSaveScreen (ScreenPtr pScreen, int on);

Bool
KdScreenInit(int index, ScreenPtr pScreen, int argc, char **argv);

void
KdInitScreen (ScreenInfo    *pScreenInfo,
	      KdScreenInfo  *screen,
	      int	    argc,
	      char	    **argv);

void
KdInitCard (ScreenInfo	    *pScreenInfo,
	    KdCardInfo	    *card,
	    int		    argc,
	    char	    **argv);

void
KdInitOutput (ScreenInfo    *pScreenInfo,
	      int	    argc,
	      char	    **argv);
 
void
KdSetSubpixelOrder (ScreenPtr pScreen, Rotation randr);
    
/* kinfo.c */
KdCardInfo *
KdCardInfoAdd (KdCardFuncs  *funcs,
	       KdCardAttr   *attr,
	       void	    *closure);

KdCardInfo *
KdCardInfoLast (void);

void
KdCardInfoDispose (KdCardInfo *ci);

KdScreenInfo *
KdScreenInfoAdd (KdCardInfo *ci);

void
KdScreenInfoDispose (KdScreenInfo *si);


/* kinput.c */
void
KdInitInput(KdMouseFuncs *, KdKeyboardFuncs *);

void
KdAddMouseDriver(KdMouseFuncs *);

int
KdAllocInputType (void);

Bool
KdRegisterFd (int type, int fd, void (*read) (int fd, void *closure), void *closure);

void
KdRegisterFdEnableDisable (int fd, 
			   int (*enable) (int fd, void *closure),
			   void (*disable) (int fd, void *closure));

void
KdUnregisterFds (int type, Bool do_close);

void
KdEnqueueKeyboardEvent(unsigned char	scan_code,
		       unsigned char	is_up);

#define KD_BUTTON_1	0x01
#define KD_BUTTON_2	0x02
#define KD_BUTTON_3	0x04
#define KD_BUTTON_4	0x08
#define KD_BUTTON_5	0x10
#define KD_MOUSE_DELTA	0x80000000

void
KdEnqueueMouseEvent(KdMouseInfo *mi, unsigned long flags, int x, int y);

void
KdEnqueueMotionEvent (KdMouseInfo *mi, int x, int y);

void
KdReleaseAllKeys (void);
    
void
KdSetLed (int led, Bool on);

void
KdSetMouseMatrix (KdMouseMatrix *matrix);

void
KdComputeMouseMatrix (KdMouseMatrix *matrix, Rotation randr, int width, int height);
    
void
KdBlockHandler (int		screen,
		pointer		blockData,
		pointer		timeout,
		pointer		readmask);

void
KdWakeupHandler (int		screen, 
		 pointer    	data,
		 unsigned long	result,
		 pointer	readmask);

void
KdDisableInput (void);

void
KdEnableInput (void);

void
ProcessInputEvents (void);

extern KdMouseFuncs	LinuxMouseFuncs;
extern KdMouseFuncs	LinuxEvdevFuncs;
extern KdMouseFuncs	Ps2MouseFuncs;
extern KdMouseFuncs	BusMouseFuncs;
extern KdMouseFuncs	MsMouseFuncs;
#ifdef TOUCHSCREEN
extern KdMouseFuncs	TsFuncs;
#endif
extern KdKeyboardFuncs	LinuxKeyboardFuncs;
extern KdOsFuncs	LinuxFuncs;

extern KdMouseFuncs	VxWorksMouseFuncs;
extern KdKeyboardFuncs	VxWorksKeyboardFuncs;
extern KdOsFuncs	VxWorksFuncs;

/* kmap.c */

#define KD_MAPPED_MODE_REGISTERS    0
#define KD_MAPPED_MODE_FRAMEBUFFER  1

void *
KdMapDevice (CARD32 addr, CARD32 size);

void
KdUnmapDevice (void *addr, CARD32 size);

void
KdSetMappedMode (CARD32 addr, CARD32 size, int mode);

void
KdResetMappedMode (CARD32 addr, CARD32 size, int mode);

/* kmode.c */
const KdMonitorTiming *
KdFindMode (KdScreenInfo    *screen,
	    Bool	    (*supported) (KdScreenInfo *,
					  const KdMonitorTiming *));

Bool
KdTuneMode (KdScreenInfo    *screen,
	    Bool	    (*usable) (KdScreenInfo *),
	    Bool	    (*supported) (KdScreenInfo *,
					  const KdMonitorTiming *));

#ifdef RANDR
Bool
KdRandRGetInfo (ScreenPtr pScreen, 
		int randr,
		Bool (*supported) (ScreenPtr pScreen, 
				   const KdMonitorTiming *));

const KdMonitorTiming *
KdRandRGetTiming (ScreenPtr	    pScreen,
		  Bool		    (*supported) (ScreenPtr pScreen, 
						  const KdMonitorTiming *),
		  int		    rate,
		  RRScreenSizePtr   pSize);
#endif

/* kpict.c */
void
KdPictureInitAsync (ScreenPtr pScreen);

#ifdef RENDER
void
KdCheckComposite (CARD8      op,
		  PicturePtr pSrc,
		  PicturePtr pMask,
		  PicturePtr pDst,
		  INT16      xSrc,
		  INT16      ySrc,
		  INT16      xMask,
		  INT16      yMask,
		  INT16      xDst,
		  INT16      yDst,
		  CARD16     width,
		  CARD16     height);

void
KdCheckRasterizeTrapezoid(PicturePtr	pMask,
			  xTrapezoid	*trap,
			  int		x_off,
			  int		y_off);
#endif

/* kshadow.c */
Bool
KdShadowFbAlloc (KdScreenInfo *screen, int fb, Bool rotate);

void
KdShadowFbFree (KdScreenInfo *screen, int fb);

Bool
KdShadowSet (ScreenPtr pScreen, int randr, ShadowUpdateProc update, ShadowWindowProc window);
    
void
KdShadowUnset (ScreenPtr pScreen);

/* ktest.c */
Bool
KdFrameBufferValid (CARD8 *base, int size);

int
KdFrameBufferSize (CARD8 *base, int max);

/* koffscreen.c */

Bool
KdOffscreenInit (ScreenPtr pScreen);

KdOffscreenArea *
KdOffscreenAlloc (ScreenPtr pScreen, int size, int align,
		  Bool locked,
		  KdOffscreenSaveProc save,
		  pointer privData);

KdOffscreenArea *
KdOffscreenFree (ScreenPtr pScreen, KdOffscreenArea *area);

void
KdOffscreenMarkUsed (PixmapPtr pPixmap);

void
KdOffscreenSwapOut (ScreenPtr pScreen);

void
KdOffscreenSwapIn (ScreenPtr pScreen);

void
KdOffscreenFini (ScreenPtr pScreen);

/* function prototypes to be implemented by the drivers */
void
InitCard (char *name);

#endif /* _KDRIVE_H_ */
