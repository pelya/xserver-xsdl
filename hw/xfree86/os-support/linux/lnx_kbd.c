/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/linux/lnx_kbd.c,v 1.5 2003/11/04 03:14:39 tsi Exp $ */

/*
 * Copyright (c) 2002 by The XFree86 Project, Inc.
 * Author: Ivan Pascal.
 *
 * Based on the code from lnx_io.c which is
 * Copyright 1992 by Orest Zborowski <obz@Kodak.com>
 * Copyright 1993 by David Dawes <dawes@xfree86.org>
 */

#define NEED_EVENTS
#include "X.h"

#include "compiler.h"

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"

#include "xf86Xinput.h"
#include "xf86OSKbd.h"
#include "atKeynames.h"
#include "lnx_kbd.h"

#define KBC_TIMEOUT 250        /* Timeout in ms for sending to keyboard controller */

static KbdProtocolRec protocols[] = {
   {"standard", PROT_STD },
   { NULL, PROT_UNKNOWN_KBD }
};

extern Bool VTSwitchEnabled;
#ifdef USE_VT_SYSREQ
extern Bool VTSysreqToggle;
#endif

static void
SoundBell(InputInfoPtr pInfo, int loudness, int pitch, int duration)
{
	if (loudness && pitch)
	{
		ioctl(pInfo->fd, KDMKTONE,
		      ((1193190 / pitch) & 0xffff) |
		      (((unsigned long)duration *
			loudness / 50) << 16));
	}
}

