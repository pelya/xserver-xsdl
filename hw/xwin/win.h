/*
 *Copyright (C) 1994-2000 The XFree86 Project, Inc. All Rights Reserved.
 *
 *Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 *"Software"), to deal in the Software without restriction, including
 *without limitation the rights to use, copy, modify, merge, publish,
 *distribute, sublicense, and/or sell copies of the Software, and to
 *permit persons to whom the Software is furnished to do so, subject to
 *the following conditions:
 *
 *The above copyright notice and this permission notice shall be
 *included in all copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *NONINFRINGEMENT. IN NO EVENT SHALL THE XFREE86 PROJECT BE LIABLE FOR
 *ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 *CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *Except as contained in this notice, the name of the XFree86 Project
 *shall not be used in advertising or otherwise to promote the sale, use
 *or other dealings in this Software without prior written authorization
 *from the XFree86 Project.
 *
 * Authors:	Dakshinamurthy Karra
 *		Suhaib M Siddiqi
 *		Peter Busch
 *		Harold L Hunt II
 *		Kensuke Matsuzaki
 */
/* $XFree86: xc/programs/Xserver/hw/xwin/win.h,v 1.38 2003/10/08 11:13:02 eich Exp $ */

#ifndef _WIN_H_
#define _WIN_H_


#ifndef NO
#define NO					0
#endif
#ifndef YES
#define YES					1
#endif

/*
 * Build toggles for experimental features
 */
#define WIN_NATIVE_GDI_SUPPORT			YES
#define WIN_LAYER_SUPPORT			NO
#define WIN_NEW_KEYBOARD_SUPPORT		NO
#define WIN_EMULATE_PSEUDO_SUPPORT		YES
#define WIN_UPDATE_STATS			NO

/* Turn debug messages on or off */
#define CYGDEBUG				NO

#define NEED_EVENTS

#define WIN_DEFAULT_BPP				0
#define WIN_DEFAULT_WHITEPIXEL			255
#define WIN_DEFAULT_BLACKPIXEL			0
#define WIN_DEFAULT_LINEBIAS			0
#define WIN_DEFAULT_E3B_TIME			50 /* milliseconds */
#define WIN_DEFAULT_DPI				75
#define WIN_DEFAULT_REFRESH			0
#define WIN_DEFAULT_WIN_KILL			TRUE
#define WIN_DEFAULT_UNIX_KILL			FALSE
#define WIN_DEFAULT_CLIP_UPDATES_NBOXES		0
#define WIN_DEFAULT_EMULATE_PSEUDO		FALSE
#define WIN_DEFAULT_USER_GAVE_HEIGHT_AND_WIDTH	FALSE

#define WIN_DIB_MAXIMUM_SIZE	0x08000000 /* 16 MB on Windows 95, 98, Me */
#define WIN_DIB_MAXIMUM_SIZE_MB (WIN_DIB_MAXIMUM_SIZE / 8 / 1024 / 1024)

/*
 * Windows only supports 256 color palettes
 */
#define WIN_NUM_PALETTE_ENTRIES			256

/*
 * Number of times to call Restore in an attempt to restore the primary surface
 */
#define WIN_REGAIN_SURFACE_RETRIES		1

/*
 * Build a supported display depths mask by shifting one to the left
 * by the number of bits in the supported depth.
 */
#define WIN_SUPPORTED_BPPS	( (1 << (32 - 1)) | (1 << (24 - 1)) \
				| (1 << (16 - 1)) | (1 << (15 - 1)) \
				| (1 << ( 8 - 1)))
#define WIN_CHECK_DEPTH		YES

/*
 * Timer IDs for WM_TIMER
 */
#define WIN_E3B_TIMER_ID		1
#define WIN_POLLING_MOUSE_TIMER_ID	2


#define WIN_E3B_OFF		-1
#define WIN_FD_INVALID		-1

#define WIN_SERVER_NONE		0x0L	/* 0 */
#define WIN_SERVER_SHADOW_GDI	0x1L	/* 1 */
#define WIN_SERVER_SHADOW_DD	0x2L	/* 2 */
#define WIN_SERVER_SHADOW_DDNL	0x4L	/* 4 */
#define WIN_SERVER_PRIMARY_DD	0x8L	/* 8 */
#define WIN_SERVER_NATIVE_GDI	0x10L	/* 16 */

#define AltMapIndex		Mod1MapIndex
#define NumLockMapIndex		Mod2MapIndex
#define AltLangMapIndex		Mod3MapIndex
#define KanaMapIndex		Mod4MapIndex
#define ScrollLockMapIndex	Mod5MapIndex

#define WIN_MOD_LALT		0x00000001
#define WIN_MOD_RALT		0x00000002
#define WIN_MOD_LCONTROL	0x00000004
#define WIN_MOD_RCONTROL	0x00000008

#define WIN_24BPP_MASK_RED	0x00FF0000
#define WIN_24BPP_MASK_GREEN	0x0000FF00
#define WIN_24BPP_MASK_BLUE	0x000000FF

#define WIN_MAX_KEYS_PER_KEY	4

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>

#include <X11/XWDFile.h>

#ifdef HAS_MMAP
#include <sys/mman.h>
#ifndef MAP_FILE
#define MAP_FILE 0
#endif /* MAP_FILE */
#endif /* HAS_MMAP */

