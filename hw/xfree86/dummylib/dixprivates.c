#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <X11/X.h>
#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"

/*
 * Utility functions required by libxf86_os. 
 */

pointer *
dixAllocatePrivate(PrivateRec **privates, const DevPrivateKey key)
{
    return NULL;	/* not used */
}

