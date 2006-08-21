/*
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany
 * Copyright 1993 by David Dawes <dawes@XFree86.org>
 * Copyright 1999 by David Holland <davidh@iquest.net)
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the names of Thomas Roell, David Dawes, and David Holland not be used
 * in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  Thomas Roell, David Dawes, and
 * David Holland make no representations about the suitability of this software
 * for any purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * THOMAS ROELL, DAVID DAWES, AND DAVID HOLLAND DISCLAIM ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL THOMAS ROELL, DAVID DAWES, OR DAVID HOLLAND
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
/* Copyright 2004-2005 Sun Microsystems, Inc.  All rights reserved.
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

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "xf86OSKbd.h"
#include "sun_kbd.h"

/* Define to provide support for using /dev/audio to ring the bell instead of
   the keyboard beeper */
#define AUDIO_BELL

#ifdef AUDIO_BELL
#include <sys/audio.h>
#include <sys/uio.h>
#include <limits.h>
#include <math.h>
#include <poll.h>
#endif

/***************************************************************************
 * Common implementation of routines shared by "keyboard" driver in sun_io.c
 * and "kbd" driver (later on in this file)
 */

#include <sys/stropts.h>
#include <sys/vuid_event.h>
#include <sys/kbd.h>

_X_HIDDEN int
sunKbdOpen(const char *devName, pointer options)
{
    int kbdFD;
    const char *kbdPath = NULL;
    const char *defaultKbd = "/dev/kbd";

    if (options != NULL) {
	kbdPath = xf86SetStrOption(options, "Device", NULL);
    }
    if (kbdPath == NULL) {
        kbdPath = defaultKbd;
    }

    kbdFD = open(kbdPath, O_RDONLY | O_NONBLOCK);
    
    if (kbdFD == -1) {
        xf86Msg(X_ERROR, "%s: cannot open \"%s\"\n", devName, kbdPath);
    } else {
	xf86MsgVerb(X_INFO, 3, "%s: Opened device \"%s\"\n", devName, kbdPath);
    }
    
    if ((kbdPath != NULL) && (kbdPath != defaultKbd)) {
	xfree(kbdPath);
    }
    return kbdFD;
}


/*
 * Save initial keyboard state.  This is called at the start of each server
 * generation.
 */

_X_HIDDEN int
sunKbdInit(sunKbdPrivPtr priv, int kbdFD, const char *devName, pointer options)
{
    int	ktype, klayout, i;
    const char *ktype_name;

    priv->kbdFD 	= kbdFD;
    priv->devName 	= devName;
    priv->otranslation 	= -1;
    priv->odirect 	= -1;

    if (options != NULL) {
	priv->strmod = xf86SetStrOption(options, "StreamsModule", NULL);
	priv->audioDevName = xf86SetStrOption(options, "BellDevice", NULL);

	if (priv->audioDevName && (priv->audioDevName[0] == '\0')) {
	    xfree(priv->audioDevName);
	    priv->audioDevName = NULL;
	}	
    } else {
	priv->strmod 		= NULL;
	priv->audioDevName	= NULL;
    }

    if (priv->strmod) {
	SYSCALL(i = ioctl(priv->kbdFD, I_PUSH, priv->strmod));
	if (i < 0) {
	    xf86Msg(X_ERROR,
		    "%s: cannot push module '%s' onto keyboard device: %s\n",
		    priv->devName, priv->strmod, strerror(errno));
	}
    }
    
    SYSCALL(i = ioctl(kbdFD, KIOCTYPE, &ktype));
    if (i < 0) {
	xf86Msg(X_ERROR, "%s: Unable to determine keyboard type: %s\n", 
		devName, strerror(errno));
	return BadImplementation;
    }
    
    SYSCALL(i = ioctl(kbdFD, KIOCLAYOUT, &klayout));
    if (i < 0) {	
	xf86Msg(X_ERROR, "%s: Unable to determine keyboard layout: %s\n", 
		devName, strerror(errno));
	return BadImplementation;
    }
    
    switch (ktype) {
    case KB_SUN3:
	ktype_name = "Sun Type 3"; break;
    case KB_SUN4:
	ktype_name = "Sun Type 4/5/6"; break;
    case KB_USB:
	ktype_name = "USB"; break;
    case KB_PC:
	ktype_name = "PC"; break;
    default:
	ktype_name = "Unknown"; break;
    }

    xf86Msg(X_PROBED, "%s: Keyboard type: %s (%d)\n",
	    devName, ktype_name, ktype);
    xf86Msg(X_PROBED, "%s: Keyboard layout: %d\n", devName, klayout);

    priv->ktype 	= ktype;
    priv->keyMap	= sunGetKbdMapping(ktype);
    priv->audioState	= AB_INITIALIZING;
    priv->oleds 	= sunKbdGetLeds(priv);

    return Success;
}

