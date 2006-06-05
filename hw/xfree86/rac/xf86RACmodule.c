#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "xf86Module.h"

static XF86ModuleVersionInfo racVersRec =
{
    "rac",
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XORG_VERSION_CURRENT,
    1, 0, 0,
    ABI_CLASS_VIDEODRV,		/* requires the video driver ABI */
    ABI_VIDEODRV_VERSION,
    MOD_CLASS_NONE,
    {0,0,0,0}
};

_X_EXPORT XF86ModuleData racModuleData = { &racVersRec, NULL, NULL };
