/* $XFree86$ */
/*
 * Copyright 2005 by J. Kean Johnston <jkj@sco.com>
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

/*
 * Based on sco_io.c which is
 * (C) Copyright 2003 J. Kean Johnston <jkj@sco.com>
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
#include "sco_kbd.h"

#include <sys/param.h>
#include <sys/emap.h>
#include <sys/nmap.h>

static KbdProtocolRec protocols[] = {
  { "standard", PROT_STD },
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
  static int once = 1;

  /*
   * sleep the first time through under SCO.  There appears to be a
   * timing problem in the driver which causes the keyboard to be lost.
   * This usleep stops it from occurring. NOTE: this was in the old code.
   * I am not convinced it is true any longer, but it doesn't hurt to
   * leave this in here.
   */
  if (once) {
    usleep(100);
    once = 0;
  }

#ifdef LED_CAP
  if (leds & XLED1)
    real_leds |= LED_CAP;
  if (leds & XLED2)
    real_leds |= LED_NUM;
  if (leds & XLED3)
    real_leds |= LED_SCR;
#ifdef LED_COMP
  if (leds & XLED4)
    real_leds |= LED_COMP;
#else
  if (leds & XLED4)
    real_leds |= LED_SCR;
#endif
#endif
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

/*
 * NOTE: Only OpenServer Release 5.0.6 with Release Supplement 5.0.6A
 * and later have the required ioctl. 5.0.6A or higher is HIGHLY
 * recommended. The console driver is quite a different beast on that OS.
 */
#undef rate

static void
SetKbdRepeat(InputInfoPtr pInfo, char rad)
{
#if defined(KBIO_SETRATE)
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

  ioctl (pInfo->fd, KBIO_SETRATE, value);
#endif /* defined(KBIO_SETRATE) */
}

static int
KbdInit(InputInfoPtr pInfo, int what)
{
  KbdDevPtr pKbd = (KbdDevPtr) pInfo->private;
  ScoKbdPrivPtr priv = (ScoKbdPrivPtr) pKbd->private;

  if (pKbd->isConsole) {
    priv->use_tcs = 1;
    priv->use_kd = 1;
    priv->no_nmap = 1;
    priv->no_emap = 1;
    priv->orig_getsc = 0;

    if (ioctl (pInfo->fd, TCGETSC, &priv->orig_getsc) < 0)
      priv->use_tcs = 0;
    if (ioctl (pInfo->fd, KDGKBMODE, &priv->orig_kbm) < 0)
      priv->use_kd = 0;

    if (!priv->use_tcs && !priv->use_kd) {
      xf86Msg (X_ERROR, "KbdInit: Could not determine keyboard mode\n");
      return !Success;
    }

    /*
     * One day this should be fixed to translate normal ASCII characters
     * back into scancodes or into events that XFree86 wants, but not
     * now. For the time being, we only support scancode mode screens.
     */
    if (priv->use_tcs && !(priv->orig_getsc & KB_ISSCANCODE)) {
      xf86Msg (X_ERROR, "KbdInit: Keyboard can not send scancodes\n");
      return !Success;
    }

    /*
     * We need to get the original keyboard map and NUL out the lock
     * modifiers. This prevents the scancode API from messing with
     * the keyboard LED's. We restore the original map when we exit.
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
          if (IS_SPECIAL(priv->noledmap, i, j) &&
            ((priv->noledmap.key[i].map[j] == K_CLK) ||
             (priv->noledmap.key[i].map[j] == K_NLK) ||
             (priv->noledmap.key[i].map[j] == K_SLK))) {
            priv->noledmap.key[i].map[j] = K_NOP;
          }
        }
      }
    }

    if (ioctl (pInfo->fd, XCGETA, &priv->kbdtty) < 0) {
      xf86Msg (X_ERROR, "KbdInit: Failed to get terminal modes (%s)\n",
        strerror(errno));
      return !Success;
    }

    priv->sc_mapbuf = xalloc (10*BSIZE);
    priv->sc_mapbuf2 = xalloc(10*BSIZE);

    /* Get the emap */
    if (ioctl (pInfo->fd, LDGMAP, priv->sc_mapbuf) < 0) {
      if (errno != ENAVAIL) {
        xf86Msg (X_ERROR, "KbdInit: Failed to retrieve e-map (%s)\n",
          strerror (errno));
        return !Success;
      }
      priv->no_emap = 0;
    }

    /* Get the nmap */
    if (ioctl (pInfo->fd, NMGMAP, priv->sc_mapbuf2) < 0) {
      if (errno != ENAVAIL) {
        xf86Msg (X_ERROR, "KbdInit: Failed to retrieve n-map (%s)\n",
          strerror (errno));
        return !Success;
      }
      priv->no_nmap = 0;
    }
  } /* End of if we are on a console */

  return Success;
}

