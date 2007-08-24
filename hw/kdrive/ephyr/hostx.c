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

struct EphyrHostScreen
{
  Window          win;
  Window          win_pre_existing; 	/* Set via -parent option like xnest */
  XImage         *ximg;
  int             win_width, win_height;
  int             server_depth;
  unsigned char  *fb_data;   	/* only used when host bpp != server bpp */
  XShmSegmentInfo shminfo;

  void           *info;   /* Pointer to the screen this is associated with */
  int             mynum;  /* Screen number */
};

struct EphyrHostXVars
{
  char           *server_dpy_name;
  Display        *dpy;
  int             screen;
  Visual         *visual;
  Window          winroot;
  GC              gc;
  int             depth;
  Bool            use_host_cursor;
  Bool            use_fullscreen;
  Bool            have_shm;

  int             n_screens;
  struct EphyrHostScreen *screens;

  long            damage_debug_msec;

  unsigned long   cmap[256];
};

/* memset ( missing> ) instead of below  */
/*static EphyrHostXVars HostX = { "?", 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};*/
static EphyrHostXVars HostX;

static int            HostXWantDamageDebug = 0;

extern EphyrKeySyms   ephyrKeySyms;

extern int            monitorResolution;

static void
hostx_set_fullscreen_hint(void);

/* X Error traps */

static int trapped_error_code = 0;
static int (*old_error_handler) (Display *d, XErrorEvent *e);

#define host_depth_matches_server(_vars) (HostX.depth == (_vars)->server_depth)

static struct EphyrHostScreen *
host_screen_from_screen_info (EphyrScreenInfo *screen)
{
  int i;

  for (i = 0 ; i < HostX.n_screens ; i++)
    {
      if ( HostX.screens[i].info == screen)
        {
          return &HostX.screens[i];
        }
    }
  return NULL;
}

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
hostx_want_screen_size (EphyrScreenInfo screen, int *width, int *height )
{
  struct EphyrHostScreen *host_screen = host_screen_from_screen_info (screen);

  if (host_screen &&
       (host_screen->win_pre_existing != None ||
         HostX.use_fullscreen == True))
    {
      *width  = host_screen->win_width;
      *height = host_screen->win_height;
      return 1;
    }

 return 0;
}

void
hostx_add_screen (EphyrScreenInfo screen,
                  unsigned long win_id,
                  int screen_num)
{
  int index = HostX.n_screens;

  HostX.n_screens += 1;
  HostX.screens = realloc (HostX.screens,
                           HostX.n_screens * sizeof(struct EphyrHostScreen));
  memset (&HostX.screens[index], 0, sizeof (struct EphyrHostScreen));

  HostX.screens[index].info       = screen;
  HostX.screens[index].win_pre_existing = win_id;
}


void
hostx_set_display_name (char *name)
{
  HostX.server_dpy_name = strdup (name);
}

void
hostx_set_screen_number(EphyrScreenInfo screen, int number)
{
  struct EphyrHostScreen *host_screen = host_screen_from_screen_info (screen);
  if (host_screen) {
    host_screen->mynum = number;
    hostx_set_win_title (host_screen->info, "") ;
  }}

void
hostx_set_win_title (EphyrScreenInfo screen, char *extra_text)
{
  struct EphyrHostScreen *host_screen = host_screen_from_screen_info (screen);
#define BUF_LEN 256
  char buf[BUF_LEN+1];

  if (!host_screen)
    return;

  memset (buf, 0, BUF_LEN+1) ;
  snprintf (buf, BUF_LEN, "Xephyr on %s.%d %s", 
            HostX.server_dpy_name, 
            host_screen->mynum,
            (extra_text != NULL) ? extra_text : "");

  XStoreName (HostX.dpy, host_screen->win, buf);
}

int
hostx_want_host_cursor (void)
{
  return HostX.use_host_cursor;
}

void
hostx_use_host_cursor (void)
{
  HostX.use_host_cursor = True;
}

int
hostx_want_preexisting_window (EphyrScreenInfo screen)
{
  struct EphyrHostScreen *host_screen = host_screen_from_screen_info (screen);

  if (host_screen && host_screen->win_pre_existing)
    {
      return 1;
    }
  else
    {
    return 0;
    }
}

