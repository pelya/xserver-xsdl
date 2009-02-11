/************************************************************

Copyright 1996 by Thomas E. Dickey <dickey@clark.net>

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of the above listed
copyright holder(s) not be used in advertising or publicity pertaining
to distribution of the software without specific, written prior
permission.

THE ABOVE LISTED COPYRIGHT HOLDER(S) DISCLAIM ALL WARRANTIES WITH REGARD
TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS, IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT HOLDER(S) BE
LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/

/********************************************************************
 * Interface of 'exevents.c'
 */

#ifndef EXEVENTS_H
#define EXEVENTS_H

#include <X11/extensions/XIproto.h>
#include "inputstr.h"

/**
 * Attached to the devPrivates of each client. Specifies the version number as
 * supported by the client.
 */
typedef struct _XIClientRec {
        int major_version;
        int minor_version;
} XIClientRec, *XIClientPtr;

extern _X_EXPORT void RegisterOtherDevice (
	DeviceIntPtr           /* device */);

extern _X_EXPORT int
UpdateDeviceState (
	DeviceIntPtr           /* device */,
	xEventPtr              /*  xE    */,
        int                    /* count  */);

extern _X_EXPORT void ProcessOtherEvent (
	xEventPtr /* FIXME deviceKeyButtonPointer * xE */,
	DeviceIntPtr           /* other */,
	int                    /* count */);

extern _X_EXPORT int InitProximityClassDeviceStruct(
	DeviceIntPtr           /* dev */);

extern _X_EXPORT void InitValuatorAxisStruct(
	DeviceIntPtr           /* dev */,
	int                    /* axnum */,
	int                    /* minval */,
	int                    /* maxval */,
	int                    /* resolution */,
	int                    /* min_res */,
	int                    /* max_res */);

extern _X_EXPORT void DeviceFocusEvent(
	DeviceIntPtr           /* dev */,
	int                    /* type */,
	int                    /* mode */,
	int                    /* detail */,
	WindowPtr              /* pWin */);

extern _X_EXPORT int GrabButton(
	ClientPtr              /* client */,
	DeviceIntPtr           /* dev */,
	BYTE                   /* this_device_mode */,
	BYTE                   /* other_devices_mode */,
	CARD16                 /* modifiers */,
	DeviceIntPtr           /* modifier_device */,
	CARD8                  /* button */,
	Window                 /* grabWindow */,
	BOOL                   /* ownerEvents */,
	Cursor                 /* rcursor */,
	Window                 /* rconfineTo */,
	Mask                   /* eventMask */);

extern _X_EXPORT int GrabKey(
	ClientPtr              /* client */,
	DeviceIntPtr           /* dev */,
	BYTE                   /* this_device_mode */,
	BYTE                   /* other_devices_mode */,
	CARD16                 /* modifiers */,
	DeviceIntPtr           /* modifier_device */,
	CARD8                  /* key */,
	Window                 /* grabWindow */,
	BOOL                   /* ownerEvents */,
	Mask                   /* mask */);

extern int SelectForWindow(
	DeviceIntPtr           /* dev */,
	WindowPtr              /* pWin */,
	ClientPtr              /* client */,
	Mask                   /* mask */,
	Mask                   /* exclusivemasks */);

extern _X_EXPORT int AddExtensionClient (
	WindowPtr              /* pWin */,
	ClientPtr              /* client */,
	Mask                   /* mask */,
	int                    /* mskidx */);

extern _X_EXPORT void RecalculateDeviceDeliverableEvents(
	WindowPtr              /* pWin */);

extern _X_EXPORT int InputClientGone(
	WindowPtr              /* pWin */,
	XID                    /* id */);

extern _X_EXPORT int SendEvent (
	ClientPtr              /* client */,
	DeviceIntPtr           /* d */,
	Window                 /* dest */,
	Bool                   /* propagate */,
	xEvent *               /* ev */,
	Mask                   /* mask */,
	int                    /* count */);

