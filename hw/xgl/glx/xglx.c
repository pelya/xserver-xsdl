/*
 * Copyright © 2004 David Reveman
 * 
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * David Reveman not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * David Reveman makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * DAVID REVEMAN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL DAVID REVEMAN BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, 
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: David Reveman <davidr@novell.com>
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <glitz-glx.h>

#include "xgl.h"
#include "inputstr.h"
#include "cursorstr.h"
#include "mipointer.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>

#define XGLX_DEFAULT_SCREEN_WIDTH  800
#define XGLX_DEFAULT_SCREEN_HEIGHT 600

typedef struct _xglxScreen {
    Window	       win;
    Colormap	       colormap;
    CloseScreenProcPtr CloseScreen;
} xglxScreenRec, *xglxScreenPtr;

int xglxScreenGeneration = -1;
int xglxScreenPrivateIndex;

#define XGLX_GET_SCREEN_PRIV(pScreen) 				         \
    ((xglxScreenPtr) (pScreen)->devPrivates[xglxScreenPrivateIndex].ptr)

#define XGLX_SET_SCREEN_PRIV(pScreen, v)			       \
    ((pScreen)->devPrivates[xglxScreenPrivateIndex].ptr = (pointer) v)

#define XGLX_SCREEN_PRIV(pScreen)			       \
    xglxScreenPtr pScreenPriv = XGLX_GET_SCREEN_PRIV (pScreen)

typedef struct _xglxCursor {
    Cursor cursor;
} xglxCursorRec, *xglxCursorPtr;

#define XGLX_GET_CURSOR_PRIV(pCursor, pScreen)		   \
    ((xglxCursorPtr) (pCursor)->devPriv[(pScreen)->myNum])

#define XGLX_SET_CURSOR_PRIV(pCursor, pScreen, v)	 \
    ((pCursor)->devPriv[(pScreen)->myNum] = (pointer) v)

#define XGLX_CURSOR_PRIV(pCursor, pScreen)			        \
    xglxCursorPtr pCursorPriv = XGLX_GET_CURSOR_PRIV (pCursor, pScreen)

char		 *xDisplayName = NULL;
Display		 *xdisplay = NULL;
int		 xscreen;
glitz_format_t	 *xglxCurrentFormat;
CARD32		 lastEventTime = 0;
ScreenPtr	 currentScreen = NULL;
Bool		 softCursor = FALSE;
xglScreenInfoRec xglScreenInfo = {
    NULL, 0, 0, 0, 0, FALSE,
    DEFAULT_GEOMETRY_DATA_TYPE,
    DEFAULT_GEOMETRY_USAGE,
    FALSE,
    XGL_DEFAULT_PBO_MASK
};

static Bool
xglxAllocatePrivates (ScreenPtr pScreen)
{
    xglxScreenPtr pScreenPriv;

    if (xglxScreenGeneration != serverGeneration)
    {
	xglxScreenPrivateIndex = AllocateScreenPrivateIndex ();
	if (xglxScreenPrivateIndex < 0)
	    return FALSE;
	
 	xglxScreenGeneration = serverGeneration;
    }

    pScreenPriv = xalloc (sizeof (xglxScreenRec));
    if (!pScreenPriv)
	return FALSE;
    
    XGLX_SET_SCREEN_PRIV (pScreen, pScreenPriv);
    
    return TRUE;
}

static void
xglxConstrainCursor (ScreenPtr pScreen,
		     BoxPtr    pBox)
{
}

static void
xglxCursorLimits (ScreenPtr pScreen,
		  CursorPtr pCursor,
		  BoxPtr    pHotBox,
		  BoxPtr    pTopLeftBox)
{
    *pTopLeftBox = *pHotBox;   
}

static Bool
xglxDisplayCursor (ScreenPtr pScreen,
		   CursorPtr pCursor)
{
    XGLX_SCREEN_PRIV (pScreen);
    XGLX_CURSOR_PRIV (pCursor, pScreen);
    
    XDefineCursor (xdisplay, pScreenPriv->win, pCursorPriv->cursor);
  
    return TRUE;
}

#ifdef ARGB_CURSOR

static Bool
xglxARGBCursorSupport (void);

static Cursor
xglxCreateARGBCursor (ScreenPtr pScreen,
		      CursorPtr pCursor);

#endif

static Bool
xglxRealizeCursor (ScreenPtr pScreen,
		   CursorPtr pCursor)
{
    xglxCursorPtr pCursorPriv;
    XImage	  *ximage;
    Pixmap	  source, mask;
    XColor	  fgColor, bgColor;
    GC		  xgc;
    unsigned long valuemask;
    XGCValues	  values;

    XGLX_SCREEN_PRIV (pScreen);

    valuemask = GCForeground | GCBackground;

    values.foreground = 1L;
    values.background = 0L;

    pCursorPriv = xalloc (sizeof (xglxCursorRec));
    if (!pCursorPriv)
	return FALSE;

    XGLX_SET_CURSOR_PRIV (pCursor, pScreen, pCursorPriv);

#ifdef ARGB_CURSOR
    if (pCursor->bits->argb)
    {
	pCursorPriv->cursor = xglxCreateARGBCursor (pScreen, pCursor);
	if (pCursorPriv->cursor)
	    return TRUE;
    }
#endif

    source = XCreatePixmap (xdisplay, 
			    pScreenPriv->win,
			    pCursor->bits->width,
			    pCursor->bits->height,
			    1);
  
    mask = XCreatePixmap (xdisplay, 
			  pScreenPriv->win,
			  pCursor->bits->width,
			  pCursor->bits->height,
			  1);
    
    xgc = XCreateGC (xdisplay, source, valuemask, &values);
  
    ximage = XCreateImage (xdisplay,
			   DefaultVisual (xdisplay, xscreen),
			   1, XYBitmap, 0,
			   (char *) pCursor->bits->source,
			   pCursor->bits->width,
			   pCursor->bits->height,
			   BitmapPad (xdisplay), 0);
  
    XPutImage (xdisplay, source, xgc, ximage,
	       0, 0, 0, 0, pCursor->bits->width, pCursor->bits->height);
  
    XFree (ximage);
  
    ximage = XCreateImage (xdisplay, 
			   DefaultVisual (xdisplay, xscreen),
			   1, XYBitmap, 0, 
			   (char *) pCursor->bits->mask,
			   pCursor->bits->width,
			   pCursor->bits->height,
			   BitmapPad (xdisplay), 0);
  
    XPutImage (xdisplay, mask, xgc, ximage,
	       0, 0, 0, 0, pCursor->bits->width, pCursor->bits->height);
  
    XFree (ximage);
    XFreeGC (xdisplay, xgc);
  
    fgColor.red   = pCursor->foreRed;
    fgColor.green = pCursor->foreGreen;
    fgColor.blue  = pCursor->foreBlue;
  
    bgColor.red   = pCursor->backRed;
    bgColor.green = pCursor->backGreen;
    bgColor.blue  = pCursor->backBlue;
  
    pCursorPriv->cursor = 
	XCreatePixmapCursor (xdisplay, source, mask, &fgColor, &bgColor,
			     pCursor->bits->xhot, pCursor->bits->yhot);

    XFreePixmap (xdisplay, mask);
    XFreePixmap (xdisplay, source);
  
    return TRUE;
}

static Bool
xglxUnrealizeCursor (ScreenPtr pScreen,
		     CursorPtr pCursor)
{
    XGLX_CURSOR_PRIV (pCursor, pScreen);
  
    XFreeCursor (xdisplay, pCursorPriv->cursor);
    xfree (pCursorPriv);
  
    return TRUE;
}

static void
xglxRecolorCursor (ScreenPtr pScreen,
		   CursorPtr pCursor,
		   Bool	     displayed)
{
    XColor fgColor, bgColor;

    XGLX_CURSOR_PRIV (pCursor, pScreen);
  
    fgColor.red   = pCursor->foreRed;
    fgColor.green = pCursor->foreGreen;
    fgColor.blue  = pCursor->foreBlue;
    
    bgColor.red   = pCursor->backRed;
    bgColor.green = pCursor->backGreen;
    bgColor.blue  = pCursor->backBlue;
  
    XRecolorCursor (xdisplay, pCursorPriv->cursor, &fgColor, &bgColor);
}

static Bool
xglxSetCursorPosition (ScreenPtr pScreen,
		       int	 x,
		       int	 y,
		       Bool	 generateEvent)
{
    XGLX_SCREEN_PRIV (pScreen);
    
    XWarpPointer (xdisplay, pScreenPriv->win, pScreenPriv->win,
		  0, 0, 0, 0, x, y);
    
    return TRUE;
}

static Bool
xglxCloseScreen (int	   index,
		 ScreenPtr pScreen)
{
    glitz_drawable_t *drawable;
    
    XGLX_SCREEN_PRIV (pScreen);

    drawable = XGL_GET_SCREEN_PRIV (pScreen)->drawable;
    if (drawable)
	glitz_drawable_destroy (drawable);

    xglClearVisualTypes ();

    if (pScreenPriv->win)
	XDestroyWindow (xdisplay, pScreenPriv->win);

    if (pScreenPriv->colormap)
	XFreeColormap (xdisplay, pScreenPriv->colormap);
    
    XGL_SCREEN_UNWRAP (CloseScreen);
    xfree (pScreenPriv);
    
    return (*pScreen->CloseScreen) (index, pScreen);
}

static Bool
xglxCursorOffScreen (ScreenPtr *ppScreen, int *x, int *y)
{
    return FALSE;
}

static void
xglxCrossScreen (ScreenPtr pScreen, Bool entering)
{
}

static void
xglxWarpCursor (ScreenPtr pScreen, int x, int y)
{
    miPointerWarpCursor (pScreen, x, y);
}

miPointerScreenFuncRec xglxPointerScreenFuncs = {
    xglxCursorOffScreen,
    xglxCrossScreen,
    xglxWarpCursor
};

static Bool
xglxScreenInit (int	  index,
		ScreenPtr pScreen,
		int	  argc,
		char	  **argv)
{
    XSetWindowAttributes    xswa;
    XWMHints		    *wmHints;
    XSizeHints		    *normalHints;
    XClassHint		    *classHint;
    xglxScreenPtr	    pScreenPriv;
    Window		    root;
    XVisualInfo		    *vinfo;
    XEvent		    xevent;
    glitz_drawable_format_t *format;
    glitz_drawable_t	    *drawable;
    
    format = xglVisuals[0].format;
	
    if (!xglxAllocatePrivates (pScreen))
	return FALSE;

    currentScreen = pScreen;

    pScreenPriv = XGLX_GET_SCREEN_PRIV (pScreen);

    root = RootWindow (xdisplay, xscreen);

    vinfo = glitz_glx_get_visual_info_from_format (xdisplay, xscreen, format);
    if (!vinfo)
    {
	ErrorF ("[%d] no visual info from format\n", index);
	return FALSE;
    }

    pScreenPriv->colormap =
	XCreateColormap (xdisplay, root, vinfo->visual, AllocNone);

    if (xglScreenInfo.fullscreen)
    {
	xglScreenInfo.width    = DisplayWidth (xdisplay, xscreen);
	xglScreenInfo.height   = DisplayHeight (xdisplay, xscreen);
	xglScreenInfo.widthMm  = DisplayWidthMM (xdisplay, xscreen);
	xglScreenInfo.heightMm = DisplayHeightMM (xdisplay, xscreen);
    }
    else if (xglScreenInfo.width == 0 || xglScreenInfo.height == 0)
    {
	xglScreenInfo.width  = XGLX_DEFAULT_SCREEN_WIDTH;
	xglScreenInfo.height = XGLX_DEFAULT_SCREEN_HEIGHT;
    }

    xswa.colormap = pScreenPriv->colormap;
    
    pScreenPriv->win =
	XCreateWindow (xdisplay, root, 0, 0,
		       xglScreenInfo.width, xglScreenInfo.height, 0,
		       vinfo->depth, InputOutput, vinfo->visual,
		       CWColormap, &xswa);
    
    XFree (vinfo);

    normalHints = XAllocSizeHints ();
    normalHints->flags      = PMinSize | PMaxSize | PSize;
    normalHints->min_width  = xglScreenInfo.width;
    normalHints->min_height = xglScreenInfo.height;
    normalHints->max_width  = xglScreenInfo.width;
    normalHints->max_height = xglScreenInfo.height;

    if (xglScreenInfo.fullscreen)
    {
	normalHints->x = 0;
	normalHints->y = 0;
	normalHints->flags |= PPosition;
    }

    classHint = XAllocClassHint ();
    classHint->res_name = "xglx";
    classHint->res_class = "Xglx";
    
    wmHints = XAllocWMHints ();
    wmHints->flags = InputHint;
    wmHints->input = TRUE;
    
    Xutf8SetWMProperties (xdisplay, pScreenPriv->win, "Xglx", "Xglx", 0, 0, 
			  normalHints, wmHints, classHint);
    
    XFree (wmHints);
    XFree (classHint);
    XFree (normalHints);

    drawable = glitz_glx_create_drawable_for_window (xdisplay, xscreen,
						     format, pScreenPriv->win,
						     xglScreenInfo.width,
						     xglScreenInfo.height);
    if (!drawable)
    {
	ErrorF ("[%d] couldn't create glitz drawable for window\n", index);
	return FALSE;
    }

    XSelectInput (xdisplay, pScreenPriv->win, ExposureMask);
    XMapWindow (xdisplay, pScreenPriv->win);

    if (xglScreenInfo.fullscreen)
    {
	XClientMessageEvent xev;
	
	memset (&xev, 0, sizeof (xev));
	
	xev.type = ClientMessage;
	xev.message_type = XInternAtom (xdisplay, "_NET_WM_STATE", FALSE);
	xev.display = xdisplay;
	xev.window = pScreenPriv->win;
	xev.format = 32;
	xev.data.l[0] = 1;
	xev.data.l[1] =
	    XInternAtom (xdisplay, "_NET_WM_STATE_FULLSCREEN", FALSE);
	
	XSendEvent (xdisplay, root, FALSE, SubstructureRedirectMask,
		    (XEvent *) &xev);
    }

    xglScreenInfo.drawable = drawable;
    
    if (!xglScreenInit (pScreen, &xglScreenInfo))
	return FALSE;
    
    XGL_SCREEN_WRAP (CloseScreen, xglxCloseScreen);

#ifdef ARGB_CURSOR
    if (!xglxARGBCursorSupport ())
	softCursor = TRUE;
#endif
    
    if (softCursor)
    {
	static char data = 0;
	XColor	    black, dummy;
	Pixmap	    bitmap;
	Cursor	    cursor;
	
	if (!XAllocNamedColor (xdisplay, pScreenPriv->colormap,
			       "black", &black, &dummy))
	    return FALSE;
	
	bitmap = XCreateBitmapFromData (xdisplay, pScreenPriv->win, &data,
					1, 1);
	if (!bitmap)
	    return FALSE;
	
	cursor = XCreatePixmapCursor (xdisplay, bitmap, bitmap, &black, &black,
				      0, 0);
	if (!cursor)
	    return FALSE;
	
	XDefineCursor (xdisplay, pScreenPriv->win, cursor);

	XFreeCursor (xdisplay, cursor);
	XFreePixmap (xdisplay, bitmap);
	XFreeColors (xdisplay, pScreenPriv->colormap, &black.pixel, 1, 0);

	miDCInitialize (pScreen, &xglxPointerScreenFuncs);
    }
    else
    {
	pScreen->ConstrainCursor   = xglxConstrainCursor;
	pScreen->CursorLimits      = xglxCursorLimits;
	pScreen->DisplayCursor     = xglxDisplayCursor;
	pScreen->RealizeCursor     = xglxRealizeCursor;
	pScreen->UnrealizeCursor   = xglxUnrealizeCursor;
	pScreen->RecolorCursor     = xglxRecolorCursor;
	pScreen->SetCursorPosition = xglxSetCursorPosition;
    }

    if (!xglFinishScreenInit (pScreen))
	return FALSE;
  
    while (XNextEvent (xdisplay, &xevent))
	if (xevent.type == Expose)
	    break;
    
    XSelectInput (xdisplay, pScreenPriv->win,
		  ButtonPressMask | ButtonReleaseMask |
		  KeyPressMask | KeyReleaseMask | EnterWindowMask |
		  PointerMotionMask);

    return TRUE;
}

void
InitOutput (ScreenInfo *pScreenInfo,
	    int	       argc,
	    char       **argv)
{
    glitz_drawable_format_t *format, templ;
    int			    i;
    unsigned long	    mask;
    unsigned long	    extraMask[] = {
	GLITZ_FORMAT_PBUFFER_MASK      |
	GLITZ_FORMAT_DOUBLEBUFFER_MASK,
	0
    };

    xglSetPixmapFormats (pScreenInfo);
    
    if (!xdisplay)
    {
	xdisplay = XOpenDisplay (xDisplayName);
	if (!xdisplay)
	    FatalError ("can't open display");

	xscreen = DefaultScreen (xdisplay);
    }

    templ.types.window     = 1;
    templ.types.pbuffer    = 1;
    templ.samples          = 1;
    templ.doublebuffer     = 1;
    templ.color.alpha_size = 8;
    
    mask =
	GLITZ_FORMAT_WINDOW_MASK |
	GLITZ_FORMAT_SAMPLES_MASK;

    for (i = 0; i < sizeof (extraMask) / sizeof (extraMask[0]); i++)
    {
	format = glitz_glx_find_drawable_format (xdisplay, xscreen,
						 mask | extraMask[i],
						 &templ, 0);
	if (format)
	    break;
    }
    
    if (!format)
	FatalError ("no visual format found");

    xglSetVisualTypesAndMasks (pScreenInfo, format, (1 << TrueColor));
    
    xglInitVisuals (pScreenInfo);
    
    AddScreen (xglxScreenInit, argc, argv);
}

static void
xglxBlockHandler (pointer   blockData,
		  OSTimePtr pTimeout,
		  pointer   pReadMask)
{
    XGL_SCREEN_PRIV (currentScreen);

    if (!xglSyncSurface (&pScreenPriv->pScreenPixmap->drawable))
	FatalError (XGL_SW_FAILURE_STRING);
    
    glitz_surface_flush (pScreenPriv->surface);
    glitz_drawable_finish (pScreenPriv->drawable);
    
    XSync (xdisplay, FALSE);
}

static void
xglxWakeupHandler (pointer blockData,
		   int     result,
		   pointer pReadMask)
{
    ScreenPtr pScreen = currentScreen;
    XEvent    X;
    xEvent    x;

    while (XPending (xdisplay)) {
	XNextEvent (xdisplay, &X);
	
	switch (X.type) {
	case KeyPress:
	    x.u.u.type = KeyPress;
	    x.u.u.detail = X.xkey.keycode;
	    x.u.keyButtonPointer.time = lastEventTime = GetTimeInMillis ();
	    mieqEnqueue (&x);
	    break;
	case KeyRelease:
	    x.u.u.type = KeyRelease;
	    x.u.u.detail = X.xkey.keycode;
	    x.u.keyButtonPointer.time = lastEventTime = GetTimeInMillis ();
	    mieqEnqueue (&x);
	    break;
	case ButtonPress:
	    x.u.u.type = ButtonPress;
	    x.u.u.detail = X.xbutton.button;
	    x.u.keyButtonPointer.time = lastEventTime = GetTimeInMillis ();
	    mieqEnqueue (&x);
	    break;
	case ButtonRelease:
	    x.u.u.type = ButtonRelease;
	    x.u.u.detail = X.xbutton.button;
	    x.u.keyButtonPointer.time = lastEventTime = GetTimeInMillis ();
	    mieqEnqueue (&x);
	    break;
	case MotionNotify:
	    x.u.u.type = MotionNotify;
	    x.u.keyButtonPointer.rootX = X.xmotion.x;
	    x.u.keyButtonPointer.rootY = X.xmotion.y;
	    x.u.keyButtonPointer.time = lastEventTime = GetTimeInMillis ();
	    miPointerAbsoluteCursor (X.xmotion.x, X.xmotion.y, lastEventTime);
	    mieqEnqueue (&x);
	    break;
	case EnterNotify:
	    if (X.xcrossing.detail != NotifyInferior) {
		if (pScreen) {
		    NewCurrentScreen (pScreen, X.xcrossing.x, X.xcrossing.y);
		    x.u.u.type = MotionNotify;
		    x.u.keyButtonPointer.rootX = X.xcrossing.x;
		    x.u.keyButtonPointer.rootY = X.xcrossing.y;
		    x.u.keyButtonPointer.time = lastEventTime =
			GetTimeInMillis ();
		    mieqEnqueue (&x);
		}
	    }
	    break;	    
	default:
	    break;
	}
    }
}

static void
xglxBell (int	       volume,
	  DeviceIntPtr pDev,
	  pointer      ctrl,
	  int	       cls)
{
  XBell (xdisplay, volume);
}

static int
xglxKeybdProc (DeviceIntPtr pDevice,
	       int	    onoff)
{
    Bool      ret;
    DevicePtr pDev = (DevicePtr) pDevice;

    if (!pDev)
	return BadImplementation;
    
    switch (onoff) {
    case DEVICE_INIT: {
      XModifierKeymap *xmodMap;
      KeySym	      *xkeyMap;
      int	      minKeyCode, maxKeyCode, mapWidth, i, j;
      KeySymsRec      xglxKeySyms;
      CARD8	      xglxModMap[256];
      XKeyboardState  values;

      if (pDev != LookupKeyboardDevice ())
	    return !Success;
      
      xmodMap = XGetModifierMapping (xdisplay);
      XDisplayKeycodes (xdisplay, &minKeyCode, &maxKeyCode);
      xkeyMap = XGetKeyboardMapping (xdisplay, 
				     minKeyCode,
				     maxKeyCode - minKeyCode + 1,
				     &mapWidth);
      
      memset (xglxModMap, 0, 256);
      
      for (j = 0; j < 8; j++)
      {
	  for (i = 0; i < xmodMap->max_keypermod; i++)
	  {
	      CARD8 keyCode;

	      keyCode = xmodMap->modifiermap[j * xmodMap->max_keypermod + i];
	      if (keyCode)
		  xglxModMap[keyCode] |= 1 << j;
	  }
      }
      
      XFreeModifiermap (xmodMap);
      
      xglxKeySyms.minKeyCode = minKeyCode;
      xglxKeySyms.maxKeyCode = maxKeyCode;
      xglxKeySyms.mapWidth   = mapWidth;
      xglxKeySyms.map	     = xkeyMap;

      XGetKeyboardControl (xdisplay, &values);

      memmove (defaultKeyboardControl.autoRepeats,
	       values.auto_repeats, sizeof (values.auto_repeats));

      ret = InitKeyboardDeviceStruct (pDev,
				      &xglxKeySyms,
				      xglxModMap,
				      xglxBell,
				      xglKbdCtrl);

      XFree (xkeyMap);

      if (!ret)
	  return BadImplementation;
    } break;
    case DEVICE_ON: 
	pDev->on = TRUE;
	break;
    case DEVICE_OFF: 
    case DEVICE_CLOSE:
	pDev->on = FALSE;
	break;
    }
    
    return Success;
}

Bool
LegalModifier (unsigned int key,
	       DevicePtr    pDev)
{
    return TRUE;
}

void
ProcessInputEvents ()
{
    mieqProcessInputEvents ();
    miPointerUpdate ();
}

void
InitInput (int argc, char **argv)
{
    DeviceIntPtr pKeyboard, pPointer;
    
    pPointer  = AddInputDevice (xglMouseProc, TRUE);
    pKeyboard = AddInputDevice (xglxKeybdProc, TRUE);
    
    RegisterPointerDevice (pPointer);
    RegisterKeyboardDevice (pKeyboard);
    
    miRegisterPointerDevice (screenInfo.screens[0], pPointer);
    mieqInit (&pKeyboard->public, &pPointer->public);
    
    AddEnabledDevice (XConnectionNumber (xdisplay));

    RegisterBlockAndWakeupHandlers (xglxBlockHandler,
				    xglxWakeupHandler,
				    NULL);
}

void
ddxUseMsg (void)
{
    ErrorF ("\nXglx usage:\n");
    ErrorF ("-display string        display name of the real server\n");
    ErrorF ("-softcursor            force software cursor\n");
    
    xglUseMsg ();
}

int
ddxProcessArgument (int argc, char **argv, int i)
{
    if (!strcmp (argv[i], "-display"))
    {
	if (++i < argc) {
	    xDisplayName = argv[i];
	    return 2;
	}
	return 0;
    }
    else if (!strcmp (argv[i], "-softcursor"))
    {
	softCursor = TRUE;
	return 1;
    }

    return xglProcessArgument (&xglScreenInfo, argc, argv, i);
}

void
AbortDDX (void)
{    
}

void
ddxGiveUp ()
{
    AbortDDX ();
}

void
OsVendorInit (void)
{
}

#ifdef ARGB_CURSOR

#include <X11/extensions/Xrender.h>

static Bool
xglxARGBCursorSupport (void)
{
    int renderMajor, renderMinor;
    
    if (!XRenderQueryVersion (xdisplay, &renderMajor, &renderMinor))
	renderMajor = renderMinor = -1;

    return (renderMajor > 0 || renderMinor > 4);
}   

static Cursor
xglxCreateARGBCursor (ScreenPtr pScreen,
		      CursorPtr pCursor)
{
    Pixmap	      xpixmap;
    GC		      xgc;
    XImage	      *ximage;
    XRenderPictFormat *xformat;
    Picture	      xpicture;
    Cursor	      cursor;

    XGLX_SCREEN_PRIV (pScreen);
    
    xpixmap = XCreatePixmap (xdisplay, 
			     pScreenPriv->win,
			     pCursor->bits->width,
			     pCursor->bits->height,
			     32);
	
    xgc = XCreateGC (xdisplay, xpixmap, 0, NULL);
    
    ximage = XCreateImage (xdisplay,
			   DefaultVisual (xdisplay, xscreen),
			   32, ZPixmap, 0,
			   (char *) pCursor->bits->argb,
			   pCursor->bits->width,
			   pCursor->bits->height,
			   32, pCursor->bits->width * 4);
    
    XPutImage (xdisplay, xpixmap, xgc, ximage,
	       0, 0, 0, 0, pCursor->bits->width, pCursor->bits->height);
    
    XFree (ximage);
    XFreeGC (xdisplay, xgc);
    
    xformat = XRenderFindStandardFormat (xdisplay, PictStandardARGB32);
    xpicture = XRenderCreatePicture (xdisplay, xpixmap, xformat, 0, 0);
    
    cursor = XRenderCreateCursor (xdisplay, xpicture,
				  pCursor->bits->xhot,
				  pCursor->bits->yhot);
    
    XRenderFreePicture (xdisplay, xpicture);
    XFreePixmap (xdisplay, xpixmap);
    
    return cursor;
}

#endif