void
hostx_use_fullscreen (void)
{
  HostX.use_fullscreen = True;
}

int
hostx_want_fullscreen (void)
{
  return HostX.use_fullscreen;
}

static void
hostx_set_fullscreen_hint (void)
{
  Atom atom_WINDOW_STATE, atom_WINDOW_STATE_FULLSCREEN;
  int index;

  atom_WINDOW_STATE 
    = XInternAtom(HostX.dpy, "_NET_WM_STATE", False);
  atom_WINDOW_STATE_FULLSCREEN 
    = XInternAtom(HostX.dpy, "_NET_WM_STATE_FULLSCREEN",False);

  for (index = 0 ; index < HostX.n_screens ; index++)
    {
      XChangeProperty (HostX.dpy, HostX.screens[index].win,
                       atom_WINDOW_STATE, XA_ATOM, 32,
                       PropModeReplace,
                       (unsigned char *)&atom_WINDOW_STATE_FULLSCREEN, 1);
    }
}


static void
hostx_toggle_damage_debug (void)
{
  HostXWantDamageDebug ^= 1;
}

void
hostx_handle_signal (int signum)
{
  hostx_toggle_damage_debug();
  EPHYR_DBG ("Signal caught. Damage Debug:%i\n",
              HostXWantDamageDebug);
}

int
hostx_init (void)
{
  XSetWindowAttributes  attr;
  Cursor                empty_cursor;
  Pixmap                cursor_pxm;
  XColor                col;
  int                   index;

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

  for (index = 0 ; index < HostX.n_screens ; index++)
    {
      struct EphyrHostScreen *host_screen = &HostX.screens[index];

      host_screen->server_depth = HostX.depth;
      if (host_screen->win_pre_existing != None)
        {
          Status            result;
          XWindowAttributes prewin_attr;

          /* Get screen size from existing window */

          hostx_errors_trap();

          result = XGetWindowAttributes (HostX.dpy,
                                         host_screen->win_pre_existing,
                                         &prewin_attr);


          if (hostx_errors_untrap() || !result)
          {
              fprintf (stderr, "\nXephyr -parent window' does not exist!\n");
              exit (1);
          }

          host_screen->win_width  = prewin_attr.width;
          host_screen->win_height = prewin_attr.height;

          host_screen->win = XCreateWindow (HostX.dpy,
                                            host_screen->win_pre_existing,
                                            0,0,
                                            host_screen->win_width,
                                            host_screen->win_height,
                                            0,
                                            CopyFromParent,
                                            CopyFromParent,
                                            CopyFromParent,
                                            CWEventMask,
                                            &attr);
        }
      else
        {
          host_screen->win = XCreateWindow (HostX.dpy,
                                            HostX.winroot,
                                            0,0,100,100, /* will resize */
                                            0,
                                            CopyFromParent,
                                            CopyFromParent,
                                            CopyFromParent,
                                            CWEventMask,
                                            &attr);

          hostx_set_win_title (host_screen->info,
                               "(ctrl+shift grabs mouse and keyboard)");

          if (HostX.use_fullscreen)
            {
              host_screen->win_width  = DisplayWidth(HostX.dpy, HostX.screen);
              host_screen->win_height = DisplayHeight(HostX.dpy, HostX.screen);

              hostx_set_fullscreen_hint();
            }
        }
    }


  XParseColor (HostX.dpy, DefaultColormap (HostX.dpy,HostX.screen),
               "red", &col);
  XAllocColor (HostX.dpy, DefaultColormap (HostX.dpy, HostX.screen),
               &col);
  XSetForeground (HostX.dpy, HostX.gc, col.pixel);

  if (!hostx_want_host_cursor ())
    {
      /* Ditch the cursor, we provide our 'own' */
      cursor_pxm = XCreatePixmap (HostX.dpy, HostX.winroot, 1, 1, 1);
      memset (&col, 0, sizeof (col));
      empty_cursor = XCreatePixmapCursor (HostX.dpy,
                                          cursor_pxm, cursor_pxm, 
                                          &col, &col, 1, 1);
      for ( index = 0 ; index < HostX.n_screens ; index++ )
        {
          XDefineCursor (HostX.dpy,
                         HostX.screens[index].win,
                         empty_cursor);
        }
      XFreePixmap (HostX.dpy, cursor_pxm);
    }

  for (index = 0 ; index < HostX.n_screens ; index++)
    {
      HostX.screens[index].ximg   = NULL;
    }
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

  if (getenv ("XEPHYR_PAUSE"))
    {
      HostX.damage_debug_msec = strtol (getenv ("XEPHYR_PAUSE"), NULL, 0);
      EPHYR_DBG ("pause is %li\n", HostX.damage_debug_msec);
    }

  return 1;
}

