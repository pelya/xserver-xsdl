/*
 * Xephyr - A kdrive X server thats runs in a host X window.
 *          Authored by Matthew Allum <mallum@o-hand.com>
 * 
 * Copyright © 2004 Nokia 
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Nokia not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission. Nokia makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * NOKIA DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL NOKIA BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "hostx.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h> 		/* for memset */
#include <time.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/XShm.h>

/*  
 * All xlib calls go here, which gets built as its own .a .
 * Mixing kdrive and xlib headers causes all sorts of types
 * to get clobbered. 
 */

struct EphyrHostXVars
{
  Display        *dpy;
  int             screen;
  Visual         *visual;
  Window          win, winroot;
  Window          win_pre_existing; 	/* Set via -parent option like xnest */
  GC              gc;
  int             depth;
  XImage         *ximg;
  int             win_width, win_height;
  Bool            use_host_cursor;
  Bool            have_shm;
  long            damage_debug_msec;

  XShmSegmentInfo shminfo;
};

static EphyrHostXVars HostX = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0 }; /* defaults */

static int            HostXWantDamageDebug = 0;

extern KeySym         EphyrKeymap[];

extern KeySym	      kdKeymap[];
extern int	      kdMinScanCode;
extern int	      kdMaxScanCode;
extern int	      kdMinKeyCode;
extern int	      kdMaxKeyCode;
extern int	      kdKeymapWidth;
extern int            monitorResolution;

/* X Error traps */

static int trapped_error_code = 0;
static int (*old_error_handler) (Display *d, XErrorEvent *e);

static int
error_handler(Display     *display,
              XErrorEvent *error)
{
  trapped_error_code = error->error_code;
  return 0;
}

static void
hostx_errors_trap(void)
{
  trapped_error_code = 0;
  old_error_handler = XSetErrorHandler(error_handler);
}

static int
hostx_errors_untrap(void)
{
  XSetErrorHandler(old_error_handler);
  return trapped_error_code;
}

int
hostx_want_screen_size(int *width, int *height)
{
 if (HostX.win_pre_existing != None)
    {
      *width  = HostX.win_width;
      *height = HostX.win_height;
      return 1;
    }

 return 0;
}

int
hostx_want_host_cursor(void)
{
  return HostX.use_host_cursor;
}

void
hostx_use_host_cursor(void)
{
  HostX.use_host_cursor = True;
}

int
hostx_want_preexisting_window(void)
{
  if (HostX.win_pre_existing) 
    return 1;
  else
    return 0;
}

void
hostx_use_preexisting_window(unsigned long win_id)
{
  HostX.win_pre_existing = win_id;
}

static void
hostx_toggle_damage_debug(void)
{
  HostXWantDamageDebug ^= 1;
}

void 
hostx_handle_signal(int signum)
{
  hostx_toggle_damage_debug();
  EPHYR_DBG("Signal caught. Damage Debug:%i\n", HostXWantDamageDebug);
}