#include "X.h"
#include "Xproto.h"
#include "Xos.h"
#include "Xprotostr.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "pixmap.h"
#include "region.h"
#include "gcstruct.h"
#include "colormap.h"
#include "colormapst.h"
#include "miscstruct.h"
#include "servermd.h"
#include "windowstr.h"
#include "mi.h"
#include "micmap.h"
#include "migc.h"
#include "mifillarc.h"
#include "mifpoly.h"
#include "mibstore.h"
#include "input.h"
#include "mipointer.h"
#include "keysym.h"
#include "mibstore.h"
#include "micoord.h"
#include "dix.h"
#include "miline.h"
#include "shadow.h"
#include "fb.h"
#include "layer.h"

#ifdef RENDER
#include "mipict.h"
#include "picturestr.h"
#endif

#ifdef RANDR
#include "randrstr.h"
#endif

/*
 * Windows headers
 */
#include "winms.h"


/*
 * Define Windows constants
 */

#define WM_TRAYICON		(WM_USER + 1000)
#define WM_INIT_SYS_MENU	(WM_USER + 1001)
#define WM_GIVEUP		(WM_USER + 1002)


/*
 * Multi-Window Window Manager header
 */

#include "winwindow.h"


/* Cygwin's winuser.h does not define VK_KANA as of 28Mar2001 */
/* NOTE: Cygwin's winuser.h was fixed shortly after 28Mar2001. */
#ifndef VK_KANA
#define VK_KANA 15
#endif


/*
 * Debugging macros
 */

