/* $XFree86: xc/programs/Xserver/Xext/xvmodproc.h,v 1.3 2001/04/08 16:33:22 dawes Exp $ */

#include "xvmcext.h"

extern int (*XvGetScreenIndexProc)(void);
extern unsigned long (*XvGetRTPortProc)(void);
extern int (*XvScreenInitProc)(ScreenPtr);
extern int (*XvMCScreenInitProc)(ScreenPtr, int, XvMCAdaptorPtr);

extern void XvRegister(void);
