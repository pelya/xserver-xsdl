/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/sunos/sun_mouse.c,v 1.4 2002/01/25 21:56:21 tsi Exp $ */
/* $XdotOrg: xc/programs/Xserver/hw/xfree86/os-support/sunos/sun_mouse.c,v 1.2 2004/04/23 19:54:13 eich Exp $ */
/*
 * Copyright 1999-2001 The XFree86 Project, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the XFree86 Project shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from the
 * XFree86 Project.
 */
/* Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
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

#include "xf86.h"
#include "xf86_OSlib.h"
#include "xf86OSmouse.h"

#if defined(__SOL8__) || !defined(__i386)

#include "xisb.h"
#include "mipointer.h"
#include <sys/stropts.h>
#include <sys/vuid_event.h>
#include <sys/msio.h>

/* Wheel mouse support in VUID drivers in Solaris 9 updates & Solaris 10 */
#ifdef WHEEL_DEVID /* Defined in vuid_event.h if VUID wheel support present */
# define HAVE_VUID_WHEEL
#endif
#ifdef HAVE_VUID_WHEEL
# include <sys/vuid_wheel.h>
#endif

/* Names of protocols that are handled internally here. */

static const char *internalNames[] = {
	"VUID",
	NULL
};

static const char *solarisMouseDevs[] = {
    /* Device file:	Protocol: 			*/
    "/dev/mouse",	"VUID",		/* USB or SPARC */
#ifdef __i386
    "/dev/kdmouse",	"PS/2",		/* PS/2 */
#endif
    NULL
};

typedef struct _VuidMseRec {
    Firm_event event;
    unsigned char *buffer;
    char *strmod;
} VuidMseRec, *VuidMsePtr;


static int  vuidMouseProc(DeviceIntPtr pPointer, int what);
static void vuidReadInput(InputInfoPtr pInfo);

/*
 * Initialize and enable the mouse wheel, if present.
 *
 * Returns 1 if mouse wheel was successfully enabled.
 * Returns 0 if an error occurred or if there is no mouse wheel.
 */
static int
vuidMouseWheelInit(InputInfoPtr pInfo)
{
#ifdef HAVE_VUID_WHEEL
    wheel_state wstate;
    int nwheel = -1;

    wstate.vers = VUID_WHEEL_STATE_VERS;
    wstate.id = 0;
    wstate.stateflags = -1;

    if (ioctl(pInfo->fd, VUIDGWHEELCOUNT, &nwheel) != 0)
	return (0);

    if (ioctl(pInfo->fd, VUIDGWHEELSTATE, &wstate) != 0) {
	xf86Msg(X_WARNING, "%s: couldn't get wheel state\n", pInfo->name);
	return (0);
    }

    wstate.stateflags |= VUID_WHEEL_STATE_ENABLED;

    if (ioctl(pInfo->fd, VUIDSWHEELSTATE, &wstate) != 0) {
	xf86Msg(X_WARNING, "%s: couldn't enable wheel\n", pInfo->name);
	return (0);
    }

    return (1);
#else
    return (0);
#endif
}


