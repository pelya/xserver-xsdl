/* $XFree86$ */

/* Prototypes for functions that the DDX must provide */

#ifndef _DPMSPROC_H_
#define _DPMSPROC_H_

void DPMSSet(int level);
int  DPMSGet(int *plevel);
Bool DPMSSupported(void);

#endif
