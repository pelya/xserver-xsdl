/*
 * Copyright 2001 by J. Kean Johnston <jkj@sco.com>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name J. Kean Johnston not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  J. Kean Johnston makes no
 * representations about the suitability of this software for any purpose.
 * It is provided "as is" without express or implied warranty.
 *
 * J. KEAN JOHNSTON DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL J. KEAN JOHNSTON BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
/* $XConsortium$ */

/* Re-written May 2001 to represent the current state of reality */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <X11/X.h>

#include "compiler.h"

#define _NEED_SYSI86
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86OSpriv.h"
#include "xf86_OSlib.h"

#include <sys/param.h>
#include <sys/emap.h>
#include <sys/nmap.h>

_X_EXPORT void
xf86SoundKbdBell(int loudness, int pitch, int duration)
{
  if (loudness && pitch) {
    ioctl(xf86Info.consoleFd, KIOCSOUND, 1193180 / pitch);
    usleep(duration * loudness * 20);
    ioctl(xf86Info.consoleFd, KIOCSOUND, 0);
  }
}

void
xf86SetKbdLeds(int leds)
{
  /*
   * sleep the first time through under SCO.  There appears to be a
   * timing problem in the driver which causes the keyboard to be lost.
   * This usleep stops it from occurring. NOTE: this was in the old code.
   * I am not convinced it is true any longer, but it doesn't hurt to
   * leave this in here.
   */
  static int once = 1;

  if (once) {
    usleep(100);
    once = 0;
  }

  ioctl(xf86Info.consoleFd, KDSETLED, leds );
}

int
xf86GetKbdLeds(void)
{
  int leds;

  ioctl (xf86Info.consoleFd, KDGETLED, &leds);
  return leds;
}

/*
 * Much of the code in this function is duplicated from the Linux code
 * by Orest Zborowski <obz@Kodak.com> and David Dawes <dawes@xfree86.org>.
 * Please see the file ../linux/lnx_io.c for full copyright information.
 *
 * NOTE: Only OpenServer Release 5.0.6 with Release Supplement 5.0.6A
 * and later have the required ioctl. 5.0.6A or higher is HIGHLY
 * recommended. The console driver is quite a different beast on that OS.
 */
void
xf86SetKbdRepeat(char rad)
{
#if defined(KBIO_SETRATE)
  int i;
  int value = 0x7f;     /* Maximum delay with slowest rate */
  int delay = 250;      /* Default delay */
  int rate = 300;       /* Default repeat rate */

  static int valid_rates[] = { 300, 267, 240, 218, 200, 185, 171, 160, 150,
                               133, 120, 109, 100, 92, 86, 80, 75, 67,
                               60, 55, 50, 46, 43, 40, 37, 33, 30, 27,
                               25, 23, 21, 20 };
#define RATE_COUNT (sizeof( valid_rates ) / sizeof( int ))

  static int valid_delays[] = { 250, 500, 750, 1000 };
#define DELAY_COUNT (sizeof( valid_delays ) / sizeof( int ))

  if (xf86Info.kbdRate >= 0) 
    rate = xf86Info.kbdRate * 10;
  if (xf86Info.kbdDelay >= 0)
    delay = xf86Info.kbdDelay;

  for (i = 0; i < RATE_COUNT; i++)
    if (rate >= valid_rates[i]) {
      value &= 0x60;
      value |= i;
      break;
    }

  for (i = 0; i < DELAY_COUNT; i++)
    if (delay <= valid_delays[i]) {
      value &= 0x1f;
      value |= i << 5;
      break;
    }

  ioctl (xf86Info.consoleFd, KBIO_SETRATE, value);
#endif /* defined(KBIO_SETRATE) */
}

static Bool use_tcs = TRUE, use_kd = TRUE;
static Bool no_nmap = TRUE, no_emap = TRUE;
static int orig_getsc, orig_kbm;
static struct termios orig_termios;
static keymap_t keymap, noledmap;
static uchar_t *sc_mapbuf;
static uchar_t *sc_mapbuf2;