static int
KbdOn(InputInfoPtr pInfo, int what)
{
  KbdDevPtr pKbd = (KbdDevPtr) pInfo->private;
  ScoKbdPrivPtr priv = (ScoKbdPrivPtr) pKbd->private;
  struct termios newtio;

  if (pKbd->isConsole) {
    ioctl (pInfo->fd, LDNMAP); /* Turn e-mapping off */
    ioctl (pInfo->fd, NMNMAP); /* Turn n-mapping off */

    newtio = priv->kbdtty;     /* structure copy */
    newtio.c_iflag = (IGNPAR | IGNBRK) & (~PARMRK) & (~ISTRIP);
    newtio.c_oflag = 0;
    newtio.c_cflag = CREAD | CS8 | B9600;
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME]=0;
    newtio.c_cc[VMIN]=1;
    cfsetispeed(&newtio, 9600);
    cfsetospeed(&newtio, 9600);
    ioctl(pInfo->fd, XCSETA, &newtio);

    /* Now tell the keyboard driver to send us raw scancodes */
    if (priv->use_tcs) {
      int nm = priv->orig_getsc;
      nm &= ~KB_XSCANCODE;
      ioctl (pInfo->fd, TCSETSC, &nm);
    }

    if (priv->use_kd)
      ioctl (pInfo->fd, KDSKBMODE, K_RAW);

    ioctl (pInfo->fd, PIO_KEYMAP, &priv->noledmap);
  }

  return Success;
}

static int
KbdOff(InputInfoPtr pInfo, int what)
{
  KbdDevPtr pKbd = (KbdDevPtr) pInfo->private;
  ScoKbdPrivPtr priv = (ScoKbdPrivPtr) pKbd->private;

  if (pKbd->isConsole) {
    /* Revert back to original translate scancode mode */
    if (priv->use_tcs)
      ioctl (pInfo->fd, TCSETSC, &priv->orig_getsc);
    if (priv->use_kd)
      ioctl (pInfo->fd, KDSKBMODE, priv->orig_kbm);

    ioctl (pInfo->fd, PIO_KEYMAP, &priv->keymap);

    if (priv->no_emap)
      ioctl (pInfo->fd, LDSMAP, priv->sc_mapbuf);
    if (priv->no_nmap)
      ioctl (pInfo->fd, NMSMAP, priv->sc_mapbuf2);

    ioctl(pInfo->fd, XCSETA, &priv->kbdtty);
  }

  return Success;
}

static int
GetSpecialKey(InputInfoPtr pInfo, int scanCode)
{
  KbdDevPtr pKbd = (KbdDevPtr) pInfo->private;
  int specialkey = scanCode;

  if (pKbd->CustomKeycodes) {
      specialkey = pKbd->specialMap->map[scanCode];
  }
  return specialkey;
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
	      ioctl(pInfo->fd, VT_ACTIVATE, sts);
	    }
            return TRUE;
          }
        case KEY_F11:
        case KEY_F12:
          if (down) {
	    int sts = key - KEY_F11 + 10;
	    if (sts != xf86Info.vtno) {
	      ioctl(pInfo->fd, VT_ACTIVATE, sts);
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

static Bool
OpenKeyboard(InputInfoPtr pInfo)
{
  KbdDevPtr pKbd = (KbdDevPtr) pInfo->private;
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
      pInfo->read_input = stdReadInput;
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

  pKbd->KbdInit           = KbdInit;
  pKbd->KbdOn             = KbdOn;
  pKbd->KbdOff            = KbdOff;
  pKbd->Bell              = SoundBell;
  pKbd->SetLeds           = SetKbdLeds;
  pKbd->GetLeds           = GetKbdLeds;
  pKbd->SetKbdRepeat      = SetKbdRepeat;
  pKbd->KbdGetMapping     = KbdGetMapping;
  pKbd->SpecialKey        = SpecialKey;
  pKbd->GetSpecialKey     = GetSpecialKey;
  pKbd->OpenKeyboard      = OpenKeyboard;
  pKbd->RemapScanCode     = ATScancode;
  pKbd->vtSwitchSupported = FALSE;

  pKbd->private = xcalloc(sizeof(ScoKbdPrivRec), 1);
  if (pKbd->private == NULL) {
    xf86Msg(X_ERROR,"can't allocate keyboard OS private data\n");
    return FALSE;
  }

  return TRUE;
}
