/* $Xorg: xf86XKB.c,v 1.3 2000/08/17 19:50:31 cpqbld Exp $ */
/************************************************************
Copyright (c) 1993 by Silicon Graphics Computer Systems, Inc.

Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and without
fee is hereby granted, provided that the above copyright
notice appear in all copies and that both that copyright
notice and this permission notice appear in supporting
documentation, and that the name of Silicon Graphics not be 
used in advertising or publicity pertaining to distribution 
of the software without specific prior written permission.
Silicon Graphics makes no representation about the suitability 
of this software for any purpose. It is provided "as is"
without any express or implied warranty.

SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS 
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY 
AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL 
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, 
DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE 
OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/
/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86XKB.c,v 3.6 1996/12/28 11:14:43 dawes Exp $ */

#include <stdio.h>
#define	NEED_EVENTS 1
#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/keysym.h>
#include "inputstr.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "XI.h"

#include "compiler.h"

#include "xf86Procs.h"
#include "xf86_OSlib.h"
#include "xf86_Config.h"

#include "XKBsrv.h"

#ifdef AMOEBA
#define LED_CAP	IOP_LED_CAP
#define LED_NUM	IOP_LED_NUM
#define LED_SCR	IOP_LED_SCROLL
#endif

#ifdef MINIX
#define LED_CAP KBD_LEDS_CAPS
#define LED_NUM KBD_LEDS_NUM
#define LED_SCR KBD_LEDS_SCROLL
#endif

void
xf86InitXkb()
{
}

void
#if NeedFunctionPrototypes
XkbDDXUpdateIndicators(DeviceIntPtr pXDev,CARD32 new)
#else
XkbDDXUpdateIndicators(pXDev,new)
    DeviceIntPtr  pXDev;
    CARD32 new;
#endif
{
    CARD32 old;
#ifdef DEBUG
/*    if (xkbDebugFlags)*/
        ErrorF("XkbDDXUpdateIndicators(...,0x%x) -- XFree86 version\n",new);
#endif
#ifdef LED_CAP
    old= new;
    new= 0;
    if (old&XLED1)	new|= LED_CAP;
    if (old&XLED2)	new|= LED_NUM;
    if (old&XLED3)	new|= LED_SCR;
#endif
    xf86SetKbdLeds(new);
    return;
}

void
#if NeedFunctionPrototypes
XkbDDXUpdateDeviceIndicators(	DeviceIntPtr		dev,
				XkbSrvLedInfoPtr 	sli,
				CARD32 			new)
#else
XkbDDXUpdateDeviceIndicators(dev,sli,new)
    DeviceIntPtr  	dev;
    XkbSrvLedInfoPtr	sli;
    CARD32 		new;
#endif
{
    if (sli->fb.kf==dev->kbdfeed)
	XkbDDXUpdateIndicators(dev,new);
    else if (sli->class==KbdFeedbackClass) {
	KbdFeedbackPtr	kf;
	kf= sli->fb.kf;
	if (kf && kf->CtrlProc) {
	    (*kf->CtrlProc)(dev,&kf->ctrl);
	}
    }
    else if (sli->class==LedFeedbackClass) {
	LedFeedbackPtr	lf;
	lf= sli->fb.lf;
	if (lf && lf->CtrlProc) {
	    (*lf->CtrlProc)(dev,&lf->ctrl);
	}
    }
    return;
}
