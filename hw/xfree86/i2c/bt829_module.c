
#include "xf86Module.h"

static MODULESETUPPROTO(bt829Setup);


static XF86ModuleVersionInfo bt829VersRec =
{
        "bt829",
        MODULEVENDORSTRING,
        MODINFOSTRING1,
        MODINFOSTRING2,
        XORG_VERSION_CURRENT,
        1, 0, 0,
        ABI_CLASS_VIDEODRV,             /* This needs the video driver ABI */
        ABI_VIDEODRV_VERSION,
        MOD_CLASS_NONE,
        {0,0,0,0}
};
 
XF86ModuleData bt829ModuleData = { &bt829VersRec, bt829Setup, NULL }; 

static pointer
bt829Setup(pointer module, pointer opts, int *errmaj, int *errmin) {
   return (pointer)1;
}
