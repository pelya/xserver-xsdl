/* $XFree86$ */

#include "xf86Module.h"

static XF86ModuleVersionInfo racVersRec =
{
    "rac",
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XF86_VERSION_CURRENT,
    1, 0, 0,
    ABI_CLASS_VIDEODRV,		/* requires the video driver ABI */
    ABI_VIDEODRV_VERSION,
    MOD_CLASS_NONE,
    {0,0,0,0}
};


XF86ModuleData racModuleData = { &racVersRec, NULL, NULL };