#if CYGDEBUG
#define DEBUG_MSG(str,...) \
if (fDebugProcMsg) \
{ \
  char *pszTemp; \
  int iLength; \
  \
  iLength = sprintf (NULL, str, ##__VA_ARGS__); \
  \
  pszTemp = malloc (iLength + 1); \
  \
  sprintf (pszTemp, str, ##__VA_ARGS__); \
  \
  MessageBox (NULL, pszTemp, szFunctionName, MB_OK); \
  \
  free (pszTemp); \
}
#else
#define DEBUG_MSG(str,...)
#endif

#if CYGDEBUG
#define DEBUG_FN_NAME(str) PTSTR szFunctionName = str
#else
#define DEBUG_FN_NAME(str)
#endif

#if CYGDEBUG || YES
#define DEBUGVARS BOOL fDebugProcMsg = FALSE
#else
#define DEBUGVARS
#endif

#if CYGDEBUG || YES
#define DEBUGPROC_MSG fDebugProcMsg = TRUE
#else
#define DEBUGPROC_MSG
#endif

/* We use xor this macro for detecting toggle key state changes */
#define WIN_XOR(a,b) ((!(a) && (b)) || ((a) && !(b)))


/*
 * Typedefs for engine dependent function pointers
 */

typedef Bool (*winAllocateFBProcPtr)(ScreenPtr);

typedef void (*winShadowUpdateProcPtr)(ScreenPtr, shadowBufPtr);

typedef Bool (*winCloseScreenProcPtr)(int, ScreenPtr);

typedef Bool (*winInitVisualsProcPtr)(ScreenPtr);

typedef Bool (*winAdjustVideoModeProcPtr)(ScreenPtr);

typedef Bool (*winCreateBoundingWindowProcPtr)(ScreenPtr);

typedef Bool (*winFinishScreenInitProcPtr)(int, ScreenPtr, int, char **);

typedef Bool (*winBltExposedRegionsProcPtr)(ScreenPtr);

typedef Bool (*winActivateAppProcPtr)(ScreenPtr);

typedef Bool (*winRedrawScreenProcPtr)(ScreenPtr pScreen);

typedef Bool (*winRealizeInstalledPaletteProcPtr)(ScreenPtr pScreen);

typedef Bool (*winInstallColormapProcPtr)(ColormapPtr pColormap);

typedef Bool (*winStoreColorsProcPtr)(ColormapPtr pmap, 
				      int ndef, xColorItem *pdefs);

typedef Bool (*winCreateColormapProcPtr)(ColormapPtr pColormap);

typedef Bool (*winDestroyColormapProcPtr)(ColormapPtr pColormap);

typedef Bool (*winHotKeyAltTabProcPtr)(ScreenPtr);

typedef Bool (*winCreatePrimarySurfaceProcPtr)(ScreenPtr);

typedef Bool (*winReleasePrimarySurfaceProcPtr)(ScreenPtr);


/*
 * GC (graphics context) privates
 */

typedef struct
{
  HDC			hdc;
  HDC			hdcMem;
} winPrivGCRec, *winPrivGCPtr;


/*
 * Pixmap privates
 */

typedef struct
{
  HDC			hdcSelected;
  HBITMAP		hBitmap;
  BYTE			*pbBits;
  DWORD			dwScanlineBytes;
  BITMAPINFOHEADER	*pbmih;
} winPrivPixmapRec, *winPrivPixmapPtr;


/*
 * Colormap privates
 */

typedef struct
{
  HPALETTE		hPalette;
  LPDIRECTDRAWPALETTE	lpDDPalette;
  RGBQUAD		rgbColors[WIN_NUM_PALETTE_ENTRIES];
  PALETTEENTRY		peColors[WIN_NUM_PALETTE_ENTRIES];
} winPrivCmapRec, *winPrivCmapPtr;


#if WIN_NEW_KEYBOARD_SUPPORT
/*
 * Keyboard event structure
 */

typedef struct
{
  DWORD			dwXKeycodes[WIN_MAX_KEYS_PER_KEY];
  DWORD			dwReleaseModifiers;
} winKeyEventsRec, *winKeyEventsPtr;

#endif /* WIN_NEW_KEYBOARD_SUPPORT */

/*
 * Screen information structure that we need before privates are available
 * in the server startup sequence.
 */

typedef struct
{
  ScreenPtr		pScreen;
  
  /* Did the user specify a height and width? */
  Bool			fUserGaveHeightAndWidth;

  DWORD			dwScreen;
  DWORD			dwUserWidth;
  DWORD			dwUserHeight;
  DWORD			dwWidth;
  DWORD			dwHeight;
  DWORD			dwWidth_mm;
  DWORD			dwHeight_mm;
  DWORD			dwPaddedWidth;

  /*
   * dwStride is the number of whole pixels that occupy a scanline,
   * including those pixels that are not displayed.  This is basically
   * a rounding up of the width.
   */
  DWORD			dwStride;

  /* Offset of the screen in the window when using scrollbars */
  DWORD			dwXOffset;
  DWORD			dwYOffset;

  DWORD			dwBPP;
  DWORD			dwDepth;
  DWORD			dwRefreshRate;
  char			*pfb;
  XWDColor		*pXWDCmap;
  XWDFileHeader		*pXWDHeader;
  DWORD			dwEngine;
  DWORD			dwEnginePreferred;
  DWORD			dwClipUpdatesNBoxes;
  Bool			fEmulatePseudo;
  Bool			fFullScreen;
  Bool			fDecoration;
  Bool			fRootless;
  Bool			fMultiWindow;
  Bool                  fMultipleMonitors;
  Bool			fClipboard;
  Bool			fLessPointer;
  Bool			fScrollbars;
  Bool			fNoTrayIcon;
  int			iE3BTimeout;
  /* Windows (Alt+F4) and Unix (Ctrl+Alt+Backspace) Killkey */
  Bool                  fUseWinKillKey;
  Bool                  fUseUnixKillKey;
  Bool			fIgnoreInput;

  /* Did the user explicitly set this screen? */
  Bool			fExplicitScreen;
} winScreenInfo, *winScreenInfoPtr;


/*
 * Screen privates
 */

typedef struct _winPrivScreenRec
{
  winScreenInfoPtr	pScreenInfo;

  Bool			fEnabled;
  Bool			fClosed;
  Bool			fActive;
  Bool			fBadDepth;

  int			iDeltaZ;

  CloseScreenProcPtr	CloseScreen;

  DWORD			dwRedMask;
  DWORD			dwGreenMask;
  DWORD			dwBlueMask;
  DWORD			dwBitsPerRGB;

  DWORD			dwModeKeyStates;

  /* Handle to icons that must be freed */
  HICON			hiconNotifyIcon;

  /* Clipboard support */
  pthread_t		ptClipboardProc;

#if 0
  HWND			hwndNextViewer;
  void			*display;
  int			window;
#endif

  /* Last width, height, and depth of the Windows display */
  DWORD			dwLastWindowsWidth;
  DWORD			dwLastWindowsHeight;
  DWORD			dwLastWindowsBitsPixel;

  /* Layer support */
#if WIN_LAYER_SUPPORT
  DWORD			dwLayerKind;
  LayerPtr		pLayer;
#endif

  /* Palette management */
  ColormapPtr		pcmapInstalled;

  /* Pointer to the root visual so we only have to look it up once */
  VisualPtr		pRootVisual;

  /* 3 button emulation variables */
  int			iE3BCachedPress;
  Bool			fE3BFakeButton2Sent;

  /* Privates used by shadow fb GDI server */
  HBITMAP		hbmpShadow;
  HDC			hdcScreen;
  HDC			hdcShadow;
  HWND			hwndScreen;
  
  /* Privates used by shadow fb and primary fb DirectDraw servers */
  LPDIRECTDRAW		pdd;
  LPDIRECTDRAWSURFACE2	pddsPrimary;
  LPDIRECTDRAW2		pdd2;

  /* Privates used by shadow fb DirectDraw server */
  LPDIRECTDRAWSURFACE2	pddsShadow;
  LPDDSURFACEDESC	pddsdShadow;

  /* Privates used by primary fb DirectDraw server */
  LPDIRECTDRAWSURFACE2	pddsOffscreen;
  LPDDSURFACEDESC	pddsdOffscreen;
  LPDDSURFACEDESC	pddsdPrimary;

  /* Privates used by shadow fb DirectDraw Nonlocking server */
  LPDIRECTDRAW4		pdd4;
  LPDIRECTDRAWSURFACE4	pddsShadow4;
  LPDIRECTDRAWSURFACE4	pddsPrimary4;

  /* Privates used by both shadow fb DirectDraw servers */
  LPDIRECTDRAWCLIPPER	pddcPrimary;

  /* Privates used by multi-window server */
  pthread_t		ptWMProc;
  pthread_t		ptXMsgProc;
  void			*pWMInfo;
  Bool                  fWindowOrderChanged;
  Bool                  fRestacking;
  Bool			fRootWindowShown;

  /* Privates used for any module running in a seperate thread */
  pthread_mutex_t	pmServerStarted;
  Bool			fServerStarted;
  
  /* Engine specific functions */
  winAllocateFBProcPtr			pwinAllocateFB;
  winShadowUpdateProcPtr		pwinShadowUpdate;
  winCloseScreenProcPtr			pwinCloseScreen;
  winInitVisualsProcPtr			pwinInitVisuals;
  winAdjustVideoModeProcPtr		pwinAdjustVideoMode;
  winCreateBoundingWindowProcPtr	pwinCreateBoundingWindow;
  winFinishScreenInitProcPtr		pwinFinishScreenInit;
  winBltExposedRegionsProcPtr		pwinBltExposedRegions;
  winActivateAppProcPtr			pwinActivateApp;
  winRedrawScreenProcPtr		pwinRedrawScreen;
  winRealizeInstalledPaletteProcPtr	pwinRealizeInstalledPalette;
  winInstallColormapProcPtr		pwinInstallColormap;
  winStoreColorsProcPtr			pwinStoreColors;
  winCreateColormapProcPtr		pwinCreateColormap;
  winDestroyColormapProcPtr		pwinDestroyColormap;
  winHotKeyAltTabProcPtr		pwinHotKeyAltTab;
  winCreatePrimarySurfaceProcPtr	pwinCreatePrimarySurface;
  winReleasePrimarySurfaceProcPtr	pwinReleasePrimarySurface;

  /* Window Procedures for Rootless mode */
  CreateWindowProcPtr			CreateWindow;
  DestroyWindowProcPtr			DestroyWindow;
  PositionWindowProcPtr			PositionWindow;
  ChangeWindowAttributesProcPtr		ChangeWindowAttributes;
  RealizeWindowProcPtr			RealizeWindow;
  UnrealizeWindowProcPtr		UnrealizeWindow;
  ValidateTreeProcPtr			ValidateTree;
  PostValidateTreeProcPtr		PostValidateTree;
  WindowExposuresProcPtr		WindowExposures;
  PaintWindowBackgroundProcPtr		PaintWindowBackground;
  PaintWindowBorderProcPtr		PaintWindowBorder;
  CopyWindowProcPtr			CopyWindow;
  ClearToBackgroundProcPtr		ClearToBackground;
  ClipNotifyProcPtr			ClipNotify;
  RestackWindowProcPtr			RestackWindow;
  ReparentWindowProcPtr			ReparentWindow;
#ifdef SHAPE
  SetShapeProcPtr			SetShape;
#endif
} winPrivScreenRec;


typedef struct {
  pointer		value;
  XID			id;
} WindowIDPairRec, *WindowIDPairPtr;


/*
 * Extern declares for general global variables
 */

extern winScreenInfo		g_ScreenInfo[];
extern miPointerScreenFuncRec	g_winPointerCursorFuncs;
extern DWORD			g_dwEvents;
extern int			g_fdMessageQueue;
extern int			g_iScreenPrivateIndex;
extern int			g_iCmapPrivateIndex;
extern int			g_iGCPrivateIndex;
extern int			g_iPixmapPrivateIndex;
extern int			g_iWindowPrivateIndex;
extern unsigned long		g_ulServerGeneration;
extern CARD32			g_c32LastInputEventTime;
extern DWORD			g_dwEnginesSupported;
extern HINSTANCE		g_hInstance;
extern HWND			g_hDlgDepthChange;
extern HWND			g_hDlgExit;
extern int                      g_copyROP[];
extern int                      g_patternROP[];
extern const char *		g_pszQueryHost;


/*
 * Extern declares for dynamically loaded libraries and function pointers
 */

extern HMODULE			g_hmodDirectDraw;
extern FARPROC			g_fpDirectDrawCreate;
extern FARPROC			g_fpDirectDrawCreateClipper;

extern HMODULE			g_hmodCommonControls;
extern FARPROC			g_fpTrackMouseEvent;


/*
 * Screen privates macros
 */

#define winGetScreenPriv(pScreen) \
	((winPrivScreenPtr) (pScreen)->devPrivates[g_iScreenPrivateIndex].ptr)

#define winSetScreenPriv(pScreen,v) \
	((pScreen)->devPrivates[g_iScreenPrivateIndex].ptr = (pointer) v)

#define winScreenPriv(pScreen) \
	winPrivScreenPtr pScreenPriv = winGetScreenPriv(pScreen)


/*
 * Colormap privates macros
 */

#define winGetCmapPriv(pCmap) \
	((winPrivCmapPtr) (pCmap)->devPrivates[g_iCmapPrivateIndex].ptr)

#define winSetCmapPriv(pCmap,v) \
	((pCmap)->devPrivates[g_iCmapPrivateIndex].ptr = (pointer) v)

#define winCmapPriv(pCmap) \
	winPrivCmapPtr pCmapPriv = winGetCmapPriv(pCmap)


/*
 * GC privates macros
 */

#define winGetGCPriv(pGC) \
	((winPrivGCPtr) (pGC)->devPrivates[g_iGCPrivateIndex].ptr)

#define winSetGCPriv(pGC,v) \
	((pGC)->devPrivates[g_iGCPrivateIndex].ptr = (pointer) v)

#define winGCPriv(pGC) \
	winPrivGCPtr pGCPriv = winGetGCPriv(pGC)


/*
 * Pixmap privates macros
 */

#define winGetPixmapPriv(pPixmap) \
	((winPrivPixmapPtr) (pPixmap)->devPrivates[g_iPixmapPrivateIndex].ptr)

#define winSetPixmapPriv(pPixmap,v) \
	((pPixmap)->devPrivates[g_iPixmapPrivateIndex].ptr = (pointer) v)

#define winPixmapPriv(pPixmap) \
	winPrivPixmapPtr pPixmapPriv = winGetPixmapPriv(pPixmap)


/*
 * Window privates macros
 */

#define winGetWindowPriv(pWin) \
	((winPrivWinPtr) (pWin)->devPrivates[g_iWindowPrivateIndex].ptr)

#define winSetWindowPriv(pWin,v) \
	((pWin)->devPrivates[g_iWindowPrivateIndex].ptr = (pointer) v)

#define winWindowPriv(pWin) \
	winPrivWinPtr pWinPriv = winGetWindowPriv(pWin)


/*
 * FIXME: Windows mouse wheel macro; should be in Cygwin w32api headers.
 * Has been fixed after May 05, 2001.  Remove this section after the
 * fixed headers are in distribution.
 */

#ifndef GET_WHEEL_DELTA_WPARAM
#define GET_WHEEL_DELTA_WPARAM(wparam)		((short)HIWORD (wparam))
#endif /* GET_WHEEL_DELTA_WPARAM */


/*
 * FIXME: Windows shell API defines.  Should be in w32api shellapi.h
 */

#ifndef ABS_AUTOHIDE
#define	ABS_AUTOHIDE		0x00000001
#endif
#ifndef ABS_ALWAYSONTOP
#define	ABS_ALWAYSONTOP		0x00000002
#endif


/*
 * BEGIN DDX and DIX Function Prototypes
 */


/*
 * winallpriv.c
 */

Bool
winAllocatePrivates (ScreenPtr pScreen);

Bool
winInitCmapPrivates (ColormapPtr pCmap);

Bool
winAllocateCmapPrivates (ColormapPtr pCmap);


/*
 * winblock.c
 */

void
winBlockHandler (int nScreen,
		 pointer pBlockData,
		 pointer pTimeout,
		 pointer pReadMask);


/*
 * winclip.c
 */

RegionPtr
winPixmapToRegionNativeGDI (PixmapPtr pPix);


/*
 * winclipboardinit.c
 */

Bool
winInitClipboard (pthread_t *ptClipboardProc,
		  pthread_mutex_t *ppmServerStarted,
		  DWORD dwScreen);

/*
 * winclipboardthread.c
 */

void
winDeinitClipboard ();


/*
 * wincmap.c
 */

int
winListInstalledColormaps (ScreenPtr pScreen, Colormap *pmaps);

void
winStoreColors (ColormapPtr pmap, int ndef, xColorItem *pdefs);

void
winInstallColormap (ColormapPtr pmap);

void
winUninstallColormap (ColormapPtr pmap);

void
winResolveColor (unsigned short *pred,
		 unsigned short *pgreen,
		 unsigned short *pblue,
		 VisualPtr	pVisual);

Bool
winCreateColormap (ColormapPtr pmap);

void
winDestroyColormap (ColormapPtr pmap);

int
winExpandDirectColors (ColormapPtr pmap, int ndef,
		       xColorItem *indefs, xColorItem *outdefs);

Bool
winCreateDefColormap (ScreenPtr pScreen);

Bool
winSetVisualTypesNativeGDI (int nDepth, int nVisuals, int nBitsPerRGB);

void
winClearVisualTypes (void);


/*
 * wincreatewnd.c
 */

Bool
winCreateBoundingWindowFullScreen (ScreenPtr pScreen);

Bool
winCreateBoundingWindowWindowed (ScreenPtr pScreen);


/*
 * wincursor.c
 */

void
winPointerWarpCursor (ScreenPtr pScreen, int x, int y);

Bool
winCursorOffScreen (ScreenPtr *ppScreen, int *x, int *y);

void
winCrossScreen (ScreenPtr pScreen, Bool fEntering);


/*
 * windialogs.c
 */

void
winDisplayExitDialog (winPrivScreenPtr pScreenPriv);


void
winDisplayDepthChangeDialog (winPrivScreenPtr pScreenPriv);


/*
 * winengine.c
 */

void
winDetectSupportedEngines ();

Bool
winSetEngine (ScreenPtr pScreen);

Bool
winGetDDProcAddresses ();


/*
 * winerror.c
 */

void
OSVenderVErrorF (const char *pszFormat, va_list va_args);


/*
 * winfillsp.c
 */

void
winFillSpansNativeGDI (DrawablePtr	pDrawable,
		       GCPtr		pGC,
		       int		nSpans,
		       DDXPointPtr	pPoints,
		       int		*pWidths,
		       int		fSorted);


/*
 * winfont.c
 */

Bool
winRealizeFontNativeGDI (ScreenPtr pScreen, FontPtr pFont);

Bool
winUnrealizeFontNativeGDI (ScreenPtr pScreen, FontPtr pFont);


/*
 * wingc.c
 */

Bool
winCreateGCNativeGDI (GCPtr pGC);

void
winChangeGCNativeGDI (GCPtr pGC, unsigned long ulChanges);

void
winPadPixmapNativdGDI (PixmapPtr pPixmap);

void
winValidateGCNativeGDI (GCPtr pGC,
			unsigned long changes,
			DrawablePtr pDrawable);

void
winCopyGCNativeGDI (GCPtr pGCsrc, unsigned long ulMask, GCPtr pGCdst);

void
winDestroyGCNativeGDI (GCPtr pGC);

void
winChangeClipNativeGDI (GCPtr pGC, int nType, pointer pValue, int nRects);

void
winDestroyClipNativeGDI (GCPtr pGC);

void
winCopyClipNativeGDI (GCPtr pGCdst, GCPtr pGCsrc);


/*
 * wingetsp.c
 */

void
winGetSpansNativeGDI (DrawablePtr	pDrawable, 
		      int		wMax, 
		      DDXPointPtr	pPoints, 
		      int		*pWidths, 
		      int		nSpans, 
		      char		*pDst);


/*
 * winkeybd.c
 */

#if WIN_NEW_KEYBOARD_SUPPORT
winKeyEventsRec
winTranslateKey (DWORD dwVirtualKey, DWORD dwKeyData);
#else
void
winTranslateKey (WPARAM wParam, LPARAM lParam, int *piScanCode);
#endif

void
winGetKeyMappings (KeySymsPtr pKeySyms, CARD8 *pModMap);

void
winKeybdBell (int iPercent, DeviceIntPtr pDeviceInt,
	      pointer pCtrl, int iClass);

void
winKeybdCtrl (DeviceIntPtr pDevice, KeybdCtrl *pCtrl);

int
winKeybdProc (DeviceIntPtr pDeviceInt, int iState);

void
winInitializeModeKeyStates (void);

void
winStoreModeKeyStates (ScreenPtr pScreen);

void
winRestoreModeKeyStates (ScreenPtr pScreen);

Bool
winIsFakeCtrl_L (UINT message, WPARAM wParam, LPARAM lParam);

void
winKeybdReleaseKeys ();

void
winSendKeyEvent (DWORD dwKey, Bool fDown);

#if WIN_NEW_KEYBOARD_SUPPORT
void
winProcessKeyEvent (DWORD dwVirtKey, DWORD dwKeyData);
#endif


/*
 * winlayer.c
 */

#if WIN_LAYER_SUPPORT

LayerPtr
winLayerCreate (ScreenPtr pScreen);

int
winLayerAdd (WindowPtr pWindow, pointer value);

int
winLayerRemove (WindowPtr pWindow, pointer value);

#ifdef RANDR
Bool
winRandRGetInfo (ScreenPtr pScreen, Rotation *pRotations);

Bool
winRandRSetConfig (ScreenPtr		pScreen,
		   Rotation		rotateKind,
		   RRScreenSizePtr	pSize);

Bool
winRandRInit (ScreenPtr pScreen);
#endif /* RANDR */
#endif /* WIN_LAYER_SUPPORT */


/*
 * winmisc.c
 */

void
winQueryBestSizeNativeGDI (int class, unsigned short *pWidth,
			   unsigned short *pHeight, ScreenPtr pScreen);

CARD8
winCountBits (DWORD dw);

Bool
winUpdateFBPointer (ScreenPtr pScreen, void *pbits);

BOOL
winPaintBackground (HWND hwnd, COLORREF colorref);


/*
 * winmouse.c
 */

void
winMouseCtrl (DeviceIntPtr pDevice, PtrCtrl *pCtrl);

int
winMouseProc (DeviceIntPtr pDeviceInt, int iState);

int
winMouseWheel (ScreenPtr pScreen, int iDeltaZ);

void
winMouseButtonsSendEvent (int iEventType, int iButton);

int
winMouseButtonsHandle (ScreenPtr pScreen,
		       int iEventType, int iButton,
		       WPARAM wParam);


/*
 * winnativegdi.c
 */

HBITMAP
winCreateDIBNativeGDI (int iWidth, int iHeight, int iDepth,
		       BYTE **ppbBits, BITMAPINFO **ppbmi);

Bool
winAllocateFBNativeGDI (ScreenPtr pScreen);

void
winShadowUpdateNativeGDI (ScreenPtr pScreen, 
			  shadowBufPtr pBuf);

Bool
winCloseScreenNativeGDI (int nIndex, ScreenPtr pScreen);

Bool
winInitVisualsNativeGDI (ScreenPtr pScreen);

Bool
winAdjustVideoModeNativeGDI (ScreenPtr pScreen);

Bool
winBltExposedRegionsNativeGDI (ScreenPtr pScreen);

Bool
winActivateAppNativeGDI (ScreenPtr pScreen);

Bool
winSetEngineFunctionsNativeGDI (ScreenPtr pScreen);

Bool
winRedrawScreenNativeGDI (ScreenPtr pScreen);

Bool
winRealizeInstalledPaletteNativeGDI (ScreenPtr pScreen);

Bool
winInstallColormapNativeGDI (ColormapPtr pColormap);

Bool
winStoreColorsNativeGDI (ColormapPtr pmap, 
			 int ndef,
			 xColorItem *pdefs);

Bool
winCreateColormapNativeGDI (ColormapPtr pColormap);

Bool
winDestroyColormapNativeGDI (ColormapPtr pColormap);



/*
 * winpfbddd.c
 */

Bool
winAllocateFBPrimaryDD (ScreenPtr pScreen);

Bool
winCloseScreenPrimaryDD (int nIndex, ScreenPtr pScreen);

Bool
winInitVisualsPrimaryDD (ScreenPtr pScreen);

Bool
winAdjustVideoModePrimaryDD (ScreenPtr pScreen);

Bool
winActivateAppPrimaryDD (ScreenPtr pScreen);

Bool
winSetEngineFunctionsPrimaryDD (ScreenPtr pScreen);

Bool
winHotKeyAltTabPrimaryDD (ScreenPtr pScreen);


/*
 * winpixmap.c
 */

PixmapPtr
winCreatePixmapNativeGDI (ScreenPtr pScreen, int width, int height, int depth);

Bool
winDestroyPixmapNativeGDI (PixmapPtr pPixmap);

void
winXRotatePixmapNativeGDI (PixmapPtr pPix, int rw);

void
winYRotatePixmapNativeGDI (PixmapPtr pPix, int rh);

void
winCopyRotatePixmapNativeGDI (PixmapPtr psrcPix, PixmapPtr *ppdstPix,
			      int xrot, int yrot);

Bool
winModifyPixmapHeaderNativeGDI (PixmapPtr pPixmap,
				int iWidth, int iHeight,
				int iDepth,
				int iBitsPerPixel,
				int devKind,
				pointer pPixData);


/*
 * winpntwin.c
 */

void
winPaintWindowNativeGDI (WindowPtr pWin, RegionPtr pRegion, int what);


/*
 * winpolyline.c
 */

void
winPolyLineNativeGDI (DrawablePtr	pDrawable,
		      GCPtr		pGC,
		      int		mode,
		      int		npt,
		      DDXPointPtr	ppt);


/*
 * winscrinit.c
 */

Bool
winScreenInit (int index,
	       ScreenPtr pScreen,
	       int argc, char **argv);

Bool
winFinishScreenInitFB (int index,
		       ScreenPtr pScreen,
		       int argc, char **argv);

Bool
winFinishScreenInitNativeGDI (int index,
			      ScreenPtr pScreen,
			      int argc, char **argv);

Bool
winSaveScreen (ScreenPtr pScreen, int on);

PixmapPtr
winGetWindowPixmap (WindowPtr pWin);

void
winSetWindowPixmap (WindowPtr pWin, PixmapPtr pPix);


/*
 * winsetsp.c
 */

void
winSetSpansNativeGDI (DrawablePtr	pDrawable,
		      GCPtr		pGC,
		      char		*pSrc,
		      DDXPointPtr	pPoints,
		      int		*pWidth,
		      int		nSpans,
		      int		fSorted);

/*
 * winshaddd.c
 */

Bool
winAllocateFBShadowDD (ScreenPtr pScreen);

void
winShadowUpdateDD (ScreenPtr pScreen, 
		   shadowBufPtr pBuf);

Bool
winCloseScreenShadowDD (int nIndex, ScreenPtr pScreen);

Bool
winInitVisualsShadowDD (ScreenPtr pScreen);

Bool
winAdjustVideoModeShadowDD (ScreenPtr pScreen);

Bool
winBltExposedRegionsShadowDD (ScreenPtr pScreen);

Bool
winActivateAppShadowDD (ScreenPtr pScreen);

Bool
winSetEngineFunctionsShadowDD (ScreenPtr pScreen);

Bool
winRedrawScreenShadowDD (ScreenPtr pScreen);

Bool
winRealizeInstalledPaletteShadowDD (ScreenPtr pScreen);

Bool
winInstallColormapShadowDD (ColormapPtr pColormap);

Bool
winStoreColorsShadowDD (ColormapPtr pmap, 
			int ndef,
			xColorItem *pdefs);

Bool
winCreateColormapShadowDD (ColormapPtr pColormap);

Bool
winDestroyColormapShadowDD (ColormapPtr pColormap);

Bool
winCreatePrimarySurfaceShadowDD (ScreenPtr pScreen);

Bool
winReleasePrimarySurfaceShadowDD (ScreenPtr pScreen);


/*
 * winshadddnl.c
 */

Bool
winAllocateFBShadowDDNL (ScreenPtr pScreen);

void
winShadowUpdateDDNL (ScreenPtr pScreen, 
		     shadowBufPtr pBuf);

Bool
winCloseScreenShadowDDNL (int nIndex, ScreenPtr pScreen);

Bool
winInitVisualsShadowDDNL (ScreenPtr pScreen);

Bool
winAdjustVideoModeShadowDDNL (ScreenPtr pScreen);

Bool
winBltExposedRegionsShadowDDNL (ScreenPtr pScreen);

Bool
winActivateAppShadowDDNL (ScreenPtr pScreen);

Bool
winSetEngineFunctionsShadowDDNL (ScreenPtr pScreen);

Bool
winRedrawScreenShadowDDNL (ScreenPtr pScreen);

Bool
winRealizeInstalledPaletteShadowDDNL (ScreenPtr pScreen);

Bool
winInstallColormapShadowDDNL (ColormapPtr pColormap);

Bool
winStoreColorsShadowDDNL (ColormapPtr pmap, 
			  int ndef,
			  xColorItem *pdefs);

Bool
winCreateColormapShadowDDNL (ColormapPtr pColormap);

Bool
winDestroyColormapShadowDDNL (ColormapPtr pColormap);

Bool
winCreatePrimarySurfaceShadowDDNL (ScreenPtr pScreen);

Bool
winReleasePrimarySurfaceShadowDDNL (ScreenPtr pScreen);


/*
 * winshadgdi.c
 */

Bool
winAllocateFBShadowGDI (ScreenPtr pScreen);

void
winShadowUpdateGDI (ScreenPtr pScreen, 
		    shadowBufPtr pBuf);

Bool
winCloseScreenShadowGDI (int nIndex, ScreenPtr pScreen);

Bool
winInitVisualsShadowGDI (ScreenPtr pScreen);

Bool
winAdjustVideoModeShadowGDI (ScreenPtr pScreen);

Bool
winActivateAppShadowGDI (ScreenPtr pScreen);

Bool
winRedrawScreenShadowGDI (ScreenPtr pScreen);

Bool
winSetEngineFunctionsShadowGDI (ScreenPtr pScreen);

Bool
winRealizeInstalledPaletteShadowGDI (ScreenPtr pScreen);

Bool
winInstallColormapShadowGDI (ColormapPtr pColormap);

Bool
winStoreColorsShadowGDI (ColormapPtr pmap, 
			 int ndef,
			 xColorItem *pdefs);

Bool
winCreateColormapShadowGDI (ColormapPtr pColormap);

Bool
winDestroyColormapShadowGDI (ColormapPtr pColormap);


/*
 * winwakeup.c
 */

void
winWakeupHandler (int nScreen,
		  pointer pWakeupData,
		  unsigned long ulResult,
		  pointer pReadmask);


/*
 * winwindow.c
 */

Bool
winCreateWindowNativeGDI (WindowPtr pWin);

Bool
winDestroyWindowNativeGDI (WindowPtr pWin);

Bool
winPositionWindowNativeGDI (WindowPtr pWin, int x, int y);

void 
winCopyWindowNativeGDI (WindowPtr pWin,
			DDXPointRec ptOldOrg,
			RegionPtr prgnSrc);

Bool
winChangeWindowAttributesNativeGDI (WindowPtr pWin, unsigned long mask);

Bool
winUnmapWindowNativeGDI (WindowPtr pWindow);

Bool
winMapWindowNativeGDI (WindowPtr pWindow);

Bool
winCreateWindowPRootless (WindowPtr pWindow);

Bool
winDestroyWindowPRootless (WindowPtr pWindow);

Bool
winPositionWindowPRootless (WindowPtr pWindow, int x, int y);

Bool
winChangeWindowAttributesPRootless (WindowPtr pWindow, unsigned long mask);

Bool
winUnmapWindowPRootless (WindowPtr pWindow);

Bool
winMapWindowPRootless (WindowPtr pWindow);

#ifdef SHAPE
void
winSetShapePRootless (WindowPtr pWindow);
#endif


/*
 * winmultiwindowicons.c
 */

HICON
winXIconToHICON (WindowPtr pWin);

void
winUpdateIcon (Window id);


/*
 * winmultiwindowshape.c
 */

#ifdef SHAPE
void
winReshapeMultiWindow (WindowPtr pWin);

void
winSetShapeMultiWindow (WindowPtr pWindow);

void
winUpdateRgnMultiWindow (WindowPtr pWindow);
#endif


/*
 * winmultiwindowwindow.c
 */

Bool
winCreateWindowMultiWindow (WindowPtr pWindow);

Bool
winDestroyWindowMultiWindow (WindowPtr pWindow);

Bool
winPositionWindowMultiWindow (WindowPtr pWindow, int x, int y);

Bool
winChangeWindowAttributesMultiWindow (WindowPtr pWindow, unsigned long mask);

Bool
winUnmapWindowMultiWindow (WindowPtr pWindow);

Bool
winMapWindowMultiWindow (WindowPtr pWindow);

void
winReparentWindowMultiWindow (WindowPtr pWin, WindowPtr pPriorParent);

void
winRestackWindowMultiWindow (WindowPtr pWin, WindowPtr pOldNextSib);

void
winReorderWindowsMultiWindow (ScreenPtr pScreen);

void
winMoveXWindow (WindowPtr pWin, int x, int y);

void
winResizeXWindow (WindowPtr pWin, int w, int h);

XID
winGetWindowID (WindowPtr pWin);


/*
 * winmultiwindowwndproc.c
 */

LRESULT CALLBACK
winTopLevelWindowProc (HWND hwnd, UINT message, 
		       WPARAM wParam, LPARAM lParam);


/*
 * wintrayicon.c
 */

void
winInitNotifyIcon (winPrivScreenPtr pScreenPriv);

void
winDeleteNotifyIcon (winPrivScreenPtr pScreenPriv);

LRESULT
winHandleIconMessage (HWND hwnd, UINT message,
		      WPARAM wParam, LPARAM lParam,
		      winPrivScreenPtr pScreenPriv);


/*
 * winwndproc.c
 */

LRESULT CALLBACK
winWindowProc (HWND hWnd, UINT message, 
	       WPARAM wParam, LPARAM lParam);


/*
 * END DDX and DIX Function Prototypes
 */

#endif /* _WIN_H_ */