static void
SetKbdLeds(InputInfoPtr pInfo, int leds)
{
    int real_leds = 0;

#if defined (__sparc__)
    KbdDevPtr pKbd = (KbdDevPtr) pInfo->private;
    if (pKbd->sunKbd) {
  	if (leds & 0x08) real_leds |= XLED1;
  	if (leds & 0x04) real_leds |= XLED3;
  	if (leds & 0x02) real_leds |= XLED4;
  	if (leds & 0x01) real_leds |= XLED2;
        leds = real_leds;
        real_leds = 0;
    }
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

/* kbd rate stuff based on kbdrate.c from Rik Faith <faith@cs.unc.edu> et.al.
 * from util-linux-2.9t package */

#include <linux/kd.h>
#include <linux/version.h>
#ifdef __sparc__
#include <asm/param.h>
#include <asm/kbio.h>
#endif

/* Deal with spurious kernel header change */
#if defined(LINUX_VERSION_CODE) && defined(KERNEL_VERSION)
# if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,42)
#  define rate period
# endif
#endif

static int
KDKBDREP_ioctl_ok(int rate, int delay) {
#if defined(KDKBDREP) && !defined(__sparc__)
     /* This ioctl is defined in <linux/kd.h> but is not
	implemented anywhere - must be in some m68k patches. */
   struct kbd_repeat kbdrep_s;

   /* don't change, just test */
   kbdrep_s.rate = -1;
   kbdrep_s.delay = -1;
   if (ioctl( 0, KDKBDREP, &kbdrep_s )) {
       return 0;
   }

   /* do the change */
   if (rate == 0)				/* switch repeat off */
     kbdrep_s.rate = 0;
   else
     kbdrep_s.rate  = 10000 / rate;		/* convert cps to msec */
   if (kbdrep_s.rate < 1)
     kbdrep_s.rate = 1;
   kbdrep_s.delay = delay;
   if (kbdrep_s.delay < 1)
     kbdrep_s.delay = 1;
   
   if (ioctl( 0, KDKBDREP, &kbdrep_s )) {
     return 0;
   }

   return 1;			/* success! */
#else /* no KDKBDREP */
   return 0;
#endif /* KDKBDREP */
}

static int
KIOCSRATE_ioctl_ok(int rate, int delay) {
#ifdef KIOCSRATE
   struct kbd_rate kbdrate_s;
   int fd;

   fd = open("/dev/kbd", O_RDONLY);
   if (fd == -1) 
     return 0;   

   kbdrate_s.rate = (rate + 5) / 10;  /* must be integer, so round up */
   kbdrate_s.delay = delay * HZ / 1000;  /* convert ms to Hz */
   if (kbdrate_s.rate > 50)
     kbdrate_s.rate = 50;

   if (ioctl( fd, KIOCSRATE, &kbdrate_s ))
     return 0;

   close( fd );

   return 1;
#else /* no KIOCSRATE */
   return 0;
#endif /* KIOCSRATE */
}

#undef rate

static void
SetKbdRepeat(InputInfoPtr pInfo, char rad)
{
  KbdDevPtr pKbd = (KbdDevPtr) pInfo->private;
  int i;
  int timeout;
  int         value = 0x7f;    /* Maximum delay with slowest rate */

#ifdef __sparc__
  int         rate  = 500;     /* Default rate */
  int         delay = 200;     /* Default delay */
#else
  int         rate  = 300;     /* Default rate */
  int         delay = 250;     /* Default delay */
#endif

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

  if(KDKBDREP_ioctl_ok(rate, delay)) 	/* m68k? */
    return;

  if(KIOCSRATE_ioctl_ok(rate, delay))	/* sparc? */
    return;

  if (xf86IsPc98())
    return;

#if defined(__alpha__) || defined (__i386__) || defined(__ia64__)

  /* The ioport way */

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

  timeout = KBC_TIMEOUT;
  while (((inb(0x64) & 2) == 2) && --timeout)
       usleep(1000); /* wait */

  if (timeout == 0)
      return;

  outb(0x60, 0xf3);             /* set typematic rate */
  while (((inb(0x64) & 2) == 2) && --timeout)
       usleep(1000); /* wait */

  usleep(10000);
  outb(0x60, value);

#endif /* __alpha__ || __i386__ || __ia64__ */
}

typedef struct {
   int kbdtrans;
   struct termios kbdtty;
} LnxKbdPrivRec, *LnxKbdPrivPtr;

static int
KbdInit(InputInfoPtr pInfo, int what)
{
    KbdDevPtr pKbd = (KbdDevPtr) pInfo->private;
    LnxKbdPrivPtr priv = (LnxKbdPrivPtr) pKbd->private;

    if (pKbd->isConsole) {
        ioctl (pInfo->fd, KDGKBMODE, &(priv->kbdtrans));
        tcgetattr (pInfo->fd, &(priv->kbdtty));
    }
    if (!pKbd->CustomKeycodes) {
        pKbd->RemapScanCode = ATScancode;
    }

    return Success;
}

static int
KbdOn(InputInfoPtr pInfo, int what)
{
    KbdDevPtr pKbd = (KbdDevPtr) pInfo->private;
    LnxKbdPrivPtr priv = (LnxKbdPrivPtr) pKbd->private;
    struct termios nTty;

    if (pKbd->isConsole) {
	if (pKbd->CustomKeycodes)
	    ioctl(pInfo->fd, KDSKBMODE, K_MEDIUMRAW);
	else
	    ioctl(pInfo->fd, KDSKBMODE, K_RAW);

	nTty = priv->kbdtty;
	nTty.c_iflag = (IGNPAR | IGNBRK) & (~PARMRK) & (~ISTRIP);
	nTty.c_oflag = 0;
	nTty.c_cflag = CREAD | CS8;
	nTty.c_lflag = 0;
	nTty.c_cc[VTIME]=0;
	nTty.c_cc[VMIN]=1;
	cfsetispeed(&nTty, 9600);
	cfsetospeed(&nTty, 9600);
	tcsetattr(pInfo->fd, TCSANOW, &nTty);
    }
    return Success;
}

static int
KbdOff(InputInfoPtr pInfo, int what)
{
    KbdDevPtr pKbd = (KbdDevPtr) pInfo->private;
    LnxKbdPrivPtr priv = (LnxKbdPrivPtr) pKbd->private;

    if (pKbd->isConsole) {
	ioctl(pInfo->fd, KDSKBMODE, priv->kbdtrans);
	tcsetattr(pInfo->fd, TCSANOW, &(priv->kbdtty));
    }
    return Success;
}

static int
GetSpecialKey(InputInfoPtr pInfo, int scanCode)
{
  KbdDevPtr pKbd = (KbdDevPtr) pInfo->private;
  int specialkey = scanCode;

#if defined (__sparc__)
  if (pKbd->sunKbd) {
      switch (scanCode) {
          case 0x2b: specialkey = KEY_BackSpace; break;
          case 0x47: specialkey = KEY_KP_Minus; break;
          case 0x7d: specialkey = KEY_KP_Plus; break;
          /* XXX needs cases for KEY_KP_Divide and KEY_KP_Multiply */
          case 0x05: specialkey = KEY_F1; break;
          case 0x06: specialkey = KEY_F2; break;
          case 0x08: specialkey = KEY_F3; break;
          case 0x0a: specialkey = KEY_F4; break;
          case 0x0c: specialkey = KEY_F5; break;
          case 0x0e: specialkey = KEY_F6; break;
          case 0x10: specialkey = KEY_F7; break;
          case 0x11: specialkey = KEY_F8; break;
          case 0x12: specialkey = KEY_F9; break;
          case 0x07: specialkey = KEY_F10; break;
          case 0x09: specialkey = KEY_F11; break;
          case 0x0b: specialkey = KEY_F12; break;
          default: specialkey = 0; break;
      }
      return specialkey;
  }
#endif

  if (pKbd->CustomKeycodes) {
      specialkey = pKbd->specialMap->map[scanCode];
  }
  return specialkey;
}

#define ModifierSet(k) ((modifiers & (k)) == (k))

static
Bool SpecialKey(InputInfoPtr pInfo, int key, Bool down, int modifiers)
{
  KbdDevPtr pKbd = (KbdDevPtr) pInfo->private;

  if(!pKbd->vtSwitchSupported)
      return FALSE;

  if ((ModifierSet(ControlMask | AltMask)) ||
      (ModifierSet(ControlMask | AltLangMask))) {
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
                    ioctl(xf86Info.consoleFd, VT_ACTIVATE, key - KEY_F1 + 1);
                    return TRUE;
                  }
             case KEY_F11:
             case KEY_F12:
                  if (down) {
                    ioctl(xf86Info.consoleFd, VT_ACTIVATE, key - KEY_F11 + 11);
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
                     ioctl(xf86Info.consoleFd, VT_ACTIVATE, key - KEY_F1 + 1);
                     VTSysreqToggle = FALSE;
                     return TRUE;
                 }
                 break;
            case KEY_F11:
            case KEY_F12:
                 if (VTSysreqToggle && down) {
                     ioctl(xf86Info.consoleFd, VT_ACTIVATE, key - KEY_F11 + 11);
                     VTSysreqToggle = FALSE;
                     return TRUE;
                 }
                 break;
            /* Ignore these keys -- ie don't let them cancel an alt-sysreq */
            case KEY_Alt:
            case KEY_AltLang:
                 break;
            case KEY_SysReqest:
                 if ((ModifierSet(AltMask) || ModifierSet(AltLangMask)) && down)
                     VTSysreqToggle = TRUE;
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
       for (i = 0; i < nBytes; i++)
           pKbd->PostEvent(pInfo, rBuf[i] & 0x7f,
                           rBuf[i] & 0x80 ? FALSE : TRUE);
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

Bool
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

    pKbd->RemapScanCode = NULL;
    pKbd->GetSpecialKey = GetSpecialKey;

    pKbd->OpenKeyboard = OpenKeyboard;
    pKbd->vtSwitchSupported = FALSE;

    pKbd->private = xcalloc(sizeof(LnxKbdPrivRec), 1);
    if (pKbd->private == NULL) {
       xf86Msg(X_ERROR,"can't allocate keyboard OS private data\n");
       return FALSE;
    }

#if defined(__powerpc__)
  {
    FILE *f;
    f = fopen("/proc/sys/dev/mac_hid/keyboard_sends_linux_keycodes","r");
    if (f) {
        if (fgetc(f) == '0')
            pKbd->CustomKeycodes = TRUE;
        fclose(f);
    }
  }
#endif
    return TRUE;
}
