/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/sunos/sun_mouse.c,v 1.4 2002/01/25 21:56:21 tsi Exp $ */
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

#include "xf86.h"
#include "xf86_OSlib.h"
#include "xf86OSmouse.h"

#if defined(__SOL8__) || !defined(i386)

#include "xisb.h"
#include "mipointer.h"
#include <sys/vuid_event.h>

/* Names of protocols that are handled internally here. */

static const char *internalNames[] = {
	"VUID",
	NULL
};

typedef struct _VuidMseRec {
    Firm_event event;
    unsigned char *buffer;
} VuidMseRec, *VuidMsePtr;


static int  vuidMouseProc(DeviceIntPtr pPointer, int what);
static void vuidReadInput(InputInfoPtr pInfo);

/* This function is called when the protocol is "VUID". */
static Bool
vuidPreInit(InputInfoPtr pInfo, const char *protocol, int flags)
{
    MouseDevPtr pMse = pInfo->private;
    VuidMsePtr pVuidMse;

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
		ioctl(pInfo->fd, VUIDSFORMAT, &fmt);
		xf86FlushInput(pInfo->fd);
		AddEnabledDevice(pInfo->fd);
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
    return "VUID";
}

static const char *
SetupAuto(InputInfoPtr pInfo, int *protoPara)
{
    return DefaultProtocol();
}

#else /* __SOL8__ || !i386 */

#undef MSE_MISC
#define MSE_MISC 0

#endif /* !__SOL8__ && i386 */

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
#if defined(__SOL8__) || !defined(i386)
    p->BuiltinNames = BuiltinNames;
    p->CheckProtocol = CheckProtocol;
    p->PreInit = sunMousePreInit;
    p->DefaultProtocol = DefaultProtocol;
    p->SetupAuto = SetupAuto;
#endif
    return p;
}

