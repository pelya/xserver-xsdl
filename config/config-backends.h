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

#ifdef HAVE_DBUS
#include <dbus/dbus.h>

typedef void (*config_dbus_core_connect_hook)(DBusConnection *connection,
                                              void *data);
typedef void (*config_dbus_core_disconnect_hook)(void *data);

struct config_dbus_core_hook {
    config_dbus_core_connect_hook connect;
    config_dbus_core_disconnect_hook disconnect;
    void *data;

    struct config_dbus_core_hook *next;
};

int config_dbus_core_init(void);
void config_dbus_core_fini(void);
int config_dbus_core_add_hook(struct config_dbus_core_hook *hook);
void config_dbus_core_remove_hook(struct config_dbus_core_hook *hook);
#endif

#ifdef CONFIG_DBUS_API
int config_dbus_init(void);
void config_dbus_fini(void);
#endif

#ifdef CONFIG_HAL
int config_hal_init(void);
void config_hal_fini(void);
#endif
