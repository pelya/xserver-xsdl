/* Copyright 2006 by Peter Hutterer <peter@cs.unisa.edu.au> */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include <X11/Xproto.h>
#include "inputstr.h"

#include <X11/extensions/MPX.h>
#include <X11/extensions/MPXproto.h>

#include "mpxextinit.h"
#include "mpxglobals.h"

#include "listdev.h"

/***********************************************************************
 *
 * This procedure lists the MPX devices available to the server.
 *
 */
int SProcMPXListDevices(register ClientPtr client)
{
    register char n;

    REQUEST(xMPXListDevicesReq);
    swaps(&stuff->length, n);
    return (ProcMPXListDevices(client));
}

/***********************************************************************
 *
 * This procedure lists the MPX devices available to the server.
 *
 * Strongly based on ProcXListInputDevices
 */
int ProcMPXListDevices(register ClientPtr client)
{
    xMPXListDevicesReply rep;
    int numdevs = 0;
    int namesize = 1;	/* need 1 extra byte for strcpy */
    int size = 0;
    int total_length; 
    char* devbuf;
    char* namebuf;
    char *savbuf;
    xMPXDeviceInfoPtr dev;
    DeviceIntPtr d;

    REQUEST_SIZE_MATCH(xMPXListDevicesReq);
    memset(&rep, 0, sizeof(xMPXListDevicesReply));
    rep.repType = X_Reply;
    rep.RepType = X_MPXListDevices;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;

    for (d = inputInfo.devices; d; d = d->next) {
        if (d->isMPDev)
        {
            SizeMPXDeviceInfo(d, &namesize, &size);
            numdevs++;
        }
    }

    for (d = inputInfo.off_devices; d; d = d->next) {
        if (d->isMPDev)
        {
            SizeMPXDeviceInfo(d, &namesize, &size);
            numdevs++;
        }
    }

    total_length = numdevs * sizeof(xMPXDeviceInfo) + size + namesize;
    devbuf = (char *)xalloc(total_length);
    namebuf = devbuf + (numdevs * sizeof(xMPXDeviceInfo));
    savbuf = devbuf;

    dev = (xMPXDeviceInfoPtr) devbuf;
    for (d = inputInfo.devices; d; d = d->next, dev++)
        if (d->isMPDev)
            SetMPXDeviceInfo(client, d, dev, &devbuf, &namebuf);
    for (d = inputInfo.off_devices; d; d = d->next, dev++)
        if (d->isMPDev)
            SetMPXDeviceInfo(client, d, dev, &devbuf, &namebuf);

    rep.ndevices = numdevs;
    rep.length = (total_length + 3) >> 2;
    WriteReplyToClient(client, sizeof(xMPXListDevicesReply), &rep);
    WriteToClient(client, total_length, savbuf);
    xfree(savbuf);
    return Success;
}

/***********************************************************************
 *
 * This procedure calculates the size of the information to be returned
 * for an input device.
 *
 */

void
SizeMPXDeviceInfo(DeviceIntPtr d, int *namesize, int *size)
{
    *namesize += 1;
    if (d->name)
	*namesize += strlen(d->name);
}

/***********************************************************************
 *
 * This procedure sets information to be returned for an input device.
 *
 */

void
SetMPXDeviceInfo(ClientPtr client, DeviceIntPtr d, xMPXDeviceInfoPtr dev,
	       char **devbuf, char **namebuf)
{
    MPXCopyDeviceName(namebuf, d->name);
    MPXCopySwapDevice(client, d, devbuf);
}


/***********************************************************************
 *
 * This procedure copies data to the DeviceInfo struct, swapping if necessary.
 *
 * We need the extra byte in the allocated buffer, because the trailing null
 * hammers one extra byte, which is overwritten by the next name except for
 * the last name copied.
 *
 */

void
MPXCopyDeviceName(char **namebuf, char *name)
{
    char *nameptr = (char *)*namebuf;

    if (name) {
	*nameptr++ = strlen(name);
	strcpy(nameptr, name);
	*namebuf += (strlen(name) + 1);
    } else {
	*nameptr++ = 0;
	*namebuf += 1;
    }
}

/***********************************************************************
 *
 * This procedure copies data to the DeviceInfo struct, swapping if necessary.
 *
 */
void
MPXCopySwapDevice(register ClientPtr client, DeviceIntPtr d, char **buf)
{
    register char n;
    xMPXDeviceInfoPtr dev;

    dev = (xMPXDeviceInfoPtr) * buf;
    memset(dev, 0, sizeof(xMPXDeviceInfo));

    dev->id = d->id;
    dev->type = d->type;
    if (client->swapped) {
	swapl(&dev->type, n);	/* macro - braces are required */
    }
    *buf += sizeof(xMPXDeviceInfo);
}

/***********************************************************************
 *
 * This procedure writes the reply for the MPXListDevices function,
 * if the client and server have a different byte ordering.
 *
 */
void
SRepMPXListDevices(ClientPtr client, int size, xMPXListDevicesReply * rep)
{
    register char n;

    swaps(&rep->sequenceNumber, n);
    swapl(&rep->length, n);
    WriteToClient(client, size, (char *)rep);
}
