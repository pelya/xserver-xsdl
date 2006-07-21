/* $XdotOrg: $ */
/*
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany
 * Copyright 1993 by David Dawes <dawes@xfree86.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of Thomas Roell and David Dawes
 * not be used in advertising or publicity pertaining to distribution of
 * the software without specific, written prior permission.  Thomas Roell and
 * David Dawes makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * THOMAS ROELL AND DAVID DAWES DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
 * IN NO EVENT SHALL THOMAS ROELL OR DAVID DAWES BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons
 * to whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
 * OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * HOLDERS INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL
 * INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * Except as contained in this notice, the name of a copyright holder
 * shall not be used in advertising or otherwise to promote the sale, use
 * or other dealings in this Software without prior written authorization
 * of the copyright holder.
 */

/* Solaris support routines for builtin "keyboard" driver */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "sun_kbd.h"

static sunKbdPrivRec sunKeyboardPriv;

_X_HIDDEN void
xf86KbdInit(void)
{
    const char *kbdName = "keyboard";
    pointer *kbdOptions = NULL;
    IDevPtr pDev;

    /* There should be a better way to find the keyboard device name, but
       this seems to work for now. */
    for (pDev = xf86ConfigLayout.inputs; pDev && pDev->identifier; pDev++) {
	if (!xf86NameCmp(pDev->driver, "keyboard")) {
	    kbdName = pDev->identifier;
	    kbdOptions = pDev->commonOptions;
	    break;
	}
    }

    if (xf86Info.kbdFd < 0) {
	xf86Info.kbdFd = sunKbdOpen(kbdName, kbdOptions);
	if (xf86Info.kbdFd < 0) {
	    FatalError("Unable to open keyboard: /dev/kbd\n");
	}
    }

    memset(&sunKeyboardPriv, 0, sizeof(sunKbdPrivRec));    
    if (sunKbdInit(&sunKeyboardPriv, xf86Info.kbdFd,
		   kbdName, kbdOptions)	!= Success) {
    	FatalError("Unable to initialize keyboard driver\n");
    }
}

_X_HIDDEN int
xf86KbdOn(void)
{
    if (sunKbdOn(&sunKeyboardPriv) != Success) {
	FatalError("Enabling keyboard");
    }

    return xf86Info.kbdFd;
}

_X_HIDDEN int
xf86KbdOff(void)
{
    if (sunKbdOff(&sunKeyboardPriv) != Success) {
	FatalError("Disabling keyboard");
    }

    return xf86Info.kbdFd;
}

_X_EXPORT void
xf86SoundKbdBell(int loudness, int pitch, int duration)
{
    sunKbdSoundBell(&sunKeyboardPriv, loudness, pitch, duration);
}

_X_HIDDEN void
xf86SetKbdLeds(int leds)
{
    sunKbdSetLeds(&sunKeyboardPriv, leds);
}

_X_HIDDEN int
xf86GetKbdLeds(void)
{
    return sunKbdGetLeds(&sunKeyboardPriv);
}

_X_HIDDEN void
xf86SetKbdRepeat(char rad)
{
    sunKbdSetRepeat(&sunKeyboardPriv, rad);
}

/*
 * Lets try reading more than one keyboard event at a time in the hopes that
 * this will be slightly more efficient.  Or we could just try the MicroSoft
 * method, and forget about efficiency. :-)
 */
_X_HIDDEN void
xf86KbdEvents(void)
{
    Firm_event event[64];
    int        nBytes, i;

    /* I certainly hope its not possible to read partial events */

    if ((nBytes = read(xf86Info.kbdFd, (char *)event, sizeof(event))) > 0)
    {
	for (i = 0; i < (nBytes / sizeof(Firm_event)); i++)
	    sunPostKbdEvent(sunKeyboardPriv.ktype, &event[i]);
    }
}
