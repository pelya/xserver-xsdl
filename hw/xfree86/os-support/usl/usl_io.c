/*
 * Copyright 2001-2005 by Kean Johnston <jkj@sco.com>
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany
 * Copyright 1993 by David Dawes <dawes@xfree86.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of Thomas Roell, David Dawes 
 * and Kean Johnston not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Thomas Roell, David Dawes and Kean Johnston make no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THOMAS ROELL, DAVID DAWES AND KEAN JOHNSTON DISCLAIM ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL THOMAS ROELLm DAVID WEXELBLAT
 * OR KEAN JOHNSTON BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
 * THIS SOFTWARE.
 *
 */
/* $XConsortium$ */

#include "X.h"

#include "compiler.h"

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"

_X_EXPORT void
xf86SoundKbdBell(int loudness, int pitch, int duration)
{
  if (loudness && pitch) {
    ioctl(xf86Info.consoleFd, KIOCSOUND, 1193180 / pitch);
    usleep(xf86Info.bell_duration * loudness * 20);
    ioctl(xf86Info.consoleFd, KIOCSOUND, 0);
  }
}

void
xf86SetKbdLeds(int leds)
{
  ioctl(xf86Info.consoleFd, KDSETLED, leds);
}

int
xf86GetKbdLeds(void)
{
  int leds;

  ioctl(xf86Info.consoleFd, KDGETLED, &leds);
  return(leds);
}

/*
 * Much of the code in this function is duplicated from the Linux code
 * by Orest Zborowski <obz@Kodak.com> and David Dawes <dawes@xfree86.org>.
 * Please see the file ../linux/lnx_io.c for full copyright information.
 */
void
xf86SetKbdRepeat(char rad)
{
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

  ioctl (xf86Info.consoleFd, KDSETTYPEMATICS, value);
}

static int orig_kbm;
static struct termio orig_termio;
static keymap_t keymap, noledmap;

void
xf86KbdInit(void)
{
  ioctl (xf86Info.consoleFd, KDGKBMODE, &orig_kbm);
  ioctl (xf86Info.consoleFd, TCGETA, &orig_termio);
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
        if (IS_SPECKEY(&noledmap, i, j) &&
            ((noledmap.key[i].map[j] == K_CLK) ||
             (noledmap.key[i].map[j] == K_NLK) ||
             (noledmap.key[i].map[j] == K_SLK))) {
          noledmap.key[i].map[j] = K_NOP;
        }
      }
    }
  }
}

int
xf86KbdOn(void)
{
  struct termio newtio;

  newtio = orig_termio;        /* structure copy */
  newtio.c_iflag = (IGNPAR | IGNBRK) & (~PARMRK) & (~ISTRIP);
  newtio.c_oflag = 0;
  newtio.c_cflag = CREAD | CS8 | B9600;
  newtio.c_lflag = 0;
  newtio.c_cc[VTIME]=0;
  newtio.c_cc[VMIN]=1;
  ioctl(xf86Info.consoleFd, TCSETA, &newtio);

  ioctl (xf86Info.consoleFd, KDSKBMODE, K_RAW);
  ioctl (xf86Info.consoleFd, PIO_KEYMAP, &noledmap);

  return(xf86Info.consoleFd);
}

int
xf86KbdOff(void)
{
  ioctl (xf86Info.consoleFd, KDSKBMODE, orig_kbm);
  ioctl (xf86Info.consoleFd, PIO_KEYMAP, &keymap);
  ioctl(xf86Info.consoleFd, TCSETA, &orig_termio);

  return(xf86Info.consoleFd);
}
