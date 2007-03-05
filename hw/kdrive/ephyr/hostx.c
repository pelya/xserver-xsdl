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
#include <X11/keysym.h>
#include <X11/extensions/XShm.h>

/*  
 * All xlib calls go here, which gets built as its own .a .
 * Mixing kdrive and xlib headers causes all sorts of types
 * to get clobbered. 
 */

struct EphyrHostXVars
{
  char           *server_dpy_name;
  Display        *dpy;
  int             screen;
  Visual         *visual;
  Window          win, winroot;
  Window          win_pre_existing; 	/* Set via -parent option like xnest */
  GC              gc;
  int             depth;
  int             server_depth;
  XImage         *ximg;
  int             win_width, win_height;
  Bool            use_host_cursor;
  Bool            use_fullscreen;
  Bool            have_shm;

  long            damage_debug_msec;

  unsigned char  *fb_data;   	/* only used when host bpp != server bpp */
  unsigned long   cmap[256];

  XShmSegmentInfo shminfo;
};

/* memset ( missing> ) instead of below  */
static EphyrHostXVars HostX = { "?", 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

static int            HostXWantDamageDebug = 0;

extern EphyrKeySyms   ephyrKeySyms;

extern int            monitorResolution;

static void
hostx_set_fullscreen_hint(void);

/* X Error traps */

static int trapped_error_code = 0;
static int (*old_error_handler) (Display *d, XErrorEvent *e);

#define host_depth_matches_server() (HostX.depth == HostX.server_depth)


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
 if (HostX.win_pre_existing != None
     || HostX.use_fullscreen == True)
    {
      *width  = HostX.win_width;
      *height = HostX.win_height;
      return 1;
    }

 return 0;
}

void
hostx_set_display_name(char *name)
{
  HostX.server_dpy_name = strdup(name);
}

