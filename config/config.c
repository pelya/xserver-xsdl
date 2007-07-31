/*
 * Copyright © 2006-2007 Daniel Stone
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the copyright holders and/or authors
 * not be used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  The copyright holders
 * and/or authors make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * THE COPYRIGHT HOLDERS AND/OR AUTHORS DISCLAIM ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS AND/OR AUTHORS BE LIABLE
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "hotplug.h"
#include "config-backends.h"

void
config_init()
{
#if defined(CONFIG_DBUS_API) || defined(CONFIG_HAL)
    if (config_dbus_core_init()) {
# ifdef CONFIG_DBUS_API
       if (!config_dbus_init())
	    ErrorF("[config] failed to initialise D-Bus API\n");
# endif
# ifdef CONFIG_HAL
        if (!config_hal_init())
            ErrorF("[config] failed to initialise HAL\n");
# endif
    }
    else {
	ErrorF("[config] failed to initialise D-Bus core\n");
    }
#endif
}

void
config_fini()
{
#if defined(CONFIG_DBUS_API) || defined(CONFIG_HAL)
# ifdef CONFIG_HAL
    config_hal_fini();
# endif
# ifdef CONFIG_DBUS_API
    config_dbus_fini();
# endif
    config_dbus_core_fini();
#endif
}