/* This function is called when the protocol is "VUID". */
static Bool
vuidPreInit(InputInfoPtr pInfo, const char *protocol, int flags)
{
    MouseDevPtr pMse = pInfo->private;
    VuidMsePtr pVuidMse;
    int buttons;

    pVuidMse = xalloc(sizeof(VuidMseRec));
    if (pVuidMse == NULL) {
	xf86Msg(X_ERROR, "%s: cannot allocate VuidMouseRec\n", pInfo->name);
	xfree(pMse);
	return FALSE;
    }

    pMse->protocol = protocol;
    xf86Msg(X_CONFIG, "%s: Protocol: %s\n", pInfo->name, protocol);

    /* Collect the options, and process the common options. */
    xf86CollectInputOptions(pInfo, NULL, NULL);
    xf86ProcessCommonOptions(pInfo, pInfo->options);

    /* Check if the device can be opened. */
    pInfo->fd = xf86OpenSerial(pInfo->options);
    if (pInfo->fd == -1) {
	if (xf86GetAllowMouseOpenFail())
	    xf86Msg(X_WARNING, "%s: cannot open input device\n", pInfo->name);
	else {
	    xf86Msg(X_ERROR, "%s: cannot open input device\n", pInfo->name);
	    xfree(pVuidMse);
	    xfree(pMse);
	    return FALSE;
	}
    }

    pVuidMse->buffer = (unsigned char *)&pVuidMse->event;

    pVuidMse->strmod = xf86SetStrOption(pInfo->options, "StreamsModule", NULL);
    if (pVuidMse->strmod && 
      	(ioctl(pInfo->fd, I_PUSH, pVuidMse->strmod) == -1)) {
	xf86Msg(X_ERROR,
	  "%s: cannot push module '%s' onto mouse device: %s\n",
	  pInfo->name, pVuidMse->strmod, strerror(errno));
	xf86CloseSerial(pInfo->fd);
	pInfo->fd = -1;
	xfree(pVuidMse);
	xfree(pMse);
	return FALSE;
    }

    buttons = xf86SetIntOption(pInfo->options, "Buttons", 0);
    if (buttons == 0) {
	if(ioctl(pInfo->fd, MSIOBUTTONS, &buttons) == 0) {
	    pInfo->conf_idev->commonOptions =
	      xf86ReplaceIntOption(pInfo->conf_idev->commonOptions, 
		"Buttons", buttons);
	    xf86Msg(X_INFO, "%s: Setting Buttons option to \"%d\"\n",
	      pInfo->name, buttons);
	}
    }

    if (pVuidMse->strmod && 
      (ioctl(pInfo->fd, I_POP, pVuidMse->strmod) == -1)) {
	xf86Msg(X_WARNING,
	  "%s: cannot pop module '%s' off mouse device: %s\n",
	  pInfo->name, pVuidMse->strmod, strerror(errno));
    }

    xf86CloseSerial(pInfo->fd);
    pInfo->fd = -1;

    /* Private structure */
    pMse->mousePriv = pVuidMse;

    /* Process common mouse options (like Emulate3Buttons, etc). */
    pMse->CommonOptions(pInfo);

    /* Setup the local procs. */
    pInfo->device_control = vuidMouseProc;
    pInfo->read_input = vuidReadInput;

    pInfo->flags |= XI86_CONFIGURED;
    return TRUE;
}

static void
vuidReadInput(InputInfoPtr pInfo)
{
    MouseDevPtr pMse;
    VuidMsePtr pVuidMse;
    int buttons;
    int dx = 0, dy = 0, dz = 0, dw = 0;
    unsigned int n;
    int c; 
    unsigned char *pBuf;
    int wmask;

    pMse = pInfo->private;
    pVuidMse = pMse->mousePriv;
    buttons = pMse->lastButtons;
    XisbBlockDuration(pMse->buffer, -1);
    pBuf = pVuidMse->buffer;
    n = 0;

    do {
	while (n < sizeof(Firm_event) && (c = XisbRead(pMse->buffer)) >= 0) {
	    pBuf[n++] = (unsigned char)c;
	}

	if (n == 0)
	    return;

	if (n != sizeof(Firm_event)) {
	    xf86Msg(X_WARNING, "%s: incomplete packet, size %d\n",
			pInfo->name, n);
	}

	if (pVuidMse->event.id >= BUT_FIRST && pVuidMse->event.id <= BUT_LAST) {
	    /* button */
	    int butnum = pVuidMse->event.id - BUT_FIRST;
	    if (butnum < 3)
		butnum = 2 - butnum;
	    if (!pVuidMse->event.value)
		buttons &= ~(1 << butnum);
	    else
		buttons |= (1 << butnum);
	} else if (pVuidMse->event.id >= VLOC_FIRST &&
		   pVuidMse->event.id <= VLOC_LAST) {
	    /* axis */
	    int delta = pVuidMse->event.value;
	    switch(pVuidMse->event.id) {
	    case LOC_X_DELTA:
		dx += delta;
		break;
	    case LOC_Y_DELTA:
		dy -= delta;
		break;
	    }
	} 
#ifdef HAVE_VUID_WHEEL
	else if (vuid_in_range(VUID_WHEEL, pVuidMse->event.id)) {
	    if (vuid_id_offset(pVuidMse->event.id) == 0)
		dz -= VUID_WHEEL_GETDELTA(pVuidMse->event.value);
	    else
		dw -= VUID_WHEEL_GETDELTA(pVuidMse->event.value);
	}
#endif

	n = 0;
	if ((c = XisbRead(pMse->buffer)) >= 0) {
	    /* Another packet.  Handle it right away. */
	    pBuf[n++] = c;
	}
    } while (n != 0);

    pMse->PostEvent(pInfo, buttons, dx, dy, dz, dw);
    return;
}

