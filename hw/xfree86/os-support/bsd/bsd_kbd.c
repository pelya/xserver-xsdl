
/*
 * Copyright (c) 2002 by The XFree86 Project, Inc.
 * Author: Ivan Pascal.
 *
 * Based on the code from bsd_io.c which is
 * Copyright 1992 by Rich Murphey <Rich@Rice.edu>
 * Copyright 1993 by David Dawes <dawes@xfree86.org>
 */

#define NEED_EVENTS
#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <X11/X.h>
#include <termios.h>

#include "compiler.h"

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"

#include "xf86Xinput.h"
#include "xf86OSKbd.h"
#include "atKeynames.h"
#include "bsd_kbd.h"

extern Bool VTSwitchEnabled;
#ifdef USE_VT_SYSREQ
extern Bool VTSysreqToggle;
#endif

static KbdProtocolRec protocols[] = {
   {"standard", PROT_STD },
#ifdef WSCONS_SUPPORT
   {"wskbd", PROT_WSCONS },
#endif
   { NULL, PROT_UNKNOWN_KBD }
};

typedef struct {
   struct termios kbdtty;
} BsdKbdPrivRec, *BsdKbdPrivPtr;

static
int KbdInit(InputInfoPtr pInfo, int what)
{
    KbdDevPtr pKbd = (KbdDevPtr) pInfo->private;
    BsdKbdPrivPtr priv = (BsdKbdPrivPtr) pKbd->private;

    if (pKbd->isConsole) {
        switch (pKbd->consType) {
#if defined(PCCONS_SUPPORT) || defined(SYSCONS_SUPPORT) || defined (PCVT_SUPPORT)  || defined (WSCONS_SUPPORT)
	    case PCCONS:
	    case SYSCONS:
	    case PCVT:
#if defined WSCONS_SUPPORT
            case WSCONS:
#endif
 	         tcgetattr(pInfo->fd, &(priv->kbdtty));
#endif
	         break;
        }
    }

    return Success;
}

static void
SetKbdLeds(InputInfoPtr pInfo, int leds)
{
    KbdDevPtr pKbd = (KbdDevPtr) pInfo->private;
    int real_leds = 0;

#ifdef LED_CAP
    if (leds & XLED1)  real_leds |= LED_CAP;
#endif
#ifdef LED_NUM
    if (leds & XLED2)  real_leds |= LED_NUM;
#endif
#ifdef LED_SCR
    if (leds & XLED3)  real_leds |= LED_SCR;
    if (leds & XLED4)  real_leds |= LED_SCR;
#endif

    switch (pKbd->consType) {

	case PCCONS:
		break;
#if defined (SYSCONS_SUPPORT) || defined (PCVT_SUPPORT)
	case SYSCONS:
	case PCVT:
	     ioctl(pInfo->fd, KDSETLED, real_leds);
	     break;
#endif
#if defined(WSCONS_SUPPORT)
        case WSCONS:
             ioctl(pInfo->fd, WSKBDIO_SETLEDS, &real_leds);
             break;
#endif
    }
}

static int
GetKbdLeds(InputInfoPtr pInfo)
{
    KbdDevPtr pKbd = (KbdDevPtr) pInfo->private;
    int leds = 0, real_leds = 0;

    switch (pKbd->consType) {
	case PCCONS:
	     break;
#if defined (SYSCONS_SUPPORT) || defined (PCVT_SUPPORT)
	case SYSCONS:
	case PCVT:
	     ioctl(pInfo->fd, KDGETLED, &real_leds);
	     break;
#endif
#if defined(WSCONS_SUPPORT)
        case WSCONS:
             ioctl(pInfo->fd, WSKBDIO_GETLEDS, &real_leds);
             break;
#endif
    }

#ifdef LED_CAP
    if (real_leds & LED_CAP) leds |= XLED1;
#endif
#ifdef LED_NUM
    if (real_leds & LED_NUM) leds |= XLED2;
#endif
#ifdef LED_SCR
    if (real_leds & LED_SCR) leds |= XLED3;
#endif

    return(leds);
}

