/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Io.c,v 3.56 2003/11/03 05:11:02 tsi Exp $ */
/*
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Thomas Roell not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Thomas Roell makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THOMAS ROELL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THOMAS ROELL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */
/*
 * Copyright (c) 1992-2003 by The XFree86 Project, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

/* $XConsortium: xf86Io.c /main/27 1996/10/19 17:58:55 kaleb $ */
/* $XdotOrg: xserver/xorg/hw/xfree86/common/xf86Io.c,v 1.6 2006/03/25 19:52:03 ajax Exp $ */

#define NEED_EVENTS
#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <X11/X.h>
#include <X11/Xproto.h>
#include "inputstr.h"
#include "scrnintstr.h"

#include "compiler.h"

#include "xf86.h"
#include "xf86Priv.h"
#define XF86_OS_PRIVS
#include "xf86_OSlib.h"
#include "mipointer.h"

#ifdef XINPUT
#include "xf86Xinput.h"
#include <X11/extensions/XIproto.h>
#include "exevents.h"
#endif

#ifdef XKB
#include <X11/extensions/XKB.h>
#include <X11/extensions/XKBstr.h>
#include <X11/extensions/XKBsrv.h>
#endif

unsigned int xf86InitialCaps = 0;
unsigned int xf86InitialNum = 0;
unsigned int xf86InitialScroll = 0;

#include "atKeynames.h"

/*
 * xf86KbdBell --
 *	Ring the terminal/keyboard bell for an amount of time proportional to
 *      "loudness".
 */

void
xf86KbdBell(percent, pKeyboard, ctrl, unused)
     int           percent;          /* Percentage of full volume */
     DeviceIntPtr  pKeyboard;        /* Keyboard to ring */
     pointer	   ctrl;	
     int	   unused;	
{
  xf86SoundKbdBell(percent, xf86Info.bell_pitch, xf86Info.bell_duration);
}

void
xf86UpdateKbdLeds()
{
  int leds = 0;
  if (xf86Info.capsLock) leds |= XLED1;
  if (xf86Info.numLock)  leds |= XLED2;
  if (xf86Info.scrollLock || xf86Info.modeSwitchLock) leds |= XLED3;
  if (xf86Info.composeLock) leds |= XLED4;
  xf86Info.leds = (xf86Info.leds & xf86Info.xleds) | (leds & ~xf86Info.xleds);
  xf86KbdLeds();
}

void
xf86KbdLeds ()
{
  int leds, real_leds = 0;

#if defined (__sparc__) && defined(__linux__)
  static int kbdSun = -1;
  if (kbdSun == -1) {
  if ((xf86Info.xkbmodel && !strcmp(xf86Info.xkbmodel, "sun")) ||
      (xf86Info.xkbrules && !strcmp(xf86Info.xkbrules, "sun")))
      kbdSun = 1;
  else
      kbdSun = 0;
  }
  if (kbdSun) {
     if (xf86Info.leds & 0x08) real_leds |= XLED1;
     if (xf86Info.leds & 0x04) real_leds |= XLED3;
     if (xf86Info.leds & 0x02) real_leds |= XLED4;
     if (xf86Info.leds & 0x01) real_leds |= XLED2;
     leds = real_leds;
     real_leds = 0;
  } else {
     leds = xf86Info.leds;
  }
#else
  leds = xf86Info.leds;
#endif /* defined (__sparc__) */

#ifdef LED_CAP
  if (leds & XLED1)  real_leds |= LED_CAP;
  if (leds & XLED2)  real_leds |= LED_NUM;
  if (leds & XLED3)  real_leds |= LED_SCR;
#ifdef LED_COMP
  if (leds & XLED4)  real_leds |= LED_COMP;
#else
  if (leds & XLED4)  real_leds |= LED_SCR;
#endif
#endif
#ifdef sun
  /* Pass through any additional LEDs, such as Kana LED on Sun Japanese kbd */
  real_leds |= (leds & 0xFFFFFFF0);
#endif
  xf86SetKbdLeds(real_leds);
  (void)leds;
}