_X_HIDDEN int
sunKbdOn(sunKbdPrivPtr priv)
{
    int	ktrans, kdirect, i;

    SYSCALL(i = ioctl(priv->kbdFD, KIOCGDIRECT, &kdirect));
    if (i < 0) {
	xf86Msg(X_ERROR, 
		"%s: Unable to determine keyboard direct setting: %s\n", 
		priv->devName, strerror(errno));
	return BadImplementation;
    }

    priv->odirect = kdirect;
    kdirect = 1;

    SYSCALL(i = ioctl(priv->kbdFD, KIOCSDIRECT, &kdirect));
    if (i < 0) {
	xf86Msg(X_ERROR, "%s: Failed turning keyboard direct mode on: %s\n",
			priv->devName, strerror(errno));
	return BadImplementation;
    }

    /* Setup translation */

    SYSCALL(i = ioctl(priv->kbdFD, KIOCGTRANS, &ktrans));
    if (i < 0) {
	xf86Msg(X_ERROR, 
		"%s: Unable to determine keyboard translation mode: %s\n", 
		priv->devName, strerror(errno));
	return BadImplementation;
    }

    priv->otranslation = ktrans;
    ktrans = TR_UNTRANS_EVENT;

    SYSCALL(i = ioctl(priv->kbdFD, KIOCTRANS, &ktrans));
    if (i < 0) {	
	xf86Msg(X_ERROR, "%s: Failed setting keyboard translation mode: %s\n",
			priv->devName, strerror(errno));
	return BadImplementation;
    }

    return Success;
}

_X_HIDDEN int
sunKbdOff(sunKbdPrivPtr priv)
{
    int i;

    /* restore original state */
    
    sunKbdSetLeds(priv, priv->oleds);
    
    if (priv->otranslation != -1) {
        SYSCALL(i = ioctl(priv->kbdFD, KIOCTRANS, &priv->otranslation));
	if (i < 0) {
	    xf86Msg(X_ERROR,
		    "%s: Unable to restore keyboard translation mode: %s\n",
		    priv->devName, strerror(errno));
	    return BadImplementation;
	}
	priv->otranslation = -1;
    }

    if (priv->odirect != -1) {
        SYSCALL(i = ioctl(priv->kbdFD, KIOCSDIRECT, &priv->odirect));
	if (i < 0) {
	    xf86Msg(X_ERROR,
		    "%s: Unable to restore keyboard direct setting: %s\n",
		    priv->devName, strerror(errno));
	    return BadImplementation;
	}
	priv->odirect = -1;
    }

    if (priv->strmod) {
	SYSCALL(i = ioctl(priv->kbdFD, I_POP, priv->strmod));
	if (i < 0) {
            xf86Msg(X_WARNING,
		    "%s: cannot pop module '%s' off keyboard device: %s\n",
		    priv->devName, priv->strmod, strerror(errno));
	}
    }

    return Success;
}

#ifdef AUDIO_BELL

/* Helper function to ring bell via audio device instead of keyboard beeper */

#define BELL_RATE       48000   /* Samples per second */
#define BELL_HZ         50      /* Fraction of a second i.e. 1/x */
#define BELL_MS         (1000/BELL_HZ)  /* MS */
#define BELL_SAMPLES    (BELL_RATE / BELL_HZ)
#define BELL_MIN        3       /* Min # of repeats */

