/*
 * Copyright © 2007 Daniel Stone
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author: Daniel Stone <daniel@fooishbar.org>
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <dbus/dbus.h>
#include <hal/libhal.h>
#include <string.h>
#include <sys/select.h>

#include "input.h"
#include "inputstr.h"
#include "hotplug.h"
#include "config-backends.h"
#include "os.h"

#define TYPE_NONE 0
#define TYPE_KEYS 1
#define TYPE_POINTER 2

struct config_hal_info {
    DBusConnection *system_bus;
    LibHalContext *hal_ctx;
};

static void
remove_device(DeviceIntPtr dev)
{
    DebugF("[config/hal] removing device %s\n", dev->name);

    /* Call PIE here so we don't try to dereference a device that's
     * already been removed. */
    OsBlockSignals();
    ProcessInputEvents();
    DeleteInputDeviceRequest(dev);
    OsReleaseSignals();
}

static void
device_removed(LibHalContext *ctx, const char *udi)
{
    DeviceIntPtr dev;
    char *value;

    value = xalloc(strlen(udi) + 5); /* "hal:" + NULL */
    if (!value)
        return;
    sprintf(value, "hal:%s", udi);

    for (dev = inputInfo.devices; dev; dev = dev->next) {
        if (dev->config_info && strcmp(dev->config_info, value) == 0)
            remove_device(dev);
    }
    for (dev = inputInfo.off_devices; dev; dev = dev->next) {
        if (dev->config_info && strcmp(dev->config_info, value) == 0)
            remove_device(dev);
    }

    xfree(value);
}

static void
add_option(InputOption **options, const char *key, const char *value)
{
    if (!value || *value == '\0')
        return;

    for (; *options; options = &(*options)->next)
        ;
    *options = xcalloc(sizeof(**options), 1);
    (*options)->key = xstrdup(key);
    (*options)->value = xstrdup(value);
    (*options)->next = NULL;
}

static char *
get_prop_string(LibHalContext *hal_ctx, const char *udi, const char *name)
{
    char *prop, *ret;

    prop = libhal_device_get_property_string(hal_ctx, udi, name, NULL);
    DebugF(" [config/hal] getting %s on %s returned %s\n", name, udi, prop);
    if (prop) {
        ret = xstrdup(prop);
        libhal_free_string(prop);
    }
    else {
        return NULL;
    }

    return ret;
}

static char *
get_prop_string_array(LibHalContext *hal_ctx, const char *udi, const char *prop)
{
    char **props, *ret, *str;
    int i, len = 0;

    props = libhal_device_get_property_strlist(hal_ctx, udi, prop, NULL);
    if (props) {
        for (i = 0; props[i]; i++)
            len += strlen(props[i]);

        ret = xcalloc(sizeof(char), len + i); /* i - 1 commas, 1 NULL */
        if (!ret) {
            libhal_free_string_array(props);
            return NULL;
        }

        str = ret;
        for (i = 0; props[i]; i++) {
            str = strcpy(str, props[i]);
            *str++ = ',';
        }
        *str = '\0';

        libhal_free_string_array(props);
    }
    else {
        return NULL;
    }

    return ret;
}

