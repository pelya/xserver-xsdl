/*
 * Copyright 2005 by Kean Johnston <jkj@sco.com>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name Kean Johnston not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Kean Johnston makes no
 * representations about the suitability of this software for any purpose.
 * It is provided "as is" without express or implied warranty.
 *
 * KEAN JOHNSTON DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEAN JOHNSTON BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
/* $XConsortium$ */

/*
 * Based on sco_io.c which is
 * (C) Copyright 2003 Kean Johnston <jkj@sco.com>
 *
 * Based on lnx_kbd.c which is 
 * Copyright (c) 2002 by The XFree86 Project, Inc.
 *
 * Based on the code from lnx_io.c which is
 * Copyright 1992 by Orest Zborowski <obz@Kodak.com>
 * Copyright 1993 by David Dawes <dawes@xfree86.org>
 */

#define NEED_EVENTS
#include "X.h"

#include "compiler.h"

#define _NEED_SYSI86
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86OSpriv.h"
#include "xf86_OSlib.h"

#include "xf86Xinput.h"
#include "xf86OSKbd.h"
#include "atKeynames.h"
#include "usl_kbd.h"
#include "usl_xqueue.h"

#include <sys/param.h>

static KbdProtocolRec protocols[] = {
  { "standard", PROT_STD },
  { "Xqueue", PROT_XQUEUE },
  { NULL, PROT_UNKNOWN_KBD }
};

extern Bool VTSwitchEnabled;
#ifdef USE_VT_SYSREQ
extern Bool VTSysreqToggle;
#endif

static void
SoundBell(InputInfoPtr pInfo, int loudness, int pitch, int duration)
{
  if (loudness && pitch) {
    ioctl(pInfo->fd, KIOCSOUND, 1193180 / pitch);
    usleep(duration * loudness * 20);
    ioctl(pInfo->fd, KIOCSOUND, 0);
  }
}

static void
SetKbdLeds(InputInfoPtr pInfo, int leds)
{
  int real_leds = 0;

  if (leds & XLED1)
    real_leds |= LED_CAP;
  if (leds & XLED2)
    real_leds |= LED_NUM;
  if (leds & XLED3)
    real_leds |= LED_SCR;
  ioctl(pInfo->fd, KDSETLED, real_leds);
}

static int
GetKbdLeds(InputInfoPtr pInfo)
{
    int real_leds, leds = 0;

    ioctl(pInfo->fd, KDGETLED, &real_leds);

    if (real_leds & LED_CAP) leds |= XLED1;
    if (real_leds & LED_NUM) leds |= XLED2;
    if (real_leds & LED_SCR) leds |= XLED3;

    return(leds);
}

static void
SetKbdRepeat(InputInfoPtr pInfo, char rad)
{
  KbdDevPtr pKbd = (KbdDevPtr) pInfo->private;
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

  if (pKbd->rate >= 0) 
    rate = pKbd->rate * 10;
  if (pKbd->delay >= 0)
    delay = pKbd->delay;

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

  ioctl (pInfo->fd, KDSETTYPEMATICS, value);
}

static int
KbdInit(InputInfoPtr pInfo, int what)
{
  KbdDevPtr pKbd = (KbdDevPtr) pInfo->private;
  USLKbdPrivPtr priv = (USLKbdPrivPtr) pKbd->private;

  if (pKbd->isConsole) {
    if (ioctl (pInfo->fd, KDGKBMODE, &priv->orig_kbm) < 0) {
      xf86Msg (X_ERROR, "KbdInit: Could not determine keyboard mode\n");
      return !Success;
    }

    /*
     * We need to get the original keyboard map and NUL out the lock
     * modifiers. This prevents the kernel from messing with
     * the keyboard LED's. We restore the original map when we exit.
     * Note that we also have to eliminate screen switch sequences
     * else the VT manager will switch for us, which we don't want.
     * For example, lets say you had changed the VT manager to switch
     * on Alt-Fx instead of Ctrl-Alt-Fx. This means that while inside
     * X, you cant use, for example, Alt-F4, which is a pain in the
     * fundamental when you're using CDE-like thingies.
     */
    if (ioctl (pInfo->fd, GIO_KEYMAP, &priv->keymap) < 0) {
      xf86Msg (X_ERROR, "KbdInit: Failed to get keyboard map (%s)\n",
        strerror(errno));
      return !Success;
    }
    if (ioctl (pInfo->fd, GIO_KEYMAP, &priv->noledmap) < 0) {
      xf86Msg (X_ERROR, "KbdInit: Failed to get keyboard map (%s)\n",
        strerror(errno));
      return !Success;
    } else {
      int i, j;

      for (i = 0; i < priv->noledmap.n_keys; i++) {
        for (j = 0; j < NUM_STATES; j++) {
          if (IS_SPECKEY(&priv->noledmap, i, j) &&
            ((priv->noledmap.key[i].map[j] == K_CLK) ||
             (priv->noledmap.key[i].map[j] == K_NLK) ||
             (priv->noledmap.key[i].map[j] == K_SLK) ||
             (priv->noledmap.key[i].map[j] == K_FRCNEXT) ||
             (priv->noledmap.key[i].map[j] == K_FRCPREV) ||
             ((priv->noledmap.key[i].map[j] >=  K_VTF) &&
	      (priv->noledmap.key[i].map[j] <= K_VTL)) )) {
            priv->noledmap.key[i].map[j] = K_NOP;
          }
        }
      }
    }

    if (ioctl (pInfo->fd, TCGETA, &priv->kbdtty) < 0) {
      xf86Msg (X_ERROR, "KbdInit: Failed to get terminal modes (%s)\n",
        strerror(errno));
      return !Success;
    }
  } /* End of if we are on a console */

  return Success;
}