static int
sunKbdAudioBell(sunKbdPrivPtr priv, int loudness, int pitch, int duration)
{
    static short    samples[BELL_SAMPLES];
    static short    silence[BELL_SAMPLES]; /* "The Sound of Silence" */
    static int      lastFreq;
    int             cnt;
    int             i;
    int		    written;
    int             repeats;
    int             freq;
    audio_info_t    audioInfo;
    struct iovec    iov[IOV_MAX];
    int		    iovcnt;
    double	    ampl, cyclen, phase;
    int		    audioFD;

    if ((loudness <= 0) || (pitch <= 0) || (duration <= 0)) {
	return 0;
    }

    if ((priv == NULL) || (priv->audioDevName == NULL)) {
	return -1;
    }

    if (priv->audioState == AB_INITIALIZING) {
	priv->audioState = AB_NORMAL;
	lastFreq = 0;
	bzero(silence, sizeof(silence));
    }
    
    audioFD = open(priv->audioDevName, O_WRONLY | O_NONBLOCK);
    if (audioFD == -1) {
	xf86Msg(X_ERROR, "%s: cannot open audio device \"%s\": %s\n",
		priv->devName, priv->audioDevName, strerror(errno));
	return -1;
    } 

    freq = pitch;
    freq = min(freq, (BELL_RATE / 2) - 1);
    freq = max(freq, 2 * BELL_HZ);

    /*
     * Ensure full waves per buffer
     */
    freq -= freq % BELL_HZ;

    if (freq != lastFreq) {
	lastFreq = freq;
	ampl =  16384.0;

	cyclen = (double) freq / (double) BELL_RATE;
	phase = 0.0;

	for (i = 0; i < BELL_SAMPLES; i++) {
	    samples[i] = (short) (ampl * sin(2.0 * M_PI * phase));
	    phase += cyclen;
	    if (phase >= 1.0)
		phase -= 1.0;
	}
    }
    
    repeats = (duration + (BELL_MS / 2)) / BELL_MS;
    repeats = max(repeats, BELL_MIN);

    loudness = max(0, loudness);
    loudness = min(loudness, 100);

#ifdef DEBUG
    ErrorF("BELL : freq %d volume %d duration %d repeats %d\n",
	   freq, loudness, duration, repeats);
#endif

    AUDIO_INITINFO(&audioInfo);
    audioInfo.play.encoding = AUDIO_ENCODING_LINEAR;
    audioInfo.play.sample_rate = BELL_RATE;
    audioInfo.play.channels = 2;
    audioInfo.play.precision = 16;
    audioInfo.play.gain = min(AUDIO_MAX_GAIN, AUDIO_MAX_GAIN * loudness / 100);

    if (ioctl(audioFD, AUDIO_SETINFO, &audioInfo) < 0){
	xf86Msg(X_ERROR,
		"%s: AUDIO_SETINFO failed on audio device \"%s\": %s\n",
		priv->devName, priv->audioDevName, strerror(errno));
	close(audioFD);
	return -1;
    }
    
    iovcnt = 0;
    
    for (cnt = 0; cnt <= repeats; cnt++) {
	iov[iovcnt].iov_base = (char *) samples;
	iov[iovcnt++].iov_len = sizeof(samples);
	if (cnt == repeats) {
	    /* Insert a bit of silence so that multiple beeps are distinct and
	     * not compressed into a single tone.
	     */
	    iov[iovcnt].iov_base = (char *) silence;
	    iov[iovcnt++].iov_len = sizeof(silence);
	}	    
	if ((iovcnt >= IOV_MAX) || (cnt == repeats)) {
	    written = writev(audioFD, iov, iovcnt);

	    if ((written < ((int)(sizeof(samples) * iovcnt)))) {
		/* audio buffer was full! */

		int naptime;

		if (written == -1) {
		    if (errno != EAGAIN) {
			xf86Msg(X_ERROR,
			       "%s: writev failed on audio device \"%s\": %s\n",
				priv->devName, priv->audioDevName,
				strerror(errno));
			close(audioFD);
			return -1;
		    }
		    i = iovcnt;
		} else {
		    i = ((sizeof(samples) * iovcnt) - written)
			/ sizeof(samples);
		}
		cnt -= i;
		
		/* sleep a little to allow audio buffer to drain */
		naptime = BELL_MS * i;
		poll(NULL, 0, naptime);

		i = ((sizeof(samples) * iovcnt) - written) % sizeof(samples);
		iovcnt = 0;
		if ((written != -1) && (i > 0)) {
		    iov[iovcnt].iov_base = ((char *) samples) + i;
		    iov[iovcnt++].iov_len = sizeof(samples) - i;
		}
	    } else {
		iovcnt = 0;
	    }
	}
    }
    
    close(audioFD);
    return 0;
}

