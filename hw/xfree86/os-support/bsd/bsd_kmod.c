/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/bsd/bsd_kmod.c,v 3.2 2002/11/29 17:47:24 tsi Exp $ */

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/linker.h>

#include "xf86_OSproc.h"
#include "xf86_ansic.h"

/*
 * Load a FreeBSD kernel module.
 * This is used by the DRI/DRM to load a DRM kernel module when
 * the X server starts.  It could be used for other purposes in the future.
 * Input:
 *    modName - name of the kernel module (Ex: "tdfx")
 * Return:
 *    0 for failure, 1 for success
 */
int xf86LoadKernelModule(const char *modName)
{
    if (kldload(modName) != -1)
	return 1;
    else
	return 0;
}
