/* $XFree86: xc/programs/Xserver/hw/xfree86/ramdac/TIPriv.h,v 1.2 2000/03/21 21:15:28 alanh Exp $ */

#include "TI.h"

typedef struct {
	char *DeviceName;
} xf86TIramdacInfo;

extern xf86TIramdacInfo TIramdacDeviceInfo[];

#ifdef INIT_TI_RAMDAC_INFO
xf86TIramdacInfo TIramdacDeviceInfo[] = {
	{"TI TVP3030"},
	{"TI TVP3026"}
};
#endif

#define TISAVE(_reg) do { 						\
    ramdacReg->DacRegs[_reg] = (*ramdacPtr->ReadDAC)(pScrn, _reg);	\
} while (0)

#define TIRESTORE(_reg) do { 						\
    (*ramdacPtr->WriteDAC)(pScrn, _reg, 				\
	(ramdacReg->DacRegs[_reg] & 0xFF00) >> 8, 			\
	ramdacReg->DacRegs[_reg]);					\
} while (0)