int
hostx_get_depth (void)
{
  return HostX.depth;
}

int
hostx_get_server_depth (EphyrScreenInfo screen)
{
  struct EphyrHostScreen *host_screen = host_screen_from_screen_info (screen);

  return (host_screen ? host_screen->server_depth : 0);
}

void
hostx_set_server_depth (EphyrScreenInfo screen, int depth)
{
  struct EphyrHostScreen *host_screen = host_screen_from_screen_info (screen);

  if (host_screen)
    host_screen->server_depth = depth;
}

int
hostx_get_bpp (EphyrScreenInfo screen)
{
  struct EphyrHostScreen *host_screen = host_screen_from_screen_info (screen);

  if (!host_screen)
    return 0;

  if (host_depth_matches_server (host_screen))
    return HostX.visual->bits_per_rgb;
  else
    return host_screen->server_depth; /*XXX correct ?*/
}

void
hostx_get_visual_masks (EphyrScreenInfo screen,
			CARD32 *rmsk,
			CARD32 *gmsk,
			CARD32 *bmsk)
{
  struct EphyrHostScreen *host_screen = host_screen_from_screen_info (screen);

  if (!host_screen)
    return;

  if (host_depth_matches_server(host_screen))
    {
      *rmsk = HostX.visual->red_mask;
      *gmsk = HostX.visual->green_mask;
      *bmsk = HostX.visual->blue_mask;
    }
  else if (host_screen->server_depth == 16)
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
hostx_screen_init (EphyrScreenInfo screen,
                   int width, int height,
                   int buffer_height)
{
  int         bitmap_pad;
  Bool        shm_success = False;
  XSizeHints *size_hints;

  struct EphyrHostScreen *host_screen = host_screen_from_screen_info (screen);
  if (!host_screen)
    {
      fprintf (stderr, "%s: Error in accessing hostx data\n", __func__ );
      exit(1);
    }

  EPHYR_DBG ("host_screen=%p wxh=%dx%d, buffer_height=%d",
             host_screen, width, height, buffer_height);

  if (host_screen->ximg != NULL)
    {
      /* Free up the image data if previously used
       * i.ie called by server reset
       */

      if (HostX.have_shm)
	{
	  XShmDetach(HostX.dpy, &host_screen->shminfo);
	  XDestroyImage (host_screen->ximg);
	  shmdt(host_screen->shminfo.shmaddr);
	  shmctl(host_screen->shminfo.shmid, IPC_RMID, 0);
	}
      else
	{
	  if (host_screen->ximg->data) 
	    {
	      free(host_screen->ximg->data);
	      host_screen->ximg->data = NULL;
	    } 

	  XDestroyImage(host_screen->ximg);
	}
    }

  if (HostX.have_shm)
    {
      host_screen->ximg = XShmCreateImage (HostX.dpy, HostX.visual, HostX.depth,
                                           ZPixmap, NULL, &host_screen->shminfo,
                                           width, buffer_height );

      host_screen->shminfo.shmid =
                      shmget(IPC_PRIVATE,
                             host_screen->ximg->bytes_per_line * buffer_height,
                             IPC_CREAT|0777);
      host_screen->ximg->data = shmat(host_screen->shminfo.shmid, 0, 0);
      host_screen->shminfo.shmaddr = host_screen->ximg->data;

      if (host_screen->ximg->data == (char *)-1)
	{
	  EPHYR_DBG("Can't attach SHM Segment, falling back to plain XImages");
	  HostX.have_shm = False;
	  XDestroyImage(host_screen->ximg);
	  shmctl(host_screen->shminfo.shmid, IPC_RMID, 0);
	}
      else
	{
	  EPHYR_DBG("SHM segment attached %p", host_screen->shminfo.shmaddr);
	  host_screen->shminfo.readOnly = False;
	  XShmAttach(HostX.dpy, &host_screen->shminfo);
	  shm_success = True;
	}
    }

  if (!shm_success)
    {
      bitmap_pad = ( HostX.depth > 16 )? 32 : (( HostX.depth > 8 )? 16 : 8 );

      EPHYR_DBG("Creating image %dx%d for screen host_screen=%p\n",
                width, buffer_height, host_screen );
      host_screen->ximg = XCreateImage (HostX.dpy,
                                        HostX.visual,
                                        HostX.depth,
                                        ZPixmap, 0, 0,
                                        width,
                                        buffer_height,
                                        bitmap_pad,
                                        0);

      host_screen->ximg->data =
              malloc (host_screen->ximg->bytes_per_line * buffer_height);
    }

  XResizeWindow (HostX.dpy, host_screen->win, width, height);

  /* Ask the WM to keep our size static */
  size_hints = XAllocSizeHints();
  size_hints->max_width = size_hints->min_width = width;
  size_hints->max_height = size_hints->min_height = height;
  size_hints->flags = PMinSize|PMaxSize;
  XSetWMNormalHints(HostX.dpy, host_screen->win, size_hints);
  XFree(size_hints);

  XMapWindow(HostX.dpy, host_screen->win);

  XSync(HostX.dpy, False);

  host_screen->win_width  = width;
  host_screen->win_height = height;

  if (host_depth_matches_server(host_screen))
    {
      EPHYR_DBG("Host matches server");
      return host_screen->ximg->data;
    }
  else
    {
      EPHYR_DBG("server bpp %i", host_screen->server_depth>>3);
      host_screen->fb_data = malloc(width*buffer_height*(host_screen->server_depth>>3));
      return host_screen->fb_data;
    }
}

static void hostx_paint_debug_rect (struct EphyrHostScreen *host_screen,
                                    int x,     int y,
                                    int width, int height);

void
hostx_paint_rect (EphyrScreenInfo screen,
                  int sx,    int sy,
                  int dx,    int dy,
                  int width, int height)
{
  struct EphyrHostScreen *host_screen = host_screen_from_screen_info (screen);

  EPHYR_DBG ("painting in screen %d\n", host_screen->mynum) ;

  /*
   *  Copy the image data updated by the shadow layer
   *  on to the window
   */

  if (HostXWantDamageDebug)
    {
      hostx_paint_debug_rect(host_screen, dx, dy, width, height);
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

  if (!host_depth_matches_server(host_screen))
    {
      int            x,y,idx, bytes_per_pixel = (host_screen->server_depth>>3);
      unsigned char  r,g,b;
      unsigned long  host_pixel;

      EPHYR_DBG("Unmatched host depth host_screen=%p\n", host_screen);
      for (y=sy; y<sy+height; y++)
	for (x=sx; x<sx+width; x++)
	  {
	    idx = (host_screen->win_width*y*bytes_per_pixel)+(x*bytes_per_pixel);

	    switch (host_screen->server_depth)
	      {
	      case 16:
		{
		  unsigned short pixel = *(unsigned short*)(host_screen->fb_data+idx);

		  r = ((pixel & 0xf800) >> 8);
		  g = ((pixel & 0x07e0) >> 3);
		  b = ((pixel & 0x001f) << 3);

		  host_pixel = (r << 16) | (g << 8) | (b);
		  
		  XPutPixel(host_screen->ximg, x, y, host_pixel);
		  break;
		}
	      case 8:
		{
		  unsigned char pixel = *(unsigned char*)(host_screen->fb_data+idx);
		  XPutPixel(host_screen->ximg, x, y, HostX.cmap[pixel]);
		  break;
		}
	      default:
		break;
	      }
	  }
    }

  if (HostX.have_shm)
    {
      XShmPutImage (HostX.dpy, host_screen->win,
                    HostX.gc, host_screen->ximg,
                    sx, sy, dx, dy, width, height, False);
    }
  else
    {
      XPutImage (HostX.dpy, host_screen->win, HostX.gc, host_screen->ximg, 
                 sx, sy, dx, dy, width, height);
    }

  XSync (HostX.dpy, False);
}

static void
hostx_paint_debug_rect (struct EphyrHostScreen *host_screen,
                        int x,     int y,
                        int width, int height)
{
  struct timespec tspec;

  tspec.tv_sec  = HostX.damage_debug_msec / (1000000);
  tspec.tv_nsec = (HostX.damage_debug_msec % 1000000) * 1000;

  EPHYR_DBG("msec: %li tv_sec %li, tv_msec %li", 
	    HostX.damage_debug_msec, tspec.tv_sec, tspec.tv_nsec);

  /* fprintf(stderr, "Xephyr updating: %i+%i %ix%i\n", x, y, width, height); */

  XFillRectangle (HostX.dpy, host_screen->win, HostX.gc, x, y, width,height);
  XSync (HostX.dpy, False);

  /* nanosleep seems to work better than usleep for me... */
  nanosleep(&tspec, NULL);
}

void
hostx_load_keymap(void)
{
  XID             *keymap;
  int              host_width, min_keycode, max_keycode, width;
  int              i,j;

  XDisplayKeycodes (HostX.dpy, &min_keycode, &max_keycode);

  EPHYR_DBG ("min: %d, max: %d", min_keycode, max_keycode);

  keymap = XGetKeyboardMapping (HostX.dpy,
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

static struct EphyrHostScreen *
host_screen_from_window (Window w)
{
  int index;

  for (index = 0 ; index < HostX.n_screens ; index++)
    {
      if (HostX.screens[index].win == w)
        {
          return &HostX.screens[index];
        }
    }
  return NULL;
}

int
hostx_get_event(EphyrHostXEvent *ev)
{
  XEvent      xev;
  static int  grabbed_screen = -1;

  if (XPending(HostX.dpy))
    {
      XNextEvent(HostX.dpy, &xev);

      switch (xev.type) 
	{
	case Expose:
	  /* Not so great event compression, but works ok */
	  while (XCheckTypedWindowEvent(HostX.dpy, xev.xexpose.window,
					Expose, &xev));
	  {
	    struct EphyrHostScreen *host_screen =
                host_screen_from_window (xev.xexpose.window);
	    hostx_paint_rect (host_screen->info, 0, 0, 0, 0,
                              host_screen->win_width,
                              host_screen->win_height);
	  }
	  return 0;

	case MotionNotify:
	  {
	    struct EphyrHostScreen *host_screen =
                host_screen_from_window (xev.xmotion.window);

	    ev->type = EPHYR_EV_MOUSE_MOTION;
	    ev->data.mouse_motion.x = xev.xmotion.x; 
	    ev->data.mouse_motion.y = xev.xmotion.y;
	    ev->data.mouse_motion.screen = (host_screen ? host_screen->mynum : -1);
	  }
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
	      struct EphyrHostScreen *host_screen =
                  host_screen_from_window (xev.xexpose.window);

	      if (grabbed_screen != -1)
		{
		  XUngrabKeyboard (HostX.dpy, CurrentTime);
		  XUngrabPointer (HostX.dpy, CurrentTime);
		  grabbed_screen = -1;
		  hostx_set_win_title (host_screen->info,
                                       "(ctrl+shift grabs mouse and keyboard)");
		}
	      else
		{
		  /* Attempt grab */
		  if (XGrabKeyboard (HostX.dpy, host_screen->win, True, 
				     GrabModeAsync, 
				     GrabModeAsync, 
				     CurrentTime) == 0)
		    {
		      if (XGrabPointer (HostX.dpy, host_screen->win, True, 
					NoEventMask, 
					GrabModeAsync, 
					GrabModeAsync, 
					host_screen->win, None, CurrentTime) == 0)
			{
			  grabbed_screen = host_screen->mynum;
			  hostx_set_win_title
                                  (host_screen->info,
                                   "(ctrl+shift releases mouse and keyboard)");
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

void*
hostx_get_display(void)
{
    return HostX.dpy ;
}

int
hostx_get_window(void)
{
    return HostX.win ;
}

int
hostx_get_extension_info (const char *a_ext_name,
                          int *a_major_opcode,
                          int *a_first_event,
                          int *a_first_error)
{
    if (!a_ext_name || !a_major_opcode || !a_first_event || !a_first_error)
      return 0 ;
   if (!XQueryExtension (HostX.dpy,
                         a_ext_name,
                         a_major_opcode,
                         a_first_event,
                         a_first_error))
     {
       return 0 ;
     }
   return 1 ;
}

