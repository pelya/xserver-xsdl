/* $XFree86: xc/programs/Xserver/Xext/xvmodproc.h,v 1.2 2001/03/05 04:51:55 mvojkovi Exp $ */

#include "xvmcext.h"

extern int (*XvGetScreenIndexProc)(void);
extern unsigned long (*XvGetRTPortProc)(void);
extern int (*XvScreenInitProc)(ScreenPtr);
extern int (*XvMCScreenInitProc)(ScreenPtr, int, XvMCAdaptorPtr);

extern void XvRegister(void);