int
hostx_init(void)
{
  XSetWindowAttributes  attr;
  Cursor                empty_cursor;
  Pixmap                cursor_pxm;
  XColor                col;

  attr.event_mask = 
    ButtonPressMask
    |ButtonReleaseMask
    |PointerMotionMask
    |KeyPressMask
    |KeyReleaseMask
    |ExposureMask;

  EPHYR_DBG("mark");

  if ((HostX.dpy = XOpenDisplay(getenv("DISPLAY"))) == NULL)
  {
    fprintf(stderr, "\nXephyr cannot open host display. Is DISPLAY set?\n");
    exit(1);
  }

  HostX.screen  = DefaultScreen(HostX.dpy);
  HostX.winroot = RootWindow(HostX.dpy, HostX.screen);
  HostX.gc      = XCreateGC(HostX.dpy, HostX.winroot, 0, NULL);
  HostX.depth   = DefaultDepth(HostX.dpy, HostX.screen);
  HostX.visual  = DefaultVisual(HostX.dpy, HostX.screen); 
 
  /* old way of getting dpi 
  HostX.mm_per_pixel_vertical = (double)DisplayHeightMM(HostX.dpy, HostX.screen)
                                 / DisplayHeight(HostX.dpy, HostX.screen);

  HostX.mm_per_pixel_horizontal = (double)DisplayWidthMM(HostX.dpy, HostX.screen)
                                  / DisplayWidth(HostX.dpy, HostX.screen);
  */

  if (HostX.win_pre_existing != None)
    {
      Status            result;
      XWindowAttributes prewin_attr;

      /* Get screen size from existing window */

      HostX.win = HostX.win_pre_existing;

      hostx_errors_trap();

      result = XGetWindowAttributes(HostX.dpy, HostX.win, &prewin_attr);

      if (hostx_errors_untrap() || !result)
	{
	  fprintf(stderr, "\nXephyr -parent window' does not exist!\n");
	  exit(1);
	}

      HostX.win_width  = prewin_attr.width;
      HostX.win_height = prewin_attr.height;

      XSelectInput(HostX.dpy, HostX.win, attr.event_mask);
    }
  else
    {
      HostX.win = XCreateWindow(HostX.dpy,
				HostX.winroot,
				0,0,100,100, /* will resize  */
				0,
				CopyFromParent,
				CopyFromParent,
				CopyFromParent,
				CWEventMask,
				&attr);

      XStoreName(HostX.dpy, HostX.win, "Xephyr");
    }

  HostX.gc = XCreateGC(HostX.dpy, HostX.winroot, 0, NULL);

  XParseColor(HostX.dpy, DefaultColormap(HostX.dpy,HostX.screen), "red", &col);
  XAllocColor(HostX.dpy, DefaultColormap(HostX.dpy, HostX.screen), &col);
  XSetForeground(HostX.dpy, HostX.gc, col.pixel);

  if (!hostx_want_host_cursor())
    {
      /* Ditch the cursor, we provide our 'own' */
      cursor_pxm = XCreatePixmap (HostX.dpy, HostX.winroot, 1, 1, 1);
      memset (&col, 0, sizeof (col));
      empty_cursor = XCreatePixmapCursor (HostX.dpy, 
					  cursor_pxm, cursor_pxm, 
					  &col, &col, 1, 1);
      XDefineCursor (HostX.dpy, HostX.win, empty_cursor);
      XFreePixmap (HostX.dpy, cursor_pxm);
    }

  HostX.ximg   = NULL;

  /* Try to get share memory ximages for a little bit more speed */

  if (!XShmQueryExtension(HostX.dpy) || getenv("XEPHYR_NO_SHM")) 
    {
      fprintf(stderr, "\nXephyr unable to use SHM XImages\n");
      HostX.have_shm = False;
    } 
  else 		            
    {	
      /* Really really check we have shm - better way ?*/
      XShmSegmentInfo shminfo; 

      HostX.have_shm = True;

      shminfo.shmid=shmget(IPC_PRIVATE, 1, IPC_CREAT|0777);
      shminfo.shmaddr=shmat(shminfo.shmid,0,0);
      shminfo.readOnly=True;

      hostx_errors_trap();
      
      XShmAttach(HostX.dpy, &shminfo);
      XSync(HostX.dpy, False);

      if (hostx_errors_untrap())
	{
	  fprintf(stderr, "\nXephyr unable to use SHM XImages\n");
	  HostX.have_shm = False;
	}

      shmdt(shminfo.shmaddr);
      shmctl(shminfo.shmid, IPC_RMID, 0);
    }

  XFlush(HostX.dpy);

  /* Setup the pause time between paints when debugging updates */

  HostX.damage_debug_msec = 20000; /* 1/50 th of a second */

  if (getenv("XEPHYR_PAUSE"))
    {
      HostX.damage_debug_msec = strtol(getenv("XEPHYR_PAUSE"), NULL, 0);
      EPHYR_DBG("pause is %li\n", HostX.damage_debug_msec);
    }

  return 1;
}

