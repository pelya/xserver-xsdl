/*
 * $Id$
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
/* $XFree86: $ */

#include <stdio.h>
#include "X.h"
#define NEED_EVENTS
#include "Xproto.h"
#include "Xos.h"
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

extern WindowPtr    *WindowTable;

#define KD_DPMS_NORMAL	    0
#define KD_DPMS_STANDBY	    1
#define KD_DPMS_SUSPEND	    2
#define KD_DPMS_POWERDOWN   3
#define KD_DPMS_MAX	    KD_DPMS_POWERDOWN

/*
 * Configuration information per video card
 */
#define KD_MAX_CARD_ADDRESS 8
typedef struct _KdCardAttr {
    CARD32  io;
    CARD32  address[KD_MAX_CARD_ADDRESS];
    int	    naddr;
} KdCardAttr;

typedef struct _KdCardInfo {
    struct _KdCardFuncs	    *cfuncs;
    void		    *closure;
    KdCardAttr		    attr;
    void		    *driver;
    struct _KdScreenInfo    *screenList;
    int			    selected;
    struct _KdCardInfo	    *next;
} KdCardInfo;

extern KdCardInfo	*kdCardInfo;

/*
 * Configuration information per X screen
 */
typedef struct _KdScreenInfo {
    struct _KdScreenInfo    *next;
    KdCardInfo	*card;
    ScreenPtr	pScreen;
    void	*driver;
    CARD8	*frameBuffer;
    int		width;
    int		height;
    int		depth;
    int		rate;
    int		bitsPerPixel;
    int		pixelStride;
    int		byteStride;
    int         dpix, dpiy;
    unsigned long   visuals;
    Pixel       redMask, greenMask, blueMask;
    Bool        dumb;
    Bool        softCursor;
    int		mynum;
} KdScreenInfo;

typedef struct _KdCardFuncs {
    Bool	(*cardinit) (KdCardInfo *); /* detect and map device */
    Bool	(*scrinit) (KdScreenInfo *);/* initialize screen information */
    void	(*preserve) (KdCardInfo *); /* save graphics card state */
    void        (*enable) (ScreenPtr);      /* set up for rendering */
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

    void        (*getColors) (ScreenPtr, int, xColorItem *);
    void        (*putColors) (ScreenPtr, int, xColorItem *);
} KdCardFuncs;

#define KD_MAX_PSEUDO_DEPTH 8
#define KD_MAX_PSEUDO_SIZE	    (1 << KD_MAX_PSEUDO_DEPTH)

typedef struct {
    KdScreenInfo    *screen;
    KdCardInfo	    *card;

    Bool	    enabled;
    int		    bytesPerPixel;

    int		    dpmsState;
    
    ColormapPtr     pInstalledmap;                  /* current colormap */
    xColorItem      systemPalette[KD_MAX_PSEUDO_SIZE];/* saved windows colors */

    CloseScreenProcPtr  CloseScreen;
} KdPrivScreenRec, *KdPrivScreenPtr;

typedef struct _KdMouseFuncs {
    int		    (*Init) (void);
    void	    (*Read) (int);
    void	    (*Fini) (int);
} KdMouseFuncs;

typedef struct _KdKeyboardFuncs {
    void	    (*Load) (void);
    int		    (*Init) (void);
    void	    (*Read) (int);
    void	    (*Leds) (int);
    void	    (*Bell) (int, int, int);
    void	    (*Fini) (int);
    int		    LockLed;
} KdKeyboardFuncs;

typedef struct _KdOsFuncs {
    int		    (*Init) (void);
    void	    (*Enable) (void);
    Bool	    (*SpecialKey) (KeySym);
    void	    (*Disable) (void);
    void	    (*Fini) (void);
} KdOsFuncs;

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
extern KdOsFuncs	*kdOsFuncs;

#define KdGetScreenPriv(pScreen) ((KdPrivScreenPtr) \
				  (pScreen)->devPrivates[kdScreenPrivateIndex].ptr)
#define KdSetScreenPriv(pScreen,v) ((pScreen)->devPrivates[kdScreenPrivateIndex].ptr = \
				    (pointer) v)
#define KdScreenPriv(pScreen) KdPrivScreenPtr pScreenPriv = KdGetScreenPriv(pScreen)

/* knoop.c */
extern GCOps		kdNoopOps;

/* kcmap.c */
void
KdSetColormap (ScreenPtr pScreen);

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

/* kdrive.c */
extern miPointerScreenFuncRec kdPointerScreenFuncs;

void
KdSetRootClip (ScreenPtr pScreen, BOOL enable);

void
KdDisableScreen (ScreenPtr pScreen);

void
KdDisableScreens (void);

void
KdEnableScreen (ScreenPtr pScreen);

void
KdEnableScreens (void);

void
KdProcessSwitch (void);

void
KdParseScreen (KdScreenInfo *screen,
	       char	    *arg);

void
KdOsInit (KdOsFuncs *pOsFuncs);

Bool
KdAllocatePrivates (ScreenPtr pScreen);

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
KdInitOutput (ScreenInfo *pScreenInfo,
	      int argc, char **argv);
 
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
KdEnqueueKeyboardEvent(unsigned char	scan_code,
		       unsigned char	is_up);

#define KD_BUTTON_1	0x01
#define KD_BUTTON_2	0x02
#define KD_BUTTON_3	0x04
#define KD_MOUSE_DELTA	0x80000000

void
KdEnqueueMouseEvent(unsigned long flags, int x, int y);

void
KdEnqueueMotionEvent (int x, int y);

void
KdReleaseAllKeys (void);
    
void
KdSetLed (int led, Bool on);

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
ProcessInputEvents ();

extern KdMouseFuncs	Ps2MouseFuncs;
extern KdKeyboardFuncs	LinuxKeyboardFuncs;
extern KdOsFuncs	LinuxFuncs;

/* kmap.c */
void *
KdMapDevice (CARD32 addr, CARD32 size);

void
KdUnmapDevice (void *addr, CARD32 size);

/* ktest.c */
Bool
KdFrameBufferValid (CARD8 *base, int size);

int
KdFrameBufferSize (CARD8 *base, int max);
