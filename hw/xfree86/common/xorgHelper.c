/* $XdotOrg: xc/programs/Xserver/hw/xfree86/common/xorgHelper.c,v 1.2 2004/04/23 19:20:32 eich Exp $ */

#include "X.h"
#include "os.h"
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
