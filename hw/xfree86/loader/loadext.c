/*
 * Copyright (c) 2000 by The XFree86 Project, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

/* Maybe this file belongs elsewhere? */

#define LOADERDECLARATIONS
#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "loaderProcs.h"
#include "misc.h"
#include "xf86.h"

/*
 * This should be static, but miinitext wants it.  FIXME: make extension
 * initialization not completely terrible.
 */
ExtensionModule *ExtensionModuleList = NULL;
static int numExtensionModules = 0;

static ExtensionModule *
NewExtensionModule(void)
{
    ExtensionModule *save = ExtensionModuleList;
    int n;

    /* Make sure built-in extensions get added to the list before those
     * in modules. */
    AddStaticExtensions();

    /* Sanity check */
    if (!ExtensionModuleList)
        numExtensionModules = 0;

    n = numExtensionModules + 1;
    ExtensionModuleList = realloc(ExtensionModuleList,
                                  (n + 1) * sizeof(ExtensionModule));
    if (ExtensionModuleList == NULL) {
        ExtensionModuleList = save;
        return NULL;
    }
    else {
        numExtensionModules++;
        ExtensionModuleList[numExtensionModules].name = NULL;
        return ExtensionModuleList + (numExtensionModules - 1);
    }
}

void
LoadExtension(ExtensionModule * e, Bool builtin)
{
    ExtensionModule *newext;

    if (e == NULL || e->name == NULL)
        return;

    if (!(newext = NewExtensionModule()))
        return;

    if (builtin)
        xf86MsgVerb(X_INFO, 2, "Initializing built-in extension %s\n", e->name);
    else
        xf86MsgVerb(X_INFO, 2, "Loading extension %s\n", e->name);

    newext->name = e->name;
    newext->initFunc = e->initFunc;
    newext->disablePtr = e->disablePtr;
    newext->setupFunc = e->setupFunc;

    if (e->setupFunc != NULL)
        e->setupFunc();
}