extern _X_EXPORT int SetButtonMapping (
	ClientPtr              /* client */,
	DeviceIntPtr           /* dev */,
	int                    /* nElts */,
	BYTE *                 /* map */);

extern _X_EXPORT int ChangeKeyMapping(
	ClientPtr              /* client */,
	DeviceIntPtr           /* dev */,
	unsigned               /* len */,
	int                    /* type */,
	KeyCode                /* firstKeyCode */,
	CARD8                  /* keyCodes */,
	CARD8                  /* keySymsPerKeyCode */,
	KeySym *               /* map */);

extern _X_EXPORT void DeleteWindowFromAnyExtEvents(
	WindowPtr              /* pWin */,
	Bool                   /* freeResources */);

extern _X_EXPORT int MaybeSendDeviceMotionNotifyHint (
	deviceKeyButtonPointer * /* pEvents */,
	Mask                   /* mask */);

extern _X_EXPORT void CheckDeviceGrabAndHintWindow (
	WindowPtr              /* pWin */,
	int                    /* type */,
	deviceKeyButtonPointer * /* xE */,
	GrabPtr                /* grab */,
	ClientPtr              /* client */,
	Mask                   /* deliveryMask */);

extern _X_EXPORT void MaybeStopDeviceHint(
	DeviceIntPtr           /* dev */,
	ClientPtr              /* client */);

extern _X_EXPORT int DeviceEventSuppressForWindow(
	WindowPtr              /* pWin */,
	ClientPtr              /* client */,
	Mask                   /* mask */,
	int                    /* maskndx */);

extern _X_EXPORT void SendEventToAllWindows(
        DeviceIntPtr           /* dev */,
        Mask                   /* mask */,
        xEvent *               /* ev */,
        int                    /* count */);

/* Input device properties */
extern _X_EXPORT void XIDeleteAllDeviceProperties(
        DeviceIntPtr            /* device */
);

extern _X_EXPORT int XIDeleteDeviceProperty(
        DeviceIntPtr            /* device */,
        Atom                    /* property */,
        Bool                    /* fromClient */
);

extern _X_EXPORT int XIChangeDeviceProperty(
        DeviceIntPtr            /* dev */,
        Atom                    /* property */,
        Atom                    /* type */,
        int                     /* format*/,
        int                     /* mode*/,
        unsigned long           /* len*/,
        pointer                 /* value*/,
        Bool                    /* sendevent*/
        );

extern _X_EXPORT int XIGetDeviceProperty(
        DeviceIntPtr            /* dev */,
        Atom                    /* property */,
        XIPropertyValuePtr*     /* value */
);

extern _X_EXPORT int XISetDevicePropertyDeletable(
        DeviceIntPtr            /* dev */,
        Atom                    /* property */,
        Bool                    /* deletable */
);

extern _X_EXPORT long XIRegisterPropertyHandler(
        DeviceIntPtr         dev,
        int (*SetProperty) (DeviceIntPtr dev,
                            Atom property,
                            XIPropertyValuePtr prop,
                            BOOL checkonly),
        int (*GetProperty) (DeviceIntPtr dev,
                            Atom property),
        int (*DeleteProperty) (DeviceIntPtr dev,
                               Atom property)
);

extern _X_EXPORT void XIUnregisterPropertyHandler(
        DeviceIntPtr          dev,
        long                  id
);

extern _X_EXPORT Atom XIGetKnownProperty(
        char*                 name
);

extern _X_EXPORT DeviceIntPtr XIGetDevice(xEvent *ev);

extern _X_EXPORT int XIPropToInt(
        XIPropertyValuePtr val,
        int *nelem_return,
        int **buf_return
);

extern _X_EXPORT int XIPropToFloat(
        XIPropertyValuePtr val,
        int *nelem_return,
        float **buf_return
);

/* For an event such as MappingNotify which affects client interpretation
 * of input events sent by device dev, should we notify the client, or
 * would it merely be irrelevant and confusing? */
extern _X_EXPORT int XIShouldNotify(ClientPtr client, DeviceIntPtr dev);

#endif /* EXEVENTS_H */
