/* $XFree86: xc/programs/Xserver/xkb/ddxPrivate.c,v 1.1 2002/11/20 04:49:02 dawes Exp $ */

#include <stdio.h>
#define NEED_EVENTS 1
#include <X11/X.h>
#include "windowstr.h"
#define XKBSRV_NEED_FILE_FUNCS
#include "XKBsrv.h"

#ifdef XF86DDXACTIONS
#include "xf86.h"
#endif

int
#if NeedFunctionPrototypes
XkbDDXPrivate(DeviceIntPtr dev,KeyCode key,XkbAction *act)
#else
XkbDDXPrivate(dev,key,act)
    DeviceIntPtr  dev;
    KeyCode	  key;
    XkbAction	 *act;
#endif
{
#ifdef XF86DDXACTIONS
    XkbMessageAction *msgact = &(act->msg);
    char msgbuf[7];
    int x;

    if (msgact->type == XkbSA_XFree86Private) {
	msgbuf[0]= msgact->flags;
	for (x=0; x<5; x++)
	    msgbuf[x+1] = msgact->message[x];
	msgbuf[6]= '\0';
	if (_XkbStrCaseCmp(msgbuf, "-vmode")==0)
	    xf86ProcessActionEvent(ACTION_PREV_MODE, NULL);
	else if (_XkbStrCaseCmp(msgbuf, "+vmode")==0)
	    xf86ProcessActionEvent(ACTION_NEXT_MODE, NULL);
	else if (_XkbStrCaseCmp(msgbuf, "ungrab")==0)
	    xf86ProcessActionEvent(ACTION_DISABLEGRAB, NULL);
	else if (_XkbStrCaseCmp(msgbuf, "clsgrb")==0)
	    xf86ProcessActionEvent(ACTION_CLOSECLIENT, NULL);
    }
#endif
    return 0;
}

