/* $XFree86: xc/programs/Xserver/Xext/dpmsproc.h,v 1.4 2003/07/16 01:38:29 dawes Exp $ */

/* Prototypes for functions that the DDX must provide */

#ifndef _DPMSPROC_H_
#define _DPMSPROC_H_

void DPMSSet(int level);
int  DPMSGet(int *plevel);
Bool DPMSSupported(void);

#endif