static void
SetKbdRepeat(InputInfoPtr pInfo, char rad)
{
    KbdDevPtr pKbd = (KbdDevPtr) pInfo->private;
    switch (pKbd->consType) {

	case PCCONS:
		break;
#if defined (SYSCONS_SUPPORT) || defined (PCVT_SUPPORT)
	case SYSCONS:
	case PCVT:
		ioctl(pInfo->fd, KDSETRAD, rad);
		break;
#endif
    }
}

static int
KbdOn(InputInfoPtr pInfo, int what)
{
    KbdDevPtr pKbd = (KbdDevPtr) pInfo->private;
#if defined(SYSCONS_SUPPORT) || defined(PCCONS_SUPPORT) || defined(PCVT_SUPPORT) || defined(WSCONS_SUPPORT)
    BsdKbdPrivPtr priv = (BsdKbdPrivPtr) pKbd->private;
    struct termios nTty;
#endif
#ifdef WSCONS_SUPPORT
    int option;
#endif

    if (pKbd->isConsole) {
        switch (pKbd->consType) {

#if defined(SYSCONS_SUPPORT) || defined(PCCONS_SUPPORT) || defined(PCVT_SUPPORT) || defined(WSCONS_SUPPORT)
	    case SYSCONS:
	    case PCCONS:
	    case PCVT:
#ifdef WSCONS_SUPPORT
            case WSCONS:
#endif
		 nTty = priv->kbdtty;
		 nTty.c_iflag = IGNPAR | IGNBRK;
		 nTty.c_oflag = 0;
		 nTty.c_cflag = CREAD | CS8;
		 nTty.c_lflag = 0;
		 nTty.c_cc[VTIME] = 0;
		 nTty.c_cc[VMIN] = 1;
		 cfsetispeed(&nTty, 9600);
		 cfsetospeed(&nTty, 9600);
		 if (tcsetattr(pInfo->fd, TCSANOW, &nTty) < 0) {
			 xf86Msg(X_ERROR, "KbdOn: tcsetattr: %s\n",
			     strerror(errno));
		 }
                 break; 
#endif 
        }
#if defined (SYSCONS_SUPPORT) || defined (PCVT_SUPPORT) || defined (WSCONS_SUPPORT)
        switch (pKbd->consType) {
	    case SYSCONS:
	    case PCVT:
#ifdef K_CODE
                 if (pKbd->CustomKeycodes)
		     ioctl(pInfo->fd, KDSKBMODE, K_CODE);
	         else
	             ioctl(pInfo->fd, KDSKBMODE, K_RAW);
#else
		 ioctl(pInfo->fd, KDSKBMODE, K_RAW);
#endif
	         break;
#endif
#ifdef WSCONS_SUPPORT
            case WSCONS:
                 option = WSKBD_RAW;
                 if (ioctl(pInfo->fd, WSKBDIO_SETMODE, &option) == -1) {
			 FatalError("can't switch keyboard to raw mode. "
				    "Enable support for it in the kernel\n"
				    "or use for example:\n\n"
				    "Option \"Protocol\" \"wskbd\"\n"
				    "Option \"Device\" \"/dev/wskbd0\"\n"
				    "\nin your xorg.conf(5) file\n");
		 }
		 break;
#endif
        }
    }
    return Success;
}

static int
KbdOff(InputInfoPtr pInfo, int what)
{
    KbdDevPtr pKbd = (KbdDevPtr) pInfo->private;
    BsdKbdPrivPtr priv = (BsdKbdPrivPtr) pKbd->private;
#ifdef WSCONS_SUPPORT
    int option;
#endif

    if (pKbd->isConsole) {
        switch (pKbd->consType) {
#if defined (SYSCONS_SUPPORT) || defined (PCVT_SUPPORT)
	    case SYSCONS:
	    case PCVT:
	         ioctl(pInfo->fd, KDSKBMODE, K_XLATE);
	         /* FALL THROUGH */
#endif
#if defined(SYSCONS_SUPPORT) || defined(PCCONS_SUPPORT) || defined(PCVT_SUPPORT)
	    case PCCONS:
	         tcsetattr(pInfo->fd, TCSANOW, &(priv->kbdtty));
	         break;
#endif
#ifdef WSCONS_SUPPORT
            case WSCONS:
                 option = WSKBD_TRANSLATED;
                 ioctl(xf86Info.consoleFd, WSKBDIO_SETMODE, &option);
                 tcsetattr(pInfo->fd, TCSANOW, &(priv->kbdtty));
	         break;
#endif
        }
    }
    return Success;
}

