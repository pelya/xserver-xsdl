/*
 * Copyright (c) 1997 Matthieu Herrb
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Matthieu Herrb not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Matthieu Herrb makes no
 * representations about the suitability of this software for any purpose.
 *  It is provided "as is" without express or implied warranty.
 *
 * MATTHIEU HERRB DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL MATTHIEU HERRB BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "xf86Module.h"
#include "xf86Opt.h"

#include <X11/Xproto.h>

#include "extinit.h"
#include "xf86Extensions.h"
#include "globals.h"

static MODULESETUPPROTO(extmodSetup);

/*
 * Array describing extensions to be initialized
 */
static ExtensionModule extensionModules[] = {
};

static XF86ModuleVersionInfo VersRec = {
    "extmod",
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XORG_VERSION_CURRENT,
    1, 0, 0,
    ABI_CLASS_EXTENSION,
    ABI_EXTENSION_VERSION,
    MOD_CLASS_EXTENSION,
    {0, 0, 0, 0}
};

/*
 * Data for the loader
 */
_X_EXPORT XF86ModuleData extmodModuleData = { &VersRec, extmodSetup, NULL };

static pointer
extmodSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(extensionModules); i++)
	LoadExtension(&extensionModules[i], FALSE);

    /* Need a non-NULL return */
    return (pointer) 1;
}
