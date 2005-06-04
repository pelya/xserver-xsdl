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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <math.h>

#include <glitz-egl.h>

#include "xgl.h"
#include "inputstr.h"
#include "cursorstr.h"
#include "mipointer.h"

#include "xegl.h"

#define XEGL_DEFAULT_SCREEN_WIDTH  800
#define XEGL_DEFAULT_SCREEN_HEIGHT 600

int xeglScreenGeneration = -1;
int xeglScreenPrivateIndex;

#define XEGL_GET_SCREEN_PRIV(pScreen) 				         \
    ((xeglScreenPtr) (pScreen)->devPrivates[xeglScreenPrivateIndex].ptr)

#define XEGL_SET_SCREEN_PRIV(pScreen, v)			       \
    ((pScreen)->devPrivates[xeglScreenPrivateIndex].ptr = (pointer) v)

#define XEGL_SCREEN_PRIV(pScreen)			       \
    xeglScreenPtr pScreenPriv = XEGL_GET_SCREEN_PRIV (pScreen)

typedef struct _xeglCursor {
    Cursor cursor;
} xeglCursorRec, *xeglCursorPtr;

#define XEGL_GET_CURSOR_PRIV(pCursor, pScreen)		   \
    ((xeglCursorPtr) (pCursor)->devPriv[(pScreen)->myNum])

#define XEGL_SET_CURSOR_PRIV(pCursor, pScreen, v)	 \
    ((pCursor)->devPriv[(pScreen)->myNum] = (pointer) v)

#define XEGL_CURSOR_PRIV(pCursor, pScreen)			        \
    xeglCursorPtr pCursorPriv = XEGL_GET_CURSOR_PRIV (pCursor, pScreen)

char		 *xDisplayName = NULL;
EGLDisplay	 xdisplay;
EGLScreenMESA    xscreen;
glitz_format_t	 *xeglCurrentFormat;
CARD32		 lastEventTime = 0;
ScreenPtr	 currentScreen = NULL;
Bool		 softCursor = TRUE;
xglScreenInfoRec xglScreenInfo = {
    NULL, 0, 0, 0, 0, FALSE,
    DEFAULT_GEOMETRY_DATA_TYPE,
    DEFAULT_GEOMETRY_USAGE,
    FALSE,
    XGL_DEFAULT_PBO_MASK,
    FALSE
};

extern miPointerScreenFuncRec kdPointerScreenFuncs;

static Bool
xeglAllocatePrivates (ScreenPtr pScreen)
{
    xeglScreenPtr pScreenPriv;

    if (xeglScreenGeneration != serverGeneration)
    {
	xeglScreenPrivateIndex = AllocateScreenPrivateIndex ();
	if (xeglScreenPrivateIndex < 0)
	    return FALSE;

 	xeglScreenGeneration = serverGeneration;
    }

    pScreenPriv = xalloc (sizeof (xeglScreenRec));
    if (!pScreenPriv)
	return FALSE;

    XEGL_SET_SCREEN_PRIV (pScreen, pScreenPriv);

    return TRUE;
}

static void
xeglConstrainCursor (ScreenPtr pScreen,
		     BoxPtr    pBox)
{
}

static void
xeglCursorLimits (ScreenPtr pScreen,
		  CursorPtr pCursor,
		  BoxPtr    pHotBox,
		  BoxPtr    pTopLeftBox)
{
    *pTopLeftBox = *pHotBox;
}

static Bool
xeglDisplayCursor (ScreenPtr pScreen,
		   CursorPtr pCursor)
{
#if 0
    XEGL_SCREEN_PRIV (pScreen);
    XEGL_CURSOR_PRIV (pCursor, pScreen);

    XDefineCursor (xdisplay, pScreenPriv->win, pCursorPriv->cursor);
#endif
    return TRUE;
}

#ifdef ARGB_CURSOR

static Bool
xeglARGBCursorSupport (void);

static Cursor
xeglCreateARGBCursor (ScreenPtr pScreen,
		      CursorPtr pCursor);

#endif

