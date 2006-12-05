/* Copyright 2006 by Peter Hutterer <peter@cs.unisa.edu.au> */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef LISTDEV_H
#define LISTDEV_H 1


#include <X11/X.h>
#include <X11/Xproto.h>
#include "inputstr.h"

#include <X11/extensions/MPX.h>
#include <X11/extensions/MPXproto.h>

#include "mpxextinit.h"
#include "mpxglobals.h"

int SProcMPXListDevices(ClientPtr	/* client */
    );

int ProcMPXListDevices(ClientPtr	/* client */
    );

void SizeMPXDeviceInfo(DeviceIntPtr /* d */ ,
		    int * /* namesize */ ,
		    int *	/* size */
    );

void SetMPXDeviceInfo(ClientPtr /* client */ ,
		    DeviceIntPtr /* d */ ,
		    xMPXDeviceInfoPtr /* dev */ ,
		    char ** /* devbuf */ ,
		    char **	/* namebuf */
    );


void MPXCopyDeviceName(char ** /* namebuf */ ,
		    char *	/* name */
    );

void MPXCopySwapDevice(ClientPtr /* client */ ,
		    DeviceIntPtr /* d */ ,
		    char **	/* buf */
    );

void SRepMPXListDevices(ClientPtr /* client */ ,
			   int /* size */ ,
			   xMPXListDevicesReply *	/* rep */
    );
#endif