static void
device_added(LibHalContext *hal_ctx, const char *udi)
{
    char **props;
    char *path = NULL, *driver = NULL, *name = NULL, *xkb_rules = NULL;
    char *xkb_model = NULL, *xkb_layout = NULL, *xkb_variant = NULL;
    char *xkb_options = NULL, *config_info = NULL;
    InputOption *options = NULL;
    DeviceIntPtr dev;
    DBusError error;
    int type = TYPE_NONE;
    int i;

    dbus_error_init(&error);

    props = libhal_device_get_property_strlist(hal_ctx, udi,
                                               "info.capabilities", &error);
    if (!props) {
        DebugF("[config/hal] couldn't get capabilities for %s: %s (%s)\n",
               udi, error.name, error.message);
        goto out_error;
    }
    for (i = 0; props[i]; i++) {
        /* input.keys is the new, of which input.keyboard is a subset, but
         * input.keyboard is the old 'we have keys', so we have to keep it
         * around. */
        if (strcmp(props[i], "input.keys") == 0 ||
            strcmp(props[i], "input.keyboard") == 0)
            type |= TYPE_KEYS;
        if (strcmp(props[i], "input.mouse") == 0)
            type |= TYPE_POINTER;
    }
    libhal_free_string_array(props);

    if (type == TYPE_NONE)
        goto out_error;

    driver = get_prop_string(hal_ctx, udi, "input.x11_driver");
    path = get_prop_string(hal_ctx, udi, "input.device");
    if (!driver || !path) {
        DebugF("[config/hal] no driver or path specified for %s\n", udi);
        goto unwind;
    }
    name = get_prop_string(hal_ctx, udi, "info.product");
    if (!name)
        name = xstrdup("(unnamed)");

    if (type & TYPE_KEYS) {
        xkb_rules = get_prop_string(hal_ctx, udi, "input.xkb.rules");
        xkb_model = get_prop_string(hal_ctx, udi, "input.xkb.model");
        xkb_layout = get_prop_string(hal_ctx, udi, "input.xkb.layout");
        xkb_variant = get_prop_string(hal_ctx, udi, "input.xkb.variant");
        xkb_options = get_prop_string_array(hal_ctx, udi, "input.xkb.options");
    }

    options = xcalloc(sizeof(*options), 1);
    options->key = xstrdup("_source");
    options->value = xstrdup("server/hal");
    if (!options->key || !options->value) {
        ErrorF("[config] couldn't allocate first key/value pair\n");
        goto unwind;
    }

    add_option(&options, "path", path);
    add_option(&options, "driver", driver);
    add_option(&options, "name", name);
    config_info = xalloc(strlen(udi) + 5); /* "hal:" and NULL */
    if (!config_info)
        goto unwind;
    sprintf(config_info, "hal:%s", udi);

    if (xkb_model)
        add_option(&options, "xkb_model", xkb_model);
    if (xkb_layout)
        add_option(&options, "xkb_layout", xkb_layout);
    if (xkb_variant)
        add_option(&options, "xkb_variant", xkb_variant);
    if (xkb_options)
        add_option(&options, "xkb_options", xkb_options);

    if (NewInputDeviceRequest(options, &dev) != Success) {
        DebugF("[config/hal] NewInputDeviceRequest failed\n");
        goto unwind;
    }

    for (; dev; dev = dev->next)
        dev->config_info = xstrdup(config_info);

unwind:
    if (path)
        xfree(path);
    if (driver)
        xfree(driver);
    if (name)
        xfree(name);
    if (xkb_rules)
        xfree(xkb_rules);
    if (xkb_model)
        xfree(xkb_model);
    if (xkb_layout)
        xfree(xkb_layout);
    if (xkb_options)
        xfree(xkb_options);
    if (config_info)
        xfree(config_info);

out_error:
    dbus_error_free(&error);

    return;
}

static void
disconnect_hook(void *data)
{
    DBusError error;
    struct config_hal_info *info = data;

    if (info->hal_ctx) {
        dbus_error_init(&error);
        if (!libhal_ctx_shutdown(info->hal_ctx, &error))
            DebugF("[config/hal] couldn't shut down context: %s (%s)\n",
                   error.name, error.message);
        libhal_ctx_free(info->hal_ctx);
        dbus_error_free(&error);
    }

    info->hal_ctx = NULL;
    info->system_bus = NULL;
}

static void
connect_hook(DBusConnection *connection, void *data)
{
    DBusError error;
    struct config_hal_info *info = data;
    char **devices;
    int num_devices, i;

    info->system_bus = connection;

    dbus_error_init(&error);

    if (!info->hal_ctx)
        info->hal_ctx = libhal_ctx_new();
    if (!info->hal_ctx) {
        ErrorF("[config/hal] couldn't create HAL context\n");
        goto out_err;
    }

    if (!libhal_ctx_set_dbus_connection(info->hal_ctx, info->system_bus)) {
        ErrorF("[config/hal] couldn't associate HAL context with bus\n");
        goto out_ctx;
    }
    if (!libhal_ctx_init(info->hal_ctx, &error)) {
        ErrorF("[config/hal] couldn't initialise context: %s (%s)\n",
               error.name, error.message);
        goto out_ctx;
    }
    if (!libhal_device_property_watch_all(info->hal_ctx, &error)) {
        ErrorF("[config/hal] couldn't watch all properties: %s (%s)\n",
               error.name, error.message);
        goto out_ctx2;
    }
    libhal_ctx_set_device_added(info->hal_ctx, device_added);
    libhal_ctx_set_device_removed(info->hal_ctx, device_removed);

    devices = libhal_find_device_by_capability(info->hal_ctx, "input",
                                               &num_devices, &error);
    /* FIXME: Get default devices if error is set. */
    for (i = 0; i < num_devices; i++)
        device_added(info->hal_ctx, devices[i]);
    libhal_free_string_array(devices);

    dbus_error_free(&error);

    return;

out_ctx2:
    if (!libhal_ctx_shutdown(info->hal_ctx, &error))
        DebugF("[config/hal] couldn't shut down context: %s (%s)\n",
               error.name, error.message);
out_ctx:
    libhal_ctx_free(info->hal_ctx);
out_err:
    dbus_error_free(&error);

    info->hal_ctx = NULL;
    info->system_bus = NULL;

    return;
}

static struct config_hal_info hal_info;
static struct config_dbus_core_hook hook = {
    .connect = connect_hook,
    .disconnect = disconnect_hook,
    .data = &hal_info,
};

int
config_hal_init(void)
{
    memset(&hal_info, 0, sizeof(hal_info));
    hal_info.system_bus = NULL;
    hal_info.hal_ctx = NULL;

    if (!config_dbus_core_add_hook(&hook)) {
        ErrorF("[config/hal] failed to add D-Bus hook\n");
        return 0;
    }

    return 1;
}

void
config_hal_fini(void)
{
    config_dbus_core_remove_hook(&hook);
}