int
hostx_get_depth (void)
{
  return HostX.depth;
}

int
hostx_get_bpp(void)
{
  return  HostX.visual->bits_per_rgb;
}

void
hostx_get_visual_masks (unsigned long *rmsk, 
		     unsigned long *gmsk, 
		     unsigned long *bmsk)
{
  *rmsk = HostX.visual->red_mask;
  *gmsk = HostX.visual->green_mask;
  *bmsk = HostX.visual->blue_mask;
}


void*
hostx_screen_init (int width, int height)
{
  int         bitmap_pad;
  Bool        shm_success = False;
  XSizeHints *size_hints;

  EPHYR_DBG("mark");

  if (HostX.ximg != NULL)
    {
      /* Free up the image data if previously used
       * i.ie called by server reset
       */

      if (HostX.have_shm)
	{
	  XShmDetach(HostX.dpy, &HostX.shminfo);
	  XDestroyImage (HostX.ximg);
	  shmdt(HostX.shminfo.shmaddr);
	  shmctl(HostX.shminfo.shmid, IPC_RMID, 0);
	}
      else
	{
	  if (HostX.ximg->data) 
	    {
	      free(HostX.ximg->data);
	      HostX.ximg->data = NULL;
	    } 

	  XDestroyImage(HostX.ximg);
	}
    }

  if (HostX.have_shm)
    {
      HostX.ximg = XShmCreateImage(HostX.dpy, HostX.visual, HostX.depth, 
				   ZPixmap, NULL, &HostX.shminfo,
				   width, height );
	  
      HostX.shminfo.shmid = shmget(IPC_PRIVATE,
				   HostX.ximg->bytes_per_line * height,
				   IPC_CREAT|0777);
      HostX.shminfo.shmaddr = HostX.ximg->data = shmat(HostX.shminfo.shmid,
						       0, 0);

      if (HostX.ximg->data == (char *)-1)
	{
	  EPHYR_DBG("Can't attach SHM Segment, falling back to plain XImages");
	  HostX.have_shm = False;
	  XDestroyImage(HostX.ximg);
	  shmctl(HostX.shminfo.shmid, IPC_RMID, 0);
	}
      else
	{
	  EPHYR_DBG("SHM segment attached");
	  HostX.shminfo.readOnly = False;
	  XShmAttach(HostX.dpy, &HostX.shminfo);
	  shm_success = True;
	}
    }

  if (!shm_success)
    {
      bitmap_pad = ( HostX.depth > 16 )? 32 : (( HostX.depth > 8 )? 16 : 8 );
	  
      HostX.ximg = XCreateImage( HostX.dpy, 
				 HostX.visual, 
				 HostX.depth, 
				 ZPixmap, 0, 0,
				 width, 
				 height, 
				 bitmap_pad, 
				 0);

      HostX.ximg->data = malloc( HostX.ximg->bytes_per_line * height );
    }


  XResizeWindow(HostX.dpy, HostX.win, width, height);

  /* Ask the WM to keep our size static */
  size_hints = XAllocSizeHints();
  size_hints->max_width = size_hints->min_width = width;
  size_hints->max_height = size_hints->min_height = height;
  size_hints->flags = PMinSize|PMaxSize;
  XSetWMNormalHints(HostX.dpy, HostX.win, size_hints);
  XFree(size_hints);

  XMapWindow(HostX.dpy, HostX.win);

  XSync(HostX.dpy, False);

  HostX.win_width  = width;
  HostX.win_height = height;

  return HostX.ximg->data;
}

