/* $XFree86: xc/programs/Xserver/Xext/dpmsproc.h,v 1.3 2001/10/28 03:32:50 tsi Exp $ */

/* Prototypes for functions that the DDX must provide */

#ifndef _DPMSPROC_H_
#define _DPMSPROC_H_

void DPMSSet(int level);
int  DPMSGet(int *level);
Bool DPMSSupported(void);

#endif
