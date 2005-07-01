/* $XdotOrg: xc/programs/Xserver/hw/xfree86/common/xorgHelper.c,v 1.3 2005/04/20 12:25:21 daniels Exp $ */

#include <X11/X.h>
#include <X11/os.h>
#include "servermd.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "propertyst.h"
#include "gcstruct.h"
#include "loaderProcs.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xorgVersion.h"


CARD32
xorgGetVersion()
{
    return XORG_VERSION_CURRENT;
}
