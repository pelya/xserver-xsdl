/* $XFree86: xc/programs/Xserver/hw/xfree86/scanpci/xf86PciData.h,v 1.2 2002/07/15 20:46:02 dawes Exp $ */



#ifndef PCI_DATA_H_
#define PCI_DATA_H_

#define NOVENDOR 0xFFFF
#define NODEVICE 0xFFFF
#define NOSUBSYS 0xFFFF

typedef Bool (*ScanPciSetupProcPtr)(void);
typedef void (*ScanPciCloseProcPtr)(void);
typedef int (*ScanPciFindByDeviceProcPtr)(
			unsigned short vendor, unsigned short device,
			unsigned short svendor, unsigned short subsys,
			const char **vname, const char **dname,
			const char **svname, const char **sname);
typedef int (*ScanPciFindBySubsysProcPtr)(
			unsigned short svendor, unsigned short subsys,
			const char **svname, const char **sname);
typedef CARD32 (*ScanPciFindClassBySubsysProcPtr)(
			unsigned short vendor, unsigned short subsystem);
typedef CARD32 (*ScanPciFindClassByDeviceProcPtr)(
			unsigned short vendor, unsigned short device);

/*
 * Whoever loads this module needs to define these and initialise them
 * after loading.
 */
extern ScanPciSetupProcPtr xf86SetupPciIds;
extern ScanPciCloseProcPtr xf86ClosePciIds;
extern ScanPciFindByDeviceProcPtr xf86FindPciNamesByDevice;
extern ScanPciFindBySubsysProcPtr xf86FindPciNamesBySubsys;
extern ScanPciFindClassBySubsysProcPtr xf86FindPciClassBySubsys;
extern ScanPciFindClassByDeviceProcPtr xf86FindPciClassByDevice;

Bool ScanPciSetupPciIds(void);
void ScanPciClosePciIds(void);
int ScanPciFindPciNamesByDevice(unsigned short vendor, unsigned short device,
				unsigned short svendor, unsigned short subsys,
				const char **vname, const char **dname,
				const char **svname, const char **sname);
int ScanPciFindPciNamesBySubsys(unsigned short svendor, unsigned short subsys,
				const char **svname, const char **sname);
CARD32 ScanPciFindPciClassBySubsys(unsigned short vendor,
				   unsigned short subsystem);
CARD32 ScanPciFindPciClassByDevice(unsigned short vendor,
				   unsigned short device);

#endif
