/* $XFree86: xc/programs/Xserver/hw/xfree86/ramdac/BTPriv.h,v 1.2 1998/07/25 16:57:18 dawes Exp $ */

#include "BT.h"

typedef struct {
	char *DeviceName;
} xf86BTramdacInfo;

extern xf86BTramdacInfo BTramdacDeviceInfo[];

#ifdef INIT_BT_RAMDAC_INFO
xf86BTramdacInfo BTramdacDeviceInfo[] = {
	{"AT&T 20C504"},
	{"AT&T 20C505"},
	{"BT485/484"}
};
#endif
