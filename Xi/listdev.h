/* $XFree86: xc/programs/Xserver/Xi/listdev.h,v 3.1 1996/04/15 11:18:57 dawes Exp $ */
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

#ifndef LISTDEV_H
#define LISTDEV_H 1

int
SProcXListInputDevices(
#if NeedFunctionPrototypes
	ClientPtr              /* client */
#endif
	);

int
ProcXListInputDevices (
#if NeedFunctionPrototypes
	ClientPtr              /* client */
#endif
	);

void
SizeDeviceInfo (
#if NeedFunctionPrototypes
	DeviceIntPtr           /* d */,
	int *                  /* namesize */,
	int *                  /* size */
#endif
	);

void
ListDeviceInfo (
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	DeviceIntPtr           /* d */,
	xDeviceInfoPtr         /* dev */,
	char **                /* devbuf */,
	char **                /* classbuf */,
	char **                /* namebuf */
#endif
	);

void
CopyDeviceName (
#if NeedFunctionPrototypes
	char **                /* namebuf */,
	char *                 /* name */
#endif
	);

void
CopySwapDevice (
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	DeviceIntPtr           /* d */,
	int                    /* num_classes */,
	char **                /* buf */
#endif
	);

void
CopySwapKeyClass (
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	KeyClassPtr            /* k */,
	char **                /* buf */
#endif
	);

void
CopySwapButtonClass (
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	ButtonClassPtr         /* b */,
	char **                /* buf */
#endif
	);

int
CopySwapValuatorClass (
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	ValuatorClassPtr       /* v */,
	char **                /* buf */
#endif
	);

void
SRepXListInputDevices (
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	int                    /* size */,
	xListInputDevicesReply * /* rep */
#endif
	);

#endif /* LISTDEV_H */
