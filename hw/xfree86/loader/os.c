/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/os.c,v 1.2 2002/05/31 18:46:00 dawes Exp $ */

#include "loaderProcs.h"

/*
 * OSNAME is a standard form of the OS name that may be used by the
 * loader and by OS-specific modules.
 */

#if defined(__linux__)
#define OSNAME "linux"
#elif defined(__FreeBSD__)
#define OSNAME "freebsd"
#elif defined(__NetBSD__)
#define OSNAME "netbsd"
#elif defined(__OpenBSD__)
#define OSNAME "openbsd"
#elif defined(Lynx)
#define OSNAME "lynxos"
#elif defined(__GNU__)
#define OSNAME "hurd"
#elif defined(SCO)
#define OSNAME "sco"
#elif defined(DGUX)
#define OSNAME "dgux"
#elif defined(ISC)
#define OSNAME "isc"
#elif defined(SVR4) && defined(sun)
#define OSNAME "solaris"
#elif defined(SVR4)
#define OSNAME "svr4"
#elif defined(__UNIXOS2__)
#define OSNAME "os2"
#else
#define OSNAME "unknown"
#endif


/* Return the OS name, and run-time OS version */

void
LoaderGetOS(const char **name, int *major, int *minor, int *teeny)
{
	if (name)
		*name = OSNAME;
		
	/* reporting runtime versions isn't supported yet */
}

