/* $XFree86$ */

#include "xvmcext.h"

extern int (*XvGetScreenIndexProc)(void);
extern unsigned long (*XvGetRTPortProc)(void);
extern int (*XvScreenInitProc)(ScreenPtr);
extern int (*XvMCScreenInitProc)(ScreenPtr, int, XvMCAdaptorPtr);

extern void XvRegister(void);