static void
SoundBell(InputInfoPtr pInfo, int loudness, int pitch, int duration)
{
    KbdDevPtr pKbd = (KbdDevPtr) pInfo->private;
#ifdef WSCONS_SUPPORT
    struct wskbd_bell_data wsb;
#endif

    if (loudness && pitch) {
    	switch (pKbd->consType) {
#ifdef PCCONS_SUPPORT
	    case PCCONS:
	         { int data[2];
		   data[0] = pitch;
		   data[1] = (duration * loudness) / 50;
		   ioctl(pInfo->fd, CONSOLE_X_BELL, data);
		   break;
		 }
#endif
#if defined (SYSCONS_SUPPORT) || defined (PCVT_SUPPORT)
	    case SYSCONS:
	    case PCVT:
		 ioctl(pInfo->fd, KDMKTONE,
		 ((1193190 / pitch) & 0xffff) |
		 (((unsigned long)duration*loudness/50)<<16));
		 break;
#endif
#if defined (WSCONS_SUPPORT)
            case WSCONS:
                 wsb.which = WSKBD_BELL_DOALL;
                 wsb.pitch = pitch;
                 wsb.period = duration;
                 wsb.volume = loudness;
                 ioctl(pInfo->fd, WSKBDIO_COMPLEXBELL, &wsb);
                 break;
#endif
	}
    }
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
      if (VTSwitchEnabled && !xf86Info.vtSysreq && !xf86Info.dontVTSwitch) {
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
#ifdef VT_ACTIVATE
                  if (down) {
                    ioctl(xf86Info.consoleFd, VT_ACTIVATE, key - KEY_F1 + 1);
                    return TRUE;
                  }
#endif
             case KEY_F11:
             case KEY_F12:
#ifdef VT_ACTIVATE
                  if (down) {
                    ioctl(xf86Info.consoleFd, VT_ACTIVATE, key - KEY_F11 + 11);
                    return TRUE;
                  }
#endif
         }
      }
    }
#ifdef USE_VT_SYSREQ
    if (VTSwitchEnabled && xf86Info.vtSysreq && !xf86Info.dontVTSwitch) {
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

#ifdef WSCONS_SUPPORT

static void
WSReadInput(InputInfoPtr pInfo)
{
    KbdDevPtr pKbd = (KbdDevPtr) pInfo->private;
    struct wscons_event events[64];
    int type;
    int blocked, n, i;

    if ((n = read( pInfo->fd, events, sizeof(events))) > 0) {
        n /=  sizeof(struct wscons_event);
        for (i = 0; i < n; i++) {
	    type = events[i].type;
	    if (type == WSCONS_EVENT_KEY_UP || type == WSCONS_EVENT_KEY_DOWN) {
		/* It seems better to block SIGIO there */
		blocked = xf86BlockSIGIO();
		pKbd->PostEvent(pInfo, (unsigned int)(events[i].value),
				type == WSCONS_EVENT_KEY_DOWN ? TRUE : FALSE);
		xf86UnblockSIGIO(blocked);
	    }
	} /* for */
    }
}

static void
printWsType(char *type, char *devname)
{
    xf86Msg(X_PROBED, "%s: Keyboard type: %s\n", type, devname); 
}
#endif

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
#ifdef WSCONS_SUPPORT
        case PROT_WSCONS:
           pInfo->read_input = WSReadInput;
           break;
#endif
        default:
           xf86Msg(X_ERROR,"\"%s\" is not a valid keyboard protocol name\n", s);
           xfree(s);
           return FALSE;
    }
    xf86Msg(X_CONFIG, "%s: Protocol: %s\n", pInfo->name, s);
    xfree(s);

    s = xf86SetStrOption(pInfo->options, "Device", NULL);
    if (s == NULL) {
       if (prot == PROT_WSCONS) {
           xf86Msg(X_ERROR,"A \"device\" option is required with"
                                  " the \"wskbd\" keyboard protocol\n");
           return FALSE;
       } else {
           pInfo->fd = xf86Info.consoleFd;
           pKbd->isConsole = TRUE;
           pKbd->consType = xf86Info.consType;
       }
    } else {
	pInfo->fd = open(s, O_RDONLY | O_NONBLOCK | O_EXCL);
       if (pInfo->fd == -1) {
           xf86Msg(X_ERROR, "%s: cannot open \"%s\"\n", pInfo->name, s);
           xfree(s);
           return FALSE;
       }
       pKbd->isConsole = FALSE;
       pKbd->consType = xf86Info.consType;
       xfree(s);
    }

