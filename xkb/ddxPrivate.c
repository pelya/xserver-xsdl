/* $XFree86: xc/programs/Xserver/xkb/ddxPrivate.c,v 1.3 2003/11/17 22:20:45 dawes Exp $ */

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
XkbDDXPrivate(DeviceIntPtr dev,KeyCode key,XkbAction *act)
{
#ifdef XF86DDXACTIONS
    XkbAnyAction *xf86act = &(act->any);
    char msgbuf[XkbAnyActionDataSize+1];

    if (xf86act->type == XkbSA_XFree86Private) {
	memcpy(msgbuf, xf86act->data, XkbAnyActionDataSize);
	msgbuf[XkbAnyActionDataSize]= '\0';
	if (_XkbStrCaseCmp(msgbuf, "-vmode")==0)
	    xf86ProcessActionEvent(ACTION_PREV_MODE, NULL);
	else if (_XkbStrCaseCmp(msgbuf, "+vmode")==0)
	    xf86ProcessActionEvent(ACTION_NEXT_MODE, NULL);
	else if (_XkbStrCaseCmp(msgbuf, "ungrab")==0)
	    xf86ProcessActionEvent(ACTION_DISABLEGRAB, NULL);
	else if (_XkbStrCaseCmp(msgbuf, "clsgrb")==0)
	    xf86ProcessActionEvent(ACTION_CLOSECLIENT, NULL);
	else
	    xf86ProcessActionEvent(ACTION_MESSAGE, (void *) msgbuf);
    }
#endif
    return 0;
}