#endif /* AUDIO_BELL */

_X_HIDDEN void
sunKbdSoundBell(sunKbdPrivPtr priv, int loudness, int pitch, int duration)
{
    int	kbdCmd, i;

    if (loudness && pitch)
    {
#ifdef AUDIO_BELL
	if (priv->audioDevName != NULL) {
	    if (sunKbdAudioBell(priv, loudness, pitch, duration) == 0) {
		return;
	    }
	}
#endif
	
 	kbdCmd = KBD_CMD_BELL;
		
	SYSCALL(i = ioctl (priv->kbdFD, KIOCCMD, &kbdCmd));
	if (i < 0) {
	    xf86Msg(X_ERROR, "%s: Failed to activate bell: %s\n",
                priv->devName, strerror(errno));
	}
	
	usleep(duration * loudness * 20);
	
	kbdCmd = KBD_CMD_NOBELL;
	SYSCALL(i = ioctl (priv->kbdFD, KIOCCMD, &kbdCmd));
	if (i < 0) {
	     xf86Msg(X_ERROR, "%s: Failed to deactivate bell: %s\n",
                priv->devName, strerror(errno));
	}
    }
}

_X_HIDDEN void
sunKbdSetLeds(sunKbdPrivPtr priv, int leds)
{
    int i;
	
    SYSCALL(i = ioctl(priv->kbdFD, KIOCSLED, &leds));
    if (i < 0) {
	xf86Msg(X_ERROR, "%s: Failed to set keyboard LED's: %s\n",
                priv->devName, strerror(errno));
    }
}

_X_HIDDEN int
sunKbdGetLeds(sunKbdPrivPtr priv)
{
    int i, leds = 0;

    SYSCALL(i = ioctl(priv->kbdFD, KIOCGLED, &leds));
    if (i < 0) {
        xf86Msg(X_ERROR, "%s: Failed to get keyboard LED's: %s\n",
                priv->devName, strerror(errno));
    }
    return leds;
}

/* ARGSUSED0 */
_X_HIDDEN void
sunKbdSetRepeat(sunKbdPrivPtr priv, char rad)
{
    /* Nothing to do */
}

/***************************************************************************
 * Routines called from "kbd" driver via proc vectors filled in by
 * xf86OSKbdPreInit().
 */


static int
KbdInit(InputInfoPtr pInfo, int what)
{
    KbdDevPtr pKbd = (KbdDevPtr) pInfo->private;
    sunKbdPrivPtr priv = (sunKbdPrivPtr) pKbd->private;

    return sunKbdInit(priv, pInfo->fd, pInfo->name, pInfo->options);
}


static int
KbdOn(InputInfoPtr pInfo, int what)
{
    KbdDevPtr pKbd = (KbdDevPtr) pInfo->private;
    sunKbdPrivPtr priv = (sunKbdPrivPtr) pKbd->private;

    return sunKbdOn(priv);
}

static int
KbdOff(InputInfoPtr pInfo, int what)
{
    KbdDevPtr pKbd = (KbdDevPtr) pInfo->private;
    sunKbdPrivPtr priv = (sunKbdPrivPtr) pKbd->private;

    return sunKbdOff(priv);
}


static void
SoundKbdBell(InputInfoPtr pInfo, int loudness, int pitch, int duration)
{
    KbdDevPtr pKbd = (KbdDevPtr) pInfo->private;
    sunKbdPrivPtr priv = (sunKbdPrivPtr) pKbd->private;

    sunKbdSoundBell(priv, loudness, pitch, duration);
}