void
hostx_set_win_title(char *extra_text)
{
  char buf[256];

  snprintf(buf, 256, "Xephyr on %s %s", 
	   HostX.server_dpy_name,
	   (extra_text != NULL) ? extra_text : "");

  XStoreName(HostX.dpy, HostX.win, buf);
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
hostx_use_fullscreen(void)
{
  HostX.use_fullscreen = True;
}

int
hostx_want_fullscreen(void)
{
  return HostX.use_fullscreen;
}

static void
hostx_set_fullscreen_hint(void)
{
  Atom atom_WINDOW_STATE, atom_WINDOW_STATE_FULLSCREEN;

  atom_WINDOW_STATE 
    = XInternAtom(HostX.dpy, "_NET_WM_STATE", False);
  atom_WINDOW_STATE_FULLSCREEN 
    = XInternAtom(HostX.dpy, "_NET_WM_STATE_FULLSCREEN",False);

  XChangeProperty(HostX.dpy, HostX.win,
		  atom_WINDOW_STATE, XA_ATOM, 32,
		  PropModeReplace,
		  (unsigned char *)&atom_WINDOW_STATE_FULLSCREEN, 1);
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

  HostX.server_depth = HostX.depth;
 
  if (HostX.win_pre_existing != None)
    {
      Status            result;
      XWindowAttributes prewin_attr;

      /* Get screen size from existing window */

      hostx_errors_trap();

      result = XGetWindowAttributes(HostX.dpy, 
				    HostX.win_pre_existing, 
				    &prewin_attr);


      if (hostx_errors_untrap() || !result)
	{
	  fprintf(stderr, "\nXephyr -parent window' does not exist!\n");
	  exit(1);
	}

      HostX.win_width  = prewin_attr.width;
      HostX.win_height = prewin_attr.height;

      HostX.win = XCreateWindow(HostX.dpy,
				HostX.win_pre_existing,
				0,0,HostX.win_width,HostX.win_height,
				0,
				CopyFromParent,
				CopyFromParent,
				CopyFromParent,
				CWEventMask,
				&attr);
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

      hostx_set_win_title("( ctrl+shift grabs mouse and keyboard )");

      if (HostX.use_fullscreen)
	{
	  HostX.win_width  = DisplayWidth(HostX.dpy, HostX.screen); 
	  HostX.win_height = DisplayHeight(HostX.dpy, HostX.screen); 

	  hostx_set_fullscreen_hint();
	}
    }


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
hostx_get_server_depth (void)
{
  return HostX.server_depth;
}

void
hostx_set_server_depth(int depth)
{
  HostX.server_depth = depth;
}

int
hostx_get_bpp(void)
{
  if (host_depth_matches_server())
    return  HostX.visual->bits_per_rgb;
  else
    return HostX.server_depth; 	/* XXX correct ? */
}

void
hostx_get_visual_masks (CARD32 *rmsk, 
			CARD32 *gmsk, 
			CARD32 *bmsk)
{
  if (host_depth_matches_server())
    {
      *rmsk = HostX.visual->red_mask;
      *gmsk = HostX.visual->green_mask;
      *bmsk = HostX.visual->blue_mask;
    }
  else if (HostX.server_depth == 16)
    {
      /* Assume 16bpp 565 */
      *rmsk = 0xf800;
      *gmsk = 0x07e0;
      *bmsk = 0x001f;
    }
  else
    {
      *rmsk = 0x0;
      *gmsk = 0x0;
      *bmsk = 0x0;
    }
}

void
hostx_set_cmap_entry(unsigned char idx, 
		     unsigned char r, 
		     unsigned char g, 
		     unsigned char b)
{
  /* XXX Will likely break for 8 on 16, not sure if this is correct */
  HostX.cmap[idx] = (r << 16) | (g << 8) | (b);
}

/**
 * hostx_screen_init creates the XImage that will contain the front buffer of
 * the ephyr screen, and possibly offscreen memory.
 *
 * @param width width of the screen
 * @param height height of the screen
 * @param buffer_height  height of the rectangle to be allocated.
 *
 * hostx_screen_init() creates an XImage, using MIT-SHM if it's available.
 * buffer_height can be used to create a larger offscreen buffer, which is used
 * by fakexa for storing offscreen pixmap data.
 */
void*
hostx_screen_init (int width, int height, int buffer_height)
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
				   width, buffer_height );
	  
      HostX.shminfo.shmid = shmget(IPC_PRIVATE,
				   HostX.ximg->bytes_per_line * buffer_height,
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
				 buffer_height, 
				 bitmap_pad, 
				 0);

      HostX.ximg->data = malloc( HostX.ximg->bytes_per_line * buffer_height );
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

  if (host_depth_matches_server())
    {
      EPHYR_DBG("Host matches server");
      return HostX.ximg->data;
    }
  else
    {
      EPHYR_DBG("server bpp %i", HostX.server_depth>>3);
      HostX.fb_data = malloc(width*buffer_height*(HostX.server_depth>>3));
      return HostX.fb_data;
    }
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

  /* 
   * If the depth of the ephyr server is less than that of the host,
   * the kdrive fb does not point to the ximage data but to a buffer
   * ( fb_data ), we shift the various bits from this onto the XImage
   * so they match the host.
   *
   * Note, This code is pretty new ( and simple ) so may break on 
   *       endian issues, 32 bpp host etc. 
   *       Not sure if 8bpp case is right either. 
   *       ... and it will be slower than the matching depth case.
   */

  if (!host_depth_matches_server())
    {
      int            x,y,idx, bytes_per_pixel = (HostX.server_depth>>3);
      unsigned char  r,g,b;
      unsigned long  host_pixel;

      for (y=sy; y<sy+height; y++)
	for (x=sx; x<sx+width; x++)
	  {
	    idx = (HostX.win_width*y*bytes_per_pixel)+(x*bytes_per_pixel);
	    
	    switch (HostX.server_depth)
	      {
	      case 16:
		{
		  unsigned short pixel = *(unsigned short*)(HostX.fb_data+idx);

		  r = ((pixel & 0xf800) >> 8);
		  g = ((pixel & 0x07e0) >> 3);
		  b = ((pixel & 0x001f) << 3);

		  host_pixel = (r << 16) | (g << 8) | (b);
		  
		  XPutPixel(HostX.ximg, x, y, host_pixel);
		  break;
		}
	      case 8:
		{
		  unsigned char pixel = *(unsigned char*)(HostX.fb_data+idx);
		  XPutPixel(HostX.ximg, x, y, HostX.cmap[pixel]);
		  break;
		}
	      default:
		break;
	      }
	  }
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

  /* fprintf(stderr, "Xephyr updating: %i+%i %ix%i\n", x, y, width, height); */

  XFillRectangle(HostX.dpy, HostX.win, HostX.gc, x, y, width,height); 
  XSync(HostX.dpy, False);

  /* nanosleep seems to work better than usleep for me... */
  nanosleep(&tspec, NULL);
}

void
hostx_load_keymap(void)
{
  XID             *keymap;
  int              host_width, min_keycode, max_keycode, width;
  int              i,j;

  XDisplayKeycodes(HostX.dpy, &min_keycode, &max_keycode);

  EPHYR_DBG("min: %d, max: %d", min_keycode, max_keycode);

  keymap = XGetKeyboardMapping(HostX.dpy, 
			       min_keycode,
			       max_keycode - min_keycode + 1,
			       &host_width);

  /* Try and copy the hosts keymap into our keymap to avoid loads
   * of messing around.
   *
   * kdrive cannot can have more than 4 keysyms per keycode
   * so we only copy at most the first 4 ( xorg has 6 per keycode, XVNC 2 )
   */
  width = (host_width > 4) ? 4 : host_width;

  ephyrKeySyms.map = (CARD32 *)calloc(sizeof(CARD32),
                                      (max_keycode - min_keycode + 1) *
                                      width);
  if (!ephyrKeySyms.map)
        return;

  for (i=0; i<(max_keycode - min_keycode+1); i++)
    for (j=0; j<width; j++)
      ephyrKeySyms.map[(i*width)+j] = (CARD32) keymap[(i*host_width) + j];

  EPHYR_DBG("keymap width, host:%d kdrive:%d", host_width, width);
  
  ephyrKeySyms.minKeyCode  = min_keycode;
  ephyrKeySyms.maxKeyCode  = max_keycode;
  ephyrKeySyms.mapWidth    = width;

  XFree(keymap);
}

int
hostx_get_event(EphyrHostXEvent *ev)
{
  XEvent      xev;
  static Bool grabbed;

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
	  ev->key_state = xev.xkey.state;
	  /* 
	   * This is a bit hacky. will break for button 5 ( defined as 0x10 )
           * Check KD_BUTTON defines in kdrive.h 
	   */
	  ev->data.mouse_down.button_num = 1<<(xev.xbutton.button-1);
	  return 1;

	case ButtonRelease:
	  ev->type = EPHYR_EV_MOUSE_RELEASE;
	  ev->key_state = xev.xkey.state;
	  ev->data.mouse_up.button_num = 1<<(xev.xbutton.button-1);
	  return 1;

	case KeyPress:
	  {
	    ev->type = EPHYR_EV_KEY_PRESS;
	    ev->key_state = xev.xkey.state;
	    ev->data.key_down.scancode = xev.xkey.keycode;  
	    return 1;
	  }
	case KeyRelease:

	  if ((XKeycodeToKeysym(HostX.dpy,xev.xkey.keycode,0) == XK_Shift_L
	       || XKeycodeToKeysym(HostX.dpy,xev.xkey.keycode,0) == XK_Shift_R)
	      && (xev.xkey.state & ControlMask))
	    {
	      if (grabbed) 
		{
		  XUngrabKeyboard (HostX.dpy, CurrentTime);
		  XUngrabPointer (HostX.dpy, CurrentTime);
		  grabbed = False;
		  hostx_set_win_title("( ctrl+shift grabs mouse and keyboard )");
		} 
	      else 
		{
		  /* Attempt grab */
		  if (XGrabKeyboard (HostX.dpy, HostX.win, True, 
				     GrabModeAsync, 
				     GrabModeAsync, 
				     CurrentTime) == 0)
		    {
		      if (XGrabPointer (HostX.dpy, HostX.win, True, 
					NoEventMask, 
					GrabModeAsync, 
					GrabModeAsync, 
					HostX.win, None, CurrentTime) == 0)
			{
			  grabbed = True;
			  hostx_set_win_title("( ctrl+shift releases mouse and keyboard )");
			}
		      else 	/* Failed pointer grabm  ungrab keyboard */
			XUngrabKeyboard (HostX.dpy, CurrentTime);
		    }
		}
	    }

	  /* Still send the release event even if above has happened
           * server will get confused with just an up event. 
           * Maybe it would be better to just block shift+ctrls getting to
           * kdrive all togeather. 
 	   */
	  ev->type = EPHYR_EV_KEY_RELEASE;
	  ev->key_state = xev.xkey.state;
	  ev->data.key_up.scancode = xev.xkey.keycode;
	  return 1;

	default:
	  break;

	}
    }
  return 0;
}

