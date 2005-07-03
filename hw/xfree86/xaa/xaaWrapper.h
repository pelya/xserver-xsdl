#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#ifndef _XAA_WRAPPER_H
# define _XAA_WRAPPER_H

typedef void (*SyncFunc)(ScreenPtr);

Bool xaaSetupWrapper(ScreenPtr pScreen,
		     XAAInfoRecPtr infoPtr, int depth, SyncFunc *func);

#endif