#if defined (SYSCONS_SUPPORT) || defined (PCVT_SUPPORT)
    if (pKbd->isConsole &&
        ((pKbd->consType == SYSCONS) || (pKbd->consType == PCVT)))
        pKbd->vtSwitchSupported = TRUE;
#endif

#ifdef WSCONS_SUPPORT
    if( prot == PROT_WSCONS) {
       pKbd->consType = WSCONS;
       /* Find out keyboard type */
       if (ioctl(pInfo->fd, WSKBDIO_GTYPE, &(pKbd->wsKbdType)) == -1) {
           xf86Msg(X_ERROR, "%s: cannot get keyboard type", pInfo->name);
           close(pInfo->fd);
           return FALSE;
       }
       switch (pKbd->wsKbdType) {
           case WSKBD_TYPE_PC_XT:
               printWsType("XT", pInfo->name);
               break;
           case WSKBD_TYPE_PC_AT:
               printWsType("AT", pInfo->name);
               break;
           case WSKBD_TYPE_USB:
               printWsType("USB", pInfo->name);
               break;
#ifdef WSKBD_TYPE_ADB
           case WSKBD_TYPE_ADB:
               printWsType("ADB", pInfo->name);
               break;
#endif
#ifdef WSKBD_TYPE_SUN
           case WSKBD_TYPE_SUN:
               printWsType("Sun", pInfo->name);
               break;
#endif
#ifdef WSKBD_TYPE_SUN5
     case WSKBD_TYPE_SUN5:
	     xf86Msg(X_PROBED, "Keyboard type: Sun5\n");
	     break;
#endif
           default:
               xf86Msg(X_ERROR, "%s: Unsupported wskbd type \"%d\"",
                                pInfo->name, pKbd->wsKbdType);
               close(pInfo->fd);
               return FALSE;
       }
    }
#endif
    return TRUE;
}

_X_EXPORT Bool
xf86OSKbdPreInit(InputInfoPtr pInfo)
{
    KbdDevPtr pKbd = pInfo->private;

    pKbd->KbdInit	= KbdInit;
    pKbd->KbdOn		= KbdOn;
    pKbd->KbdOff	= KbdOff;
    pKbd->Bell		= SoundBell;
    pKbd->SetLeds	= SetKbdLeds;
    pKbd->GetLeds	= GetKbdLeds;
    pKbd->SetKbdRepeat	= SetKbdRepeat;
    pKbd->KbdGetMapping	= KbdGetMapping;
    pKbd->SpecialKey	= SpecialKey;

    pKbd->RemapScanCode = NULL;
    pKbd->GetSpecialKey = NULL;

    pKbd->OpenKeyboard = OpenKeyboard;
    pKbd->vtSwitchSupported = FALSE;
    pKbd->CustomKeycodes = FALSE;
    
    pKbd->private = xcalloc(sizeof(BsdKbdPrivRec), 1);
    if (pKbd->private == NULL) {
       xf86Msg(X_ERROR,"can't allocate keyboard OS private data\n");
       return FALSE;
    }
    return TRUE;
}