void
hostx_paint_rect(int sx,    int sy,
		 int dx,    int dy, 
		 int width, int height)
{
  /* 
   *  Copy the image data updated by the shadow layer
   *  on to the window
   */

  if (HostXWantDamageDebug)
    {
      hostx_paint_debug_rect(dx, dy, width, height);
    }

  if (HostX.have_shm)
    {
      XShmPutImage(HostX.dpy, HostX.win, HostX.gc, HostX.ximg, 
		   sx, sy, dx, dy, width, height, False);
    }
  else
    {
      XPutImage(HostX.dpy, HostX.win, HostX.gc, HostX.ximg, 
		sx, sy, dx, dy, width, height);
    }

  XSync(HostX.dpy, False);
}

void
hostx_paint_debug_rect(int x,     int y, 
		       int width, int height)
{
  struct timespec tspec;

  tspec.tv_sec  = HostX.damage_debug_msec / (1000000);
  tspec.tv_nsec = (HostX.damage_debug_msec % 1000000) * 1000;

  EPHYR_DBG("msec: %li tv_sec %li, tv_msec %li", 
	    HostX.damage_debug_msec, tspec.tv_sec, tspec.tv_nsec);

  XFillRectangle(HostX.dpy, HostX.win, HostX.gc, x, y, width,height); 
  XSync(HostX.dpy, False);

  /* nanosleep seems to work better than usleep for me... */
  nanosleep(&tspec, NULL);
}

void
hostx_load_keymap(void)
{
  KeySym          *keymap;
  int              mapWidth, min_keycode, max_keycode;

  XDisplayKeycodes(HostX.dpy, &min_keycode, &max_keycode);

  EPHYR_DBG("min: %d, max: %d", min_keycode, max_keycode);

  keymap = XGetKeyboardMapping(HostX.dpy, 
			       min_keycode,
			       max_keycode - min_keycode + 1,
			       &mapWidth);

  memcpy (kdKeymap, keymap, 
	  (max_keycode - min_keycode + 1)*mapWidth*sizeof(KeySym));

  EPHYR_DBG("keymap width: %d", mapWidth);
  
  /* all kdrive vars - see kkeymap.c */

  kdMinScanCode = min_keycode;
  kdMaxScanCode = max_keycode;
  kdMinKeyCode  = min_keycode;
  kdMaxKeyCode  = max_keycode;
  kdKeymapWidth = mapWidth;

  XFree(keymap);
}

int
hostx_get_event(EphyrHostXEvent *ev)
{
  XEvent xev;

  if (XPending(HostX.dpy))
    {
      XNextEvent(HostX.dpy, &xev);

      switch (xev.type) 
	{
	case Expose:
	  /* Not so great event compression, but works ok */
	  while (XCheckTypedWindowEvent(HostX.dpy, xev.xexpose.window, 
					Expose, &xev));
	  hostx_paint_rect(0, 0, 0, 0, HostX.win_width, HostX.win_height);
	  return 0;

	case MotionNotify:
	  ev->type = EPHYR_EV_MOUSE_MOTION;
	  ev->data.mouse_motion.x = xev.xmotion.x; 
	  ev->data.mouse_motion.y = xev.xmotion.y;
	  return 1;

	case ButtonPress:
	  ev->type = EPHYR_EV_MOUSE_PRESS;
	  /* 
	   * This is a bit hacky. will break for button 5 ( defined as 0x10 )
           * Check KD_BUTTON defines in kdrive.h 
	   */
	  ev->data.mouse_down.button_num = 1<<(xev.xbutton.button-1);
	  return 1;

	case ButtonRelease:
	  ev->type = EPHYR_EV_MOUSE_RELEASE;
	  ev->data.mouse_up.button_num = 1<<(xev.xbutton.button-1);
	  return 1;

	case KeyPress:
	  {
	    ev->type = EPHYR_EV_KEY_PRESS;
	    ev->data.key_down.scancode = xev.xkey.keycode;  
	    return 1;
	  }
	case KeyRelease:
	  ev->type = EPHYR_EV_KEY_RELEASE;
	  ev->data.key_up.scancode = xev.xkey.keycode;

	  return 1;

	default:
	  break;

	}
    }
  return 0;
}

