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

#ifndef _XLIBS_STUFF_H_
#define _XLIBS_STUFF_H_

#include <X11/X.h>
#include <X11/Xmd.h>

#define EPHYR_WANT_DEBUG 0

#if (EPHYR_WANT_DEBUG)
#define EPHYR_DBG(x, a...) \
 fprintf(stderr, __FILE__ ":%d,%s() " x "\n", __LINE__, __func__, ##a)
#else
#define EPHYR_DBG(x, a...) do {} while (0) 
#endif

typedef struct EphyrHostXVars  EphyrHostXVars;
typedef struct EphyrHostXEvent EphyrHostXEvent;

typedef enum EphyrHostXEventType 
{
  EPHYR_EV_MOUSE_MOTION,
  EPHYR_EV_MOUSE_PRESS,
  EPHYR_EV_MOUSE_RELEASE,
  EPHYR_EV_KEY_PRESS,
  EPHYR_EV_KEY_RELEASE
} 
EphyrHostXEventType;

/* I can't believe it's not a KeySymsRec. */
typedef struct {
  int             minKeyCode;
  int             maxKeyCode;
  int             mapWidth;
  CARD32         *map;
} EphyrKeySyms;

struct EphyrHostXEvent
{
  EphyrHostXEventType type;

  union 
  {
    struct mouse_motion { 	
      int x;
      int y;
    } mouse_motion;

    struct mouse_down { 	
      int button_num;
    } mouse_down;

    struct mouse_up {
      int button_num;
    } mouse_up;

    struct key_up {
      int scancode;
    } key_up;

    struct key_down {
      int scancode;
    } key_down;

  } data;

  int key_state;
};

int
hostx_want_screen_size(int *width, int *height);

int
hostx_want_host_cursor(void);

void
hostx_use_host_cursor(void);

void
hostx_use_fullscreen(void);

int
hostx_want_fullscreen(void);

int
hostx_want_preexisting_window(void);

void
hostx_use_preexisting_window(unsigned long win_id);

void 
hostx_handle_signal(int signum);

int
hostx_init(void);

void
hostx_set_display_name(char *name);

void
hostx_set_win_title(char *extra_text);

int
hostx_get_depth (void);

int
hostx_get_server_depth (void);

void
hostx_set_server_depth(int depth);

int
hostx_get_bpp(void);

void
hostx_get_visual_masks (CARD32 *rmsk, 
			CARD32 *gmsk, 
			CARD32 *bmsk);
void
hostx_set_cmap_entry(unsigned char idx, 
		     unsigned char r, 
		     unsigned char g, 
		     unsigned char b);

void*
hostx_screen_init (int width, int height, int buffer_height);

void
hostx_paint_rect(int sx,    int sy,
		 int dx,    int dy, 
		 int width, int height);
void
hostx_paint_debug_rect(int x,     int y, 
		       int width, int height);

void
hostx_load_keymap(void);

int
hostx_get_event(EphyrHostXEvent *ev);

#endif