static Bool
xeglRealizeCursor (ScreenPtr pScreen,
		   CursorPtr pCursor)
{
#if 0
    xeglCursorPtr pCursorPriv;
    XImage	  *ximage;
    Pixmap	  source, mask;
    XColor	  fgColor, bgColor;
    GC		  xgc;
    unsigned long valuemask;
    XGCValues	  values;

    XEGL_SCREEN_PRIV (pScreen);

    valuemask = GCForeground | GCBackground;

    values.foreground = 1L;
    values.background = 0L;

    pCursorPriv = xalloc (sizeof (xeglCursorRec));
    if (!pCursorPriv)
	return FALSE;

    XEGL_SET_CURSOR_PRIV (pCursor, pScreen, pCursorPriv);

#ifdef ARGB_CURSOR
    if (pCursor->bits->argb)
    {
	pCursorPriv->cursor = xeglCreateARGBCursor (pScreen, pCursor);
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
#endif
    return TRUE;
}

static Bool
xeglUnrealizeCursor (ScreenPtr pScreen,
		     CursorPtr pCursor)
{
#if 0
    XEGL_CURSOR_PRIV (pCursor, pScreen);

    XFreeCursor (xdisplay, pCursorPriv->cursor);
    xfree (pCursorPriv);
#endif
    return TRUE;
}

static void
xeglRecolorCursor (ScreenPtr pScreen,
		   CursorPtr pCursor,
		   Bool	     displayed)
{
#if 0
    XColor fgColor, bgColor;

    XEGL_CURSOR_PRIV (pCursor, pScreen);

    fgColor.red   = pCursor->foreRed;
    fgColor.green = pCursor->foreGreen;
    fgColor.blue  = pCursor->foreBlue;

    bgColor.red   = pCursor->backRed;
    bgColor.green = pCursor->backGreen;
    bgColor.blue  = pCursor->backBlue;

    XRecolorCursor (xdisplay, pCursorPriv->cursor, &fgColor, &bgColor);
#endif
}

static Bool
xeglSetCursorPosition (ScreenPtr pScreen,
		       int	 x,
		       int	 y,
		       Bool	 generateEvent)
{
#if 0
    XEGL_SCREEN_PRIV (pScreen);

    XWarpPointer (xdisplay, pScreenPriv->win, pScreenPriv->win,
		  0, 0, 0, 0, x, y);
#endif
    return TRUE;
}

static Bool
xeglCloseScreen (int	   index,
		 ScreenPtr pScreen)
{
    glitz_drawable_t *drawable;

    XEGL_SCREEN_PRIV (pScreen);

    drawable = XGL_GET_SCREEN_PRIV (pScreen)->drawable;
    if (drawable)
	glitz_drawable_destroy (drawable);

    xglClearVisualTypes ();
#if 0
    if (pScreenPriv->win)
	XDestroyWindow (xdisplay, pScreenPriv->win);

    if (pScreenPriv->colormap)
	XFreeColormap (xdisplay, pScreenPriv->colormap);
#endif
    XGL_SCREEN_UNWRAP (CloseScreen);
    xfree (pScreenPriv);

    return (*pScreen->CloseScreen) (index, pScreen);
}

static Bool
xeglCursorOffScreen (ScreenPtr *ppScreen, int *x, int *y)
{
    return FALSE;
}

static void
xeglCrossScreen (ScreenPtr pScreen, Bool entering)
{
}

static void
xeglWarpCursor (ScreenPtr pScreen, int x, int y)
{
    miPointerWarpCursor (pScreen, x, y);
}

miPointerScreenFuncRec xeglPointerScreenFuncs = {
    xeglCursorOffScreen,
    xeglCrossScreen,
    xeglWarpCursor
};

static void
xeglMoveCursor(ScreenPtr pScreen, int x, int y)
{
}


#define FB_CUR_SETIMAGE 0x01
#define FB_CUR_SETPOS   0x02
#define FB_CUR_SETHOT   0x04
#define FB_CUR_SETCMAP  0x08
#define FB_CUR_SETSHAPE 0x10
#define FB_CUR_SETSIZE  0x20
#define FB_CUR_SETALL   0xFF

struct fbcurpos {
        unsigned short x, y;
};

struct fb_cmap_user {
        unsigned long start;                    /* First entry  */
        unsigned long len;                      /* Number of entries */
        unsigned short *red;              /* Red values   */
        unsigned short *green;
        unsigned short *blue;
        unsigned short *transp;           /* transparency, can be NULL */
};

struct fb_image_user {
        unsigned long dx;                       /* Where to place image */
        unsigned long dy;
        unsigned long width;                    /* Size of image */
        unsigned long height;
        unsigned long fg_color;                 /* Only used when a mono bitmap */
        unsigned long bg_color;
        unsigned char depth;                    /* Depth of the image */
        const char *data;        /* Pointer to image data */
        struct fb_cmap_user cmap;       /* color map info */
};

struct fb_cursor_user {
        unsigned short set;             /* what to set */
        unsigned short enable;          /* cursor on/off */
        unsigned short rop;             /* bitop operation */
        const char *mask;               /* cursor mask bits */
        struct fbcurpos hot;            /* cursor hot spot */
        struct fb_image_user image;     /* Cursor image */
};
#define FBIO_CURSOR            _IOWR('F', 0x08, struct fb_cursor_user)


static void
xeglSetCursor(ScreenPtr pScreen, CursorPtr pCursor, int x, int y)
{
#if 0
    int fd, err;
    struct fb_cursor_user cursor;

    fd = open("/dev/fb0", O_RDWR);
    memset(&cursor, 0, sizeof(cursor));
    cursor.set = FB_CUR_SETPOS;
    cursor.image.dx = 50;
    cursor.image.dy = 50;
    cursor.enable = 1;
    err = ioctl(fd, FBIO_CURSOR, &cursor);
    err = errno;
    printf("errno %d\n", err);
    close(fd);
#endif
}

miPointerSpriteFuncRec eglPointerSpriteFuncs = {
        xeglRealizeCursor,
        xeglUnrealizeCursor,
        xeglSetCursor,
        xeglMoveCursor,
};

static Bool
xeglScreenInit (int	  index,
		ScreenPtr pScreen,
		int	  argc,
		char	  **argv)
{
    EGLSurface screen_surf;
    EGLModeMESA mode;
    int count;
    xeglScreenPtr	    pScreenPriv;
    glitz_drawable_format_t *format;
    glitz_drawable_t	    *drawable;
    const EGLint screenAttribs[] = {
        EGL_WIDTH, 1024,
        EGL_HEIGHT, 768,
        EGL_NONE
    };

    xglScreenInfo.width = 1024;
    xglScreenInfo.height = 768;

    format = xglVisuals[0].format;

    if (!xeglAllocatePrivates (pScreen))
	return FALSE;

    currentScreen = pScreen;

    pScreenPriv = XEGL_GET_SCREEN_PRIV (pScreen);

    if (xglScreenInfo.fullscreen)
    {
//	xglScreenInfo.width    = DisplayWidth (xdisplay, xscreen);
//	xglScreenInfo.height   = DisplayHeight (xdisplay, xscreen);
//	xglScreenInfo.widthMm  = DisplayWidthMM (xdisplay, xscreen);
//	xglScreenInfo.heightMm = DisplayHeightMM (xdisplay, xscreen);
    }
    else if (xglScreenInfo.width == 0 || xglScreenInfo.height == 0)
    {
	xglScreenInfo.width  = XEGL_DEFAULT_SCREEN_WIDTH;
	xglScreenInfo.height = XEGL_DEFAULT_SCREEN_HEIGHT;
    }

    eglGetModesMESA(xdisplay, xscreen, &mode, 1, &count);
    screen_surf = eglCreateScreenSurfaceMESA(xdisplay, format->id, screenAttribs);
    if (screen_surf == EGL_NO_SURFACE) {
        printf("failed to create screen surface\n");
        return FALSE;
    }

    eglShowSurfaceMESA(xdisplay, xscreen, screen_surf, mode);

    drawable = glitz_egl_create_surface (xdisplay, xscreen, format, screen_surf,
                                         xglScreenInfo.width, xglScreenInfo.height);
    if (!drawable)
    {
	ErrorF ("[%d] couldn't create glitz drawable for window\n", index);
	return FALSE;
    }

//    XSelectInput (xdisplay, pScreenPriv->win, ExposureMask);
//    XMapWindow (xdisplay, pScreenPriv->win);

    if (xglScreenInfo.fullscreen)
    {
#if 0
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
#endif
    }

    xglScreenInfo.drawable = drawable;

    if (!xglScreenInit (pScreen, &xglScreenInfo))
	return FALSE;

    XGL_SCREEN_WRAP (CloseScreen, xeglCloseScreen);

#ifdef ARGB_CURSOR
    if (!xeglARGBCursorSupport ())
	softCursor = TRUE;
#endif
    miDCInitialize (pScreen, &xeglPointerScreenFuncs);

//  miPointerInitialize(pScreen, &eglPointerSpriteFuncs,
//    &kdPointerScreenFuncs, FALSE);

    if (!xglFinishScreenInit (pScreen))
	return FALSE;

//    while (XNextEvent (xdisplay, &xevent))
//	if (xevent.type == Expose)
//	    break;

//    XSelectInput (xdisplay, pScreenPriv->win,
//		  ButtonPressMask | ButtonReleaseMask |
//		  KeyPressMask | KeyReleaseMask | EnterWindowMask |
//		  PointerMotionMask);

    return TRUE;
}

void
InitOutput (ScreenInfo *pScreenInfo,
	    int	       argc,
	    char       **argv)
{
    glitz_drawable_format_t *format, templ;
    int i, maj, min, count;
    unsigned long	    mask;
    unsigned long	    extraMask[] = {
	GLITZ_FORMAT_PBUFFER_MASK      |
	GLITZ_FORMAT_DOUBLEBUFFER_MASK,
	0
    };

    xglSetPixmapFormats (pScreenInfo);

    if (!xdisplay)
    {
        xdisplay = eglGetDisplay("!fb_dri");
        assert(xdisplay);

        if (!eglInitialize(xdisplay, &maj, &min))
	    FatalError ("can't open display");

        eglGetScreensMESA(xdisplay, &xscreen, 1, &count);

        glitz_egl_init (NULL);
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
	format = glitz_egl_find_config (xdisplay, xscreen,
						 mask | extraMask[i],
						 &templ, 0);
	if (format)
	    break;
    }

    if (!format)
	FatalError ("no visual format found");

    xglSetVisualTypesAndMasks (pScreenInfo, format, (1 << TrueColor));

    xglInitVisuals (pScreenInfo);

    AddScreen (xeglScreenInit, argc, argv);
}

static void
xeglBlockHandler (pointer   blockData,
		  OSTimePtr pTimeout,
		  pointer   pReadMask)
{
    XGL_SCREEN_PRIV (currentScreen);

    if (!xglSyncSurface (&pScreenPriv->pScreenPixmap->drawable))
	FatalError (XGL_SW_FAILURE_STRING);

    glitz_surface_flush (pScreenPriv->surface);
    glitz_drawable_finish (pScreenPriv->drawable);

//  XSync (xdisplay, FALSE);
}

static void
xeglWakeupHandler (pointer blockData,
		   int     result,
		   pointer pReadMask)
{
#if 0
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
#endif
}

void
InitInput (int argc, char **argv)
{
    eglInitInput (&LinuxEvdevMouseFuncs, &LinuxEvdevKeyboardFuncs);
    RegisterBlockAndWakeupHandlers (xeglBlockHandler,
                                    KdWakeupHandler,
                                    NULL);
}

void
ddxUseMsg (void)
{
    ErrorF ("\nXegl usage:\n");
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

//#include <X11/extensions/Xrender.h>

static Bool
xeglARGBCursorSupport (void)
{
#if 0
    int renderMajor, renderMinor;

    if (!XRenderQueryVersion (xdisplay, &renderMajor, &renderMinor))
	renderMajor = renderMinor = -1;

    return (renderMajor > 0 || renderMinor > 4);
#endif
    return TRUE;
}

static Cursor
xeglCreateARGBCursor (ScreenPtr pScreen,
		      CursorPtr pCursor)
{
    Cursor            cursor;
#if 0
    Pixmap	      xpixmap;
    GC		      xgc;
    XImage	      *ximage;
    XRenderPictFormat *xformat;
    Picture	      xpicture;

    XEGL_SCREEN_PRIV (pScreen);

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
#endif
    return cursor;
}

#endif
