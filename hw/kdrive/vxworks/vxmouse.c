/*
 * Id: vxmouse.c,v 1.1 1999/11/24 08:35:24 keithp Exp $
 *
 * Copyright © 1999 Network Computing Devices, Inc.  All rights reserved.
 *
 * Author: Keith Packard
 */

#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "inputstr.h"
#include "scrnintstr.h"
#include "kdrive.h"
#include "Xpoll.h"
#include <event.h>
#include <smem.h>

static unsigned long	mouseState;

#define BUTTON1		0x01
#define BUTTON2		0x02
#define BUTTON3		0x04

#include <errno.h>

static int mouseFd = -1;

static eventqueue   *eventQueue;

void
VxMouseRead (int mousePort)
{
    Event	    ev;
    int		    dx, dy;
    unsigned long   flags;
    unsigned long   mask;
    int		    n;
    
    while (eventQueue->head != eventQueue->tail)
    {
	ev = *eventQueue->head;
	if (eventQueue->head >= &eventQueue->events[eventQueue->size-1])
	    eventQueue->head = &eventQueue->events[0];
	else
	    eventQueue->head++;
	switch (ev.e_type) {
	case E_BUTTON:
	    switch (ev.e_device) {
	    case E_MOUSE:
		switch (ev.e_key) {
		case BUTTON1:
		    mask = KD_BUTTON_1;
		    break;
		case BUTTON2:
		    mask = KD_BUTTON_2;
		    break;
		case BUTTON3:
		    mask = KD_BUTTON_3;
		    break;
		default:
		    mask = 0;
		    break;
		}
		if (ev.e_direction == E_KBUP)
		    mouseState &= ~mask;
		else
		    mouseState |= mask;
		KdEnqueueMouseEvent (mouseState | KD_MOUSE_DELTA, 0, 0);
		break;
	    case E_DKB:
		KdEnqueueKeyboardEvent (ev.e_key, ev.e_direction == E_KBUP);
		break;
	    }
	    break;
	case E_MMOTION:
	    KdEnqueueMouseEvent (mouseState | KD_MOUSE_DELTA,
				 ev.e_x, ev.e_y);
	    break;
	}
    }
}

int
VxMouseInit (void)
{
    int		    mousePort;
    unsigned long   ev_size;
    
    mouseState = 0;
    mousePort = open ("/dev/xdev", O_RDONLY, 0);
    if (mousePort < 0)
	ErrorF ("event port open failure %d\n", errno);
    mouseFd = open ("/dev/mouse", O_RDONLY, 0);
    if (mouseFd < 0)
	ErrorF ("mouse open failure %d\n", errno);
    if (eventQueue == 0)
    {
	ioctl (mousePort, EVENT_QUEUE_SMSIZE, &ev_size);
	eventQueue = (eventqueue *) smem_get ("event", ev_size, (SM_READ|SM_WRITE));
    }
    return mousePort;
}

void
VxMouseFini (int mousePort)
{
    if (mousePort >= 0)
	close (mousePort);
    if (mouseFd >= 0)
    {
	close (mouseFd);
	mouseFd = -1;
    }
}

KdMouseFuncs VxWorksMouseFuncs = {
    VxMouseInit,
    VxMouseRead,
    VxMouseFini
};