#define NUMEVENTS 64

static int
vuidMouseProc(DeviceIntPtr pPointer, int what)
{
    InputInfoPtr pInfo;
    MouseDevPtr pMse;
    VuidMsePtr pVuidMse;
    unsigned char map[MSE_MAXBUTTONS + 1];
    int nbuttons;

    pInfo = pPointer->public.devicePrivate;
    pMse = pInfo->private;
    pMse->device = pPointer;
    pVuidMse = pMse->mousePriv;

    switch (what) {
    case DEVICE_INIT: 
	pPointer->public.on = FALSE;

	for (nbuttons = 0; nbuttons < MSE_MAXBUTTONS; ++nbuttons)
	    map[nbuttons + 1] = nbuttons + 1;

	InitPointerDeviceStruct((DevicePtr)pPointer, 
				map, 
				min(pMse->buttons, MSE_MAXBUTTONS),
				miPointerGetMotionEvents, 
				pMse->Ctrl,
				miPointerGetMotionBufferSize());

	/* X valuator */
	xf86InitValuatorAxisStruct(pPointer, 0, 0, -1, 1, 0, 1);
	xf86InitValuatorDefaults(pPointer, 0);
	/* Y valuator */
	xf86InitValuatorAxisStruct(pPointer, 1, 0, -1, 1, 0, 1);
	xf86InitValuatorDefaults(pPointer, 1);
	xf86MotionHistoryAllocate(pInfo);
	break;

    case DEVICE_ON:
	pInfo->fd = xf86OpenSerial(pInfo->options);
	if (pInfo->fd == -1)
	    xf86Msg(X_WARNING, "%s: cannot open input device\n", pInfo->name);
	else {
	    pMse->buffer = XisbNew(pInfo->fd,
			      NUMEVENTS * sizeof(Firm_event));
	    if (!pMse->buffer) {
		xfree(pMse);
		xf86CloseSerial(pInfo->fd);
		pInfo->fd = -1;
	    } else {
	        int fmt = VUID_FIRM_EVENT;

		if (pVuidMse->strmod && 
		    (ioctl(pInfo->fd, I_PUSH, pVuidMse->strmod) == -1)) {
		    xf86Msg(X_ERROR,
		      "%s: cannot push module '%s' onto mouse device: %s\n",
		      pInfo->name, pVuidMse->strmod, strerror(errno));
		    xf86CloseSerial(pInfo->fd);
		    pInfo->fd = -1;
		} else {
		    ioctl(pInfo->fd, VUIDSFORMAT, &fmt);
		    vuidMouseWheelInit(pInfo);
		    xf86FlushInput(pInfo->fd);
		    AddEnabledDevice(pInfo->fd);
		}
	    }
	}
	pMse->lastButtons = 0;
	pMse->emulateState = 0;
	pPointer->public.on = TRUE;
	break;

    case DEVICE_OFF:
    case DEVICE_CLOSE:
	if (pInfo->fd != -1) {
	    RemoveEnabledDevice(pInfo->fd);
	    if (pMse->buffer) {
		XisbFree(pMse->buffer);
		pMse->buffer = NULL;
	    }
	    if (pVuidMse->strmod && 
		(ioctl(pInfo->fd, I_POP, pVuidMse->strmod) == -1)) {
		xf86Msg(X_WARNING,
		      "%s: cannot pop module '%s' off mouse device: %s\n",
		      pInfo->name, pVuidMse->strmod, strerror(errno));
	    }
	    xf86CloseSerial(pInfo->fd);
	    pInfo->fd = -1;
	}
	pPointer->public.on = FALSE;
	usleep(300000);
	break;
    }
    return Success;
}

static Bool
sunMousePreInit(InputInfoPtr pInfo, const char *protocol, int flags)
{
    /* The protocol is guaranteed to be one of the internalNames[] */
    if (xf86NameCmp(protocol, "VUID") == 0) {
	return vuidPreInit(pInfo, protocol, flags);
    }
    return TRUE;
}    

static const char **
BuiltinNames(void)
{
    return internalNames;
}