static void
SetKbdLeds(InputInfoPtr pInfo, int leds)
{
    KbdDevPtr pKbd = (KbdDevPtr) pInfo->private;
    sunKbdPrivPtr priv = (sunKbdPrivPtr) pKbd->private;
    int real_leds = sunKbdGetLeds(priv);

    real_leds &= ~(LED_CAPS_LOCK | LED_NUM_LOCK | LED_SCROLL_LOCK | LED_COMPOSE);

    if (leds & XLED1)  real_leds |= LED_CAPS_LOCK;
    if (leds & XLED2)  real_leds |= LED_NUM_LOCK;
    if (leds & XLED3)  real_leds |= LED_SCROLL_LOCK;
    if (leds & XLED4)  real_leds |= LED_COMPOSE;
    
    sunKbdSetLeds(priv, real_leds);
}

static int
GetKbdLeds(InputInfoPtr pInfo)
{
    KbdDevPtr pKbd = (KbdDevPtr) pInfo->private;
    sunKbdPrivPtr priv = (sunKbdPrivPtr) pKbd->private;
    int leds = 0;
    int real_leds = sunKbdGetLeds(priv);

    if (real_leds & LED_CAPS_LOCK)	leds |= XLED1;
    if (real_leds & LED_NUM_LOCK)	leds |= XLED2;
    if (real_leds & LED_SCROLL_LOCK)	leds |= XLED3;
    if (real_leds & LED_COMPOSE)	leds |= XLED4;
	
    return leds;
}

static void
SetKbdRepeat(InputInfoPtr pInfo, char rad)
{
    KbdDevPtr pKbd = (KbdDevPtr) pInfo->private;
    sunKbdPrivPtr priv = (sunKbdPrivPtr) pKbd->private;
    
    sunKbdSetRepeat(priv, rad);
}

static void
KbdGetMapping (InputInfoPtr pInfo, KeySymsPtr pKeySyms, CARD8 *pModMap)
{
    /* Should probably do something better here */
    xf86KbdGetMapping(pKeySyms, pModMap);
}

static void
ReadInput(InputInfoPtr pInfo)
{
    KbdDevPtr pKbd = (KbdDevPtr) pInfo->private;
    sunKbdPrivPtr priv = (sunKbdPrivPtr) pKbd->private;
    Firm_event event[64];
    int        nBytes, i;

    /* I certainly hope its not possible to read partial events */

    if ((nBytes = read(pInfo->fd, (char *)event, sizeof(event))) > 0)
    {
        for (i = 0; i < (nBytes / sizeof(Firm_event)); i++) {
	    pKbd->PostEvent(pInfo, priv->keyMap[event[i].id],
			    event[i].value == VKEY_DOWN ? TRUE : FALSE);
	}
    }
}

static Bool
OpenKeyboard(InputInfoPtr pInfo)
{
    pInfo->fd = sunKbdOpen(pInfo->name, pInfo->options);

    if (pInfo->fd >= 0) {
	pInfo->read_input = ReadInput;
	return TRUE;
    } else {
	return FALSE;
    }
}

_X_EXPORT Bool
xf86OSKbdPreInit(InputInfoPtr pInfo)
{
    KbdDevPtr pKbd = pInfo->private;

    pKbd->KbdInit       = KbdInit;
    pKbd->KbdOn         = KbdOn;
    pKbd->KbdOff        = KbdOff;
    pKbd->Bell          = SoundKbdBell;
    pKbd->SetLeds       = SetKbdLeds;
    pKbd->GetLeds       = GetKbdLeds;
    pKbd->SetKbdRepeat  = SetKbdRepeat;
    pKbd->KbdGetMapping = KbdGetMapping;

    pKbd->RemapScanCode = NULL;
    pKbd->GetSpecialKey = NULL;
    pKbd->SpecialKey    = NULL;

    pKbd->OpenKeyboard = OpenKeyboard;

    pKbd->vtSwitchSupported = FALSE;
    pKbd->CustomKeycodes = FALSE;

    pKbd->private = xcalloc(sizeof(sunKbdPrivRec), 1);
    if (pKbd->private == NULL) {
       xf86Msg(X_ERROR,"can't allocate keyboard OS private data\n");
       return FALSE;
    } else {
	sunKbdPrivPtr priv = (sunKbdPrivPtr) pKbd->private;
	priv->otranslation = -1;
	priv->odirect = -1;
    }

    return TRUE;
}