void
xf86KbdInit(void)
{
  orig_getsc = 0;
  if (ioctl (xf86Info.consoleFd, TCGETSC, &orig_getsc) < 0)
    use_tcs = FALSE;
  if (ioctl (xf86Info.consoleFd, KDGKBMODE, &orig_kbm) < 0)
    use_kd = FALSE;

  if (!use_tcs && !use_kd)
    FatalError ("xf86KbdInit: Could not determine keyboard mode\n");

  /*
   * One day this should be fixed to translate normal ASCII characters
   * back into scancodes or into events that XFree86 wants, but not
   * now. For the time being, we only support scancode mode screens.
  */
  if (use_tcs && !(orig_getsc & KB_ISSCANCODE))
    FatalError ("xf86KbdInit: Keyboard can not send scancodes\n");

  /*
   * We need to get the original keyboard map and NUL out the lock
   * modifiers. This prevents the scancode API from messing with
   * the keyboard LED's. We restore the original map when we exit.
   */
  if (ioctl (xf86Info.consoleFd, GIO_KEYMAP, &keymap) < 0) {
    FatalError ("xf86KbdInit: Failed to get keyboard map (%s)\n",
        strerror(errno));
  }
  if (ioctl (xf86Info.consoleFd, GIO_KEYMAP, &noledmap) < 0) {
    FatalError ("xf86KbdInit: Failed to get keyboard map (%s)\n",
        strerror(errno));
  } else {
    int i, j;

    for (i = 0; i < noledmap.n_keys; i++) {
      for (j = 0; j < NUM_STATES; j++) {
        if (IS_SPECIAL(noledmap, i, j) &&
            ((noledmap.key[i].map[j] == K_CLK) ||
             (noledmap.key[i].map[j] == K_NLK) ||
             (noledmap.key[i].map[j] == K_SLK))) {
          noledmap.key[i].map[j] = K_NOP;
        }
      }
    }
  }

  if (ioctl (xf86Info.consoleFd, XCGETA, &orig_termios) < 0) {
    FatalError ("xf86KbdInit: Failed to get terminal modes (%s)\n",
        strerror(errno));
  }

  sc_mapbuf = xalloc (10*BSIZE);
  sc_mapbuf2 = xalloc(10*BSIZE);

  /* Get the emap */
  if (ioctl (xf86Info.consoleFd, LDGMAP, sc_mapbuf) < 0) {
    if (errno != ENAVAIL) {
      FatalError ("xf86KbdInit: Failed to retrieve e-map (%s)\n",
          strerror (errno));
    }
    no_emap = FALSE;
  }

  /* Get the nmap */
  if (ioctl (xf86Info.consoleFd, NMGMAP, sc_mapbuf2) < 0) {
    if (errno != ENAVAIL) {
      FatalError ("xf86KbdInit: Failed to retrieve n-map (%s)\n",
          strerror (errno));
    }
    no_nmap = FALSE;
  }
}

int
xf86KbdOn(void)
{
  struct termios newtio;

  ioctl (xf86Info.consoleFd, LDNMAP); /* Turn e-mapping off */
  ioctl (xf86Info.consoleFd, NMNMAP); /* Turn n-mapping off */

  newtio = orig_termios;        /* structure copy */
  newtio.c_iflag = (IGNPAR | IGNBRK) & (~PARMRK) & (~ISTRIP);
  newtio.c_oflag = 0;
  newtio.c_cflag = CREAD | CS8 | B9600;
  newtio.c_lflag = 0;
  newtio.c_cc[VTIME]=0;
  newtio.c_cc[VMIN]=1;
  cfsetispeed(&newtio, 9600);
  cfsetospeed(&newtio, 9600);
  ioctl(xf86Info.consoleFd, XCSETA, &newtio);

  /* Now tell the keyboard driver to send us raw scancodes */
  if (use_tcs) {
    int nm = orig_getsc;
    nm &= ~KB_XSCANCODE;
    ioctl (xf86Info.consoleFd, TCSETSC, &nm);
  }

  if (use_kd)
    ioctl (xf86Info.consoleFd, KDSKBMODE, K_RAW);

  ioctl (xf86Info.consoleFd, PIO_KEYMAP, &noledmap);

  return(xf86Info.consoleFd);
}

int
xf86KbdOff(void)
{
  /* Revert back to original translate scancode mode */
  if (use_tcs)
    ioctl (xf86Info.consoleFd, TCSETSC, &orig_getsc);
  if (use_kd)
    ioctl (xf86Info.consoleFd, KDSKBMODE, orig_kbm);

  ioctl (xf86Info.consoleFd, PIO_KEYMAP, &keymap);

  if (no_emap)
    ioctl (xf86Info.consoleFd, LDSMAP, sc_mapbuf);
  if (no_nmap)
    ioctl (xf86Info.consoleFd, NMSMAP, sc_mapbuf2);

  ioctl(xf86Info.consoleFd, XCSETA, &orig_termios);

  return(xf86Info.consoleFd);
}
