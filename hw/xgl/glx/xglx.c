/*
 * Copyright © 2004 David Reveman
 * 
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the names of
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
 * Author: David Reveman <davidr@freedesktop.org>
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <glitz-glx.h>

#include "xgl.h"
#include "inputstr.h"
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

char		 *xDisplayName = NULL;
Display		 *xdisplay = NULL;
int		 xscreen;
glitz_format_t	 *xglxCurrentFormat;
CARD32		 lastEventTime = 0;
ScreenPtr	 currentScreen = NULL;
xglScreenInfoRec xglScreenInfo = { 0, 0, 0, 0, FALSE };

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
    return TRUE;
}

static Bool
xglxRealizeCursor (ScreenPtr pScreen,
		   CursorPtr pCursor)
{
    return TRUE;
}

static Bool
xglxUnrealizeCursor (ScreenPtr pScreen,
		     CursorPtr pCursor)
{
    return TRUE;
}

static void
xglxRecolorCursor (ScreenPtr pScreen,
		   CursorPtr pCursor,
		   Bool	     displayed)
{
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

    pScreen->ConstrainCursor   = xglxConstrainCursor;
    pScreen->CursorLimits      = xglxCursorLimits;
    pScreen->DisplayCursor     = xglxDisplayCursor;
    pScreen->RealizeCursor     = xglxRealizeCursor;
    pScreen->UnrealizeCursor   = xglxUnrealizeCursor;
    pScreen->RecolorCursor     = xglxRecolorCursor;
    pScreen->SetCursorPosition = xglxSetCursorPosition;

    XGL_SCREEN_WRAP (CloseScreen, xglxCloseScreen);

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
	GLITZ_FORMAT_DOUBLEBUFFER_MASK | GLITZ_FORMAT_ALPHA_SIZE_MASK,
	GLITZ_FORMAT_DOUBLEBUFFER_MASK | GLITZ_FORMAT_ALPHA_SIZE_MASK,
	GLITZ_FORMAT_ALPHA_SIZE_MASK,
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
    glitz_surface_flush (XGL_GET_SCREEN_PRIV (currentScreen)->surface);
    glitz_drawable_flush (XGL_GET_SCREEN_PRIV (currentScreen)->drawable);
    XFlush (xdisplay);
}

static void
xglxWakeupHandler (pointer blockData,
		   int     result,
		   pointer pReadMask)
{
    ScreenPtr pScreen = currentScreen;
    XEvent X;
    xEvent x;

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
    ErrorF ("\nXglx Usage:\n");
    ErrorF ("-display string        display name of the real server\n");
    
    xglUseMsg ();
}

int
ddxProcessArgument (int argc, char **argv, int i)
{
    if (!strcmp (argv[i], "-display")) {
	if (++i < argc) {
	    xDisplayName = argv[i];
	    return 2;
	}
	return 0;
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