static Bool
CheckProtocol(const char *protocol)
{
    int i;

    for (i = 0; internalNames[i]; i++)
	if (xf86NameCmp(protocol, internalNames[i]) == 0)
	    return TRUE;

    return FALSE;
}

static const char *
DefaultProtocol(void)
{
    return "Auto";
}

static Bool
solarisMouseAutoProbe(InputInfoPtr pInfo, const char **protocol, 
	const char **device)
{
    const char **pdev, **pproto, *dev = NULL;
    int fd = -1;
    Bool found;

    for (pdev = solarisMouseDevs; *pdev; pdev += 2) {
	pproto = pdev + 1;
	if ((*protocol != NULL) && (strcmp(*protocol, "Auto") != 0) &&
	  (*pproto != NULL) && (strcmp(*pproto, *protocol) != 0)) {
	    continue;
	}
	if ((*device != NULL) && (strcmp(*device, *pdev) != 0)) {
	    continue;
	}
        SYSCALL (fd = open(*pdev, O_RDWR | O_NONBLOCK));
	if (fd == -1) {
#ifdef DEBUG
	    ErrorF("Cannot open %s (%s)\n", pdev, strerror(errno));
#endif
	} else {
	    found = TRUE;
	    if ((*pproto != NULL) && (strcmp(*pproto, "VUID") == 0)) {
		int i;
		if (ioctl(fd, VUIDGFORMAT, &i) < 0) {
		    found = FALSE;
		}
	    }
	    close(fd);
	    if (found == TRUE) {
		if (*pproto != NULL) {
		    *protocol = *pproto;
		}
		*device = *pdev;
		return TRUE;
	    }
	}
    }
    return FALSE;
}

static const char *
SetupAuto(InputInfoPtr pInfo, int *protoPara)
{
    const char *pdev = NULL;
    const char *pproto = NULL;
    MouseDevPtr pMse = pInfo->private;

    if (pInfo->fd == -1) {
	/* probe to find device/protocol to use */
	if (solarisMouseAutoProbe(pInfo, &pproto, &pdev) != FALSE) {
	    /* Set the Device option. */
	    pInfo->conf_idev->commonOptions =
	     xf86AddNewOption(pInfo->conf_idev->commonOptions, "Device", pdev);
	    xf86Msg(X_INFO, "%s: Setting Device option to \"%s\"\n",
	      pInfo->name, pdev);
	}
    } else if (pMse->protocolID == PROT_AUTO) {
	pdev = xf86CheckStrOption(pInfo->conf_idev->commonOptions, 
		"Device", NULL);
	solarisMouseAutoProbe(pInfo, &pproto, &pdev);
    }
    return pproto;
}

static const char *
FindDevice(InputInfoPtr pInfo, const char *protocol, int flags)
{
    const char *pdev = NULL;
    const char *pproto = protocol;

    if (solarisMouseAutoProbe(pInfo, &pproto, &pdev) != FALSE) {
	/* Set the Device option. */
	pInfo->conf_idev->commonOptions =
	  xf86AddNewOption(pInfo->conf_idev->commonOptions, "Device", pdev);
	xf86Msg(X_INFO, "%s: Setting Device option to \"%s\"\n",
	  pInfo->name, pdev);
    }
    return pdev;
}

#else /* __SOL8__ || !__i386 */

#undef MSE_MISC
#define MSE_MISC 0

#endif /* !__SOL8__ && __i386 */

static int
SupportedInterfaces(void)
{
    /* XXX This needs to be checked. */
    return MSE_SERIAL | MSE_BUS | MSE_PS2 | MSE_AUTO | MSE_XPS2 | MSE_MISC;
}

OSMouseInfoPtr
xf86OSMouseInit(int flags)
{
    OSMouseInfoPtr p;

    p = xcalloc(sizeof(OSMouseInfoRec), 1);
    if (!p)
	return NULL;
    p->SupportedInterfaces = SupportedInterfaces;
#if defined(__SOL8__) || !defined(__i386)
    p->BuiltinNames = BuiltinNames;
    p->CheckProtocol = CheckProtocol;
    p->PreInit = sunMousePreInit;
    p->DefaultProtocol = DefaultProtocol;
    p->SetupAuto = SetupAuto;
    p->FindDevice = FindDevice;
#endif
    return p;
}