static int
KbdOn(InputInfoPtr pInfo, int what)
{
  KbdDevPtr pKbd = (KbdDevPtr) pInfo->private;
  USLKbdPrivPtr priv = (USLKbdPrivPtr) pKbd->private;
  struct termio newtio;

  if (pKbd->isConsole) {
    /*
     * Use the calculated keyboard map that does not have active
     * LED lock handling (we track LEDs ourselves).
     */
    ioctl (pInfo->fd, PIO_KEYMAP, &priv->noledmap);

#ifdef NOTYET
    newtio = priv->kbdtty;     /* structure copy */
    newtio.c_iflag = (IGNPAR | IGNBRK) & (~PARMRK) & (~ISTRIP);
    newtio.c_oflag = 0;
    newtio.c_cflag = CREAD | CS8 | B9600;
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME]=0;
    newtio.c_cc[VMIN]=1;
    ioctl(pInfo->fd, TCSETA, &newtio);

    if (priv->xq == 0)
      ioctl (pInfo->fd, KDSKBMODE, K_RAW);
    else
#endif
      XqKbdOnOff (pInfo, 1);
  }

  return Success;
}

static int
KbdOff(InputInfoPtr pInfo, int what)
{
  KbdDevPtr pKbd = (KbdDevPtr) pInfo->private;
  USLKbdPrivPtr priv = (USLKbdPrivPtr) pKbd->private;

  if (pKbd->isConsole) {
    /* Revert back to original translate scancode mode */
#ifdef NOTYET
    if (priv->xq == 0)
      ioctl (pInfo->fd, KDSKBMODE, priv->orig_kbm);
    else
#endif
      XqKbdOnOff (pInfo, 0);

    ioctl (pInfo->fd, PIO_KEYMAP, &priv->keymap);
    ioctl(pInfo->fd, TCSETA, &priv->kbdtty);
  }

  return Success;
}

#define ModifierSet(k) ((modifiers & (k)) == (k))

static Bool
SpecialKey(InputInfoPtr pInfo, int key, Bool down, int modifiers)
{
  KbdDevPtr pKbd = (KbdDevPtr) pInfo->private;

  if(!pKbd->vtSwitchSupported)
    return FALSE;

  if ((!ModifierSet(ShiftMask)) && ((ModifierSet(ControlMask | AltMask)) ||
      (ModifierSet(ControlMask | AltLangMask)))) {
    if (VTSwitchEnabled && !xf86Info.vtSysreq) {
      switch (key) {
        case KEY_F1:
        case KEY_F2:
        case KEY_F3:
        case KEY_F4:
        case KEY_F5:
        case KEY_F6:
        case KEY_F7:
        case KEY_F8:
        case KEY_F9:
        case KEY_F10:
          if (down) {
	    int sts = key - KEY_F1;
	    if (sts != xf86Info.vtno) {
	      ioctl(pInfo->fd, VT_SWITCH, sts);
	    }
            return TRUE;
          }
        case KEY_F11:
        case KEY_F12:
          if (down) {
	    int sts = key - KEY_F11 + 10;
	    if (sts != xf86Info.vtno) {
	      ioctl(pInfo->fd, VT_SWITCH, sts);
	    }
            return TRUE;
          }
      }
    }
  }
#ifdef USE_VT_SYSREQ
  if (VTSwitchEnabled && xf86Info.vtSysreq) {
    switch (key) {
      case KEY_F1:
      case KEY_F2:
      case KEY_F3:
      case KEY_F4:
      case KEY_F5:
      case KEY_F6:
      case KEY_F7:
      case KEY_F8:
      case KEY_F9:
      case KEY_F10:
        if (VTSysreqToggle && down) {
          ioctl(pInfo->fd, VT_ACTIVATE, key - KEY_F1);
          VTSysreqToggle = FALSE;
          return TRUE;
        }
        break;
      case KEY_F11:
      case KEY_F12:
        if (VTSysreqToggle && down) {
          ioctl(pInfo->fd, VT_ACTIVATE, key - KEY_F11 + 10);
          VTSysreqToggle = FALSE;
          return TRUE;
        }
        break;
        /* Ignore these keys -- ie don't let them cancel an alt-sysreq */
      case KEY_Alt:
      case KEY_AltLang:
        break;
      case KEY_SysReqest:
        if (!(ModifierSet(ShiftMask) || ModifierSet(ControlMask))) {
          if ((ModifierSet(AltMask) || ModifierSet(AltLangMask)) && down)
            VTSysreqToggle = TRUE;
        }
        break;
      default:
        /*
         * We only land here when Alt-SysReq is followed by a
         * non-switching key.
         */
        if (VTSysreqToggle)
          VTSysreqToggle = FALSE;
    }
  }
#endif /* USE_VT_SYSREQ */
  return FALSE;
} 

#ifdef NOTYET
static void
stdReadInput(InputInfoPtr pInfo)
{
  KbdDevPtr pKbd = (KbdDevPtr) pInfo->private;
  unsigned char rBuf[64];
  int nBytes, i;

  if ((nBytes = read( pInfo->fd, (char *)rBuf, sizeof(rBuf))) > 0) {
    for (i = 0; i < nBytes; i++) {
      pKbd->PostEvent(pInfo, rBuf[i] & 0x7f, rBuf[i] & 0x80 ? FALSE : TRUE);
    }
  }
}
#endif

static Bool
OpenKeyboard(InputInfoPtr pInfo)
{
  KbdDevPtr pKbd = (KbdDevPtr) pInfo->private;
  USLKbdPrivPtr priv = (USLKbdPrivPtr) pKbd->private;
  int i;
  KbdProtocolId prot = PROT_UNKNOWN_KBD;
  char *s;

  s = xf86SetStrOption(pInfo->options, "Protocol", NULL);
  for (i = 0; protocols[i].name; i++) {
    if (xf86NameCmp(s, protocols[i].name) == 0) {
      prot = protocols[i].id;
      break;
    }
  }

  switch (prot) {
    case PROT_STD:
#ifdef NOTYET
      pInfo->read_input = stdReadInput;
      priv->xq = 0;
      break;
#endif
    case PROT_XQUEUE:
      pInfo->read_input = NULL;	/* Handled by the XQUEUE signal handler */
      priv->xq = 1;
      break;
    default:
      xf86Msg(X_ERROR,"\"%s\" is not a valid keyboard protocol name\n", s);
      xfree(s);
      return FALSE;
  }

  xf86Msg(X_CONFIG, "%s: Protocol: %s\n", pInfo->name, s);
  xfree(s);

  s = xf86SetStrOption(pInfo->options, "Device", NULL);
  if (s == NULL) {
    pInfo->fd = xf86Info.consoleFd;
    pKbd->isConsole = TRUE;
  } else {
    pInfo->fd = open(s, O_RDONLY | O_NONBLOCK | O_EXCL);
    if (pInfo->fd == -1) {
      xf86Msg(X_ERROR, "%s: cannot open \"%s\"\n", pInfo->name, s);
      xfree(s);
      return FALSE;
    }
    pKbd->isConsole = FALSE;
    xfree(s);
  }

  if (pKbd->isConsole)
    pKbd->vtSwitchSupported = TRUE;

  return TRUE;
}

_X_EXPORT Bool
xf86OSKbdPreInit(InputInfoPtr pInfo)
{
  KbdDevPtr pKbd = pInfo->private;

  pKbd->KbdInit       = KbdInit;
  pKbd->KbdOn         = KbdOn;
  pKbd->KbdOff        = KbdOff;
  pKbd->Bell          = SoundBell;
  pKbd->SetLeds       = SetKbdLeds;
  pKbd->GetLeds       = GetKbdLeds;
  pKbd->SetKbdRepeat  = SetKbdRepeat;
  pKbd->KbdGetMapping = KbdGetMapping;
  pKbd->SpecialKey    = SpecialKey;
  pKbd->OpenKeyboard  = OpenKeyboard;

  pKbd->GetSpecialKey = NULL;
  pKbd->RemapScanCode = ATScancode;
  pKbd->vtSwitchSupported = FALSE;

  pKbd->private = xcalloc(sizeof(USLKbdPrivRec), 1);
  if (pKbd->private == NULL) {
    xf86Msg(X_ERROR,"can't allocate keyboard OS private data\n");
    return FALSE;
  }

  return TRUE;
}
