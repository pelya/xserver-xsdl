/*
 * Copyright © 2006 Daniel Stone
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
#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>
#include <string.h>

#include <X11/X.h>

#include "opaque.h" /* for 'display': there has to be a better way */
                    /* the above comment lies.  there is no better way. */
#include "input.h"
#include "inputstr.h"
#include "config.h"
#include "os.h"

#define MATCH_RULE "type='method_call',interface='org.x.config.input'"

#define MALFORMED_MSG "[config] malformed message, dropping"
#define MALFORMED_MESSAGE() { DebugF(MALFORMED_MSG "\n"); \
                            ret = BadValue; \
                            goto unwind; }
#define MALFORMED_MESSAGE_ERROR() { DebugF(MALFORMED_MSG ": %s, %s", \
                                       error->name, error->message); \
                                  ret = BadValue; \
                                  goto unwind; }

static DBusConnection *configConnection = NULL;
static int configfd = -1;
static char busobject[32] = { 0 };
static char busname[64] = { 0 };

void
configDispatch()
{
    if (!configConnection)
        return;

    dbus_connection_read_write_dispatch(configConnection, 0);
}

static int
configAddDevice(DBusMessage *message, DBusMessageIter *iter, DBusError *error)
{
    DBusMessageIter subiter;
    InputOption *tmpo = NULL, *options = NULL;
    char *tmp = NULL;
    int ret = BadMatch;

    DebugF("[config] adding device\n");

    /* signature should be [ss][ss]... */
    options = (InputOption *) xcalloc(sizeof(InputOption), 1);
    if (!options) {
        ErrorF("[config] couldn't allocate option\n");
        return BadAlloc;
    }

    options->key = xstrdup("_source");
    options->value = xstrdup("client/dbus");

    while (dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_ARRAY) {
        tmpo = (InputOption *) xcalloc(sizeof(InputOption), 1);
        if (!tmpo) {
            ErrorF("[config] couldn't allocate option\n");
            ret = BadAlloc;
            goto unwind;
        }

        dbus_message_iter_recurse(iter, &subiter);

        if (dbus_message_iter_get_arg_type(&subiter) != DBUS_TYPE_STRING)
            MALFORMED_MESSAGE();

        dbus_message_iter_get_basic(&subiter, &tmp);
        if (!tmp)
            MALFORMED_MESSAGE();
        if (tmp[0] == '_') {
            ErrorF("[config] attempted subterfuge: option name %s given\n",
                   tmp);
            MALFORMED_MESSAGE();
        }
        tmpo->key = xstrdup(tmp);
        if (!tmpo->key) {
            ErrorF("[config] couldn't duplicate key!\n");
            ret = BadAlloc;
            goto unwind;
        }

        if (!dbus_message_iter_has_next(&subiter))
            MALFORMED_MESSAGE();
        dbus_message_iter_next(&subiter);
        if (dbus_message_iter_get_arg_type(&subiter) != DBUS_TYPE_STRING)
            MALFORMED_MESSAGE();

        dbus_message_iter_get_basic(&subiter, &tmp);
        if (!tmp)
            MALFORMED_MESSAGE();
        tmpo->value = xstrdup(tmp);
        if (!tmpo->value) {
            ErrorF("[config] couldn't duplicate option!\n");
            ret = BadAlloc;
            goto unwind;
        }

        tmpo->next = options;
        options = tmpo;
        dbus_message_iter_next(iter);
    }

    ret = NewInputDeviceRequest(options);
    if (ret != Success)
        DebugF("[config] NewInputDeviceRequest failed\n");

    return ret;

unwind:
    if (tmpo->key)
        xfree(tmpo->key);
    if (tmpo->value)
        xfree(tmpo->value);
    if (tmpo)
        xfree(tmpo);

    while (options) {
        tmpo = options;
        options = options->next;
        if (tmpo->key)
            xfree(tmpo->key);
        if (tmpo->value)
            xfree(tmpo->value);
        xfree(tmpo);
    }

    return ret;
}

static int
configRemoveDevice(DBusMessage *message, DBusMessageIter *iter,
                   DBusError *error)
{
    int deviceid = -1;
    int ret = BadMatch;
    DeviceIntPtr pDev = NULL;

    if (!dbus_message_get_args(message, error, DBUS_TYPE_INT32,
                               &deviceid, DBUS_TYPE_INVALID)) {
        MALFORMED_MESSAGE_ERROR();
    }

    if (deviceid < 0 || !(pDev = LookupDeviceIntRec(deviceid))) {
        DebugF("[config] bogus device id %d given\n", deviceid);
        ret = BadMatch;
        goto unwind;
    }

    DebugF("[config] removing device %s (id %d)\n", pDev->name, deviceid);

    /* Call PIE here so we don't try to dereference a device that's
     * already been removed. */
    OsBlockSignals();
    ProcessInputEvents();
    RemoveDevice(pDev);
    OsReleaseSignals();

    return Success;

unwind:
    return ret;
}

static DBusHandlerResult
configMessage(DBusConnection *connection, DBusMessage *message, void *closure)
{
    DBusMessageIter iter;
    DBusError error;
    DBusMessage *reply;
    DBusConnection *bus = closure;
    int ret = BadDrawable; /* nonsensical value */

    dbus_error_init(&error);

    if (strcmp(dbus_message_get_interface(message),
               "org.x.config.input") == 0) {
        if (!dbus_message_iter_init(message, &iter)) {
            ErrorF("[config] failed to init iterator\n");
            dbus_error_free(&error);
            return DBUS_HANDLER_RESULT_NEED_MEMORY; /* ?? */
        }

        if (strcmp(dbus_message_get_member(message), "add") == 0)
            ret = configAddDevice(message, &iter, &error);
        else if (strcmp(dbus_message_get_member(message), "remove") == 0)
            ret = configRemoveDevice(message, &iter, &error);
    }

    if (ret != BadDrawable && ret != BadAlloc) {
        reply = dbus_message_new_method_return(message);
        dbus_message_iter_init_append(reply, &iter);

        if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &ret)) {
            ErrorF("[config] couldn't append to iterator\n");
            dbus_error_free(&error);
            return DBUS_HANDLER_RESULT_HANDLED;
        }

        if (!dbus_connection_send(bus, reply, NULL))
            ErrorF("[config] failed to send reply\n");
        dbus_connection_flush(bus);

        dbus_message_unref(reply);
    }

    dbus_error_free(&error);

    if (ret == BadAlloc)
        return DBUS_HANDLER_RESULT_NEED_MEMORY;
    else if (ret == BadDrawable)
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    else
        return DBUS_HANDLER_RESULT_HANDLED;
}

void
configInitialise()
{
    DBusConnection *bus = NULL;
    DBusError error;
    DBusObjectPathVTable vtable;

    configConnection = NULL;

    dbus_error_init(&error);
    bus = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
    if (!bus || dbus_error_is_set(&error)) {
        dbus_error_free(&error);
        FatalError("[dbus] some kind of error occurred: %s (%s)\n", error.name,
                   error.message);
        return;
    }

    if (!dbus_connection_get_unix_fd(bus, &configfd)) {
        dbus_connection_close(bus);
        configfd = -1;
        FatalError("[dbus] couldn't get fd for bus\n");
        return;
    }

    snprintf(busname, sizeof(busname), "org.x.config.display%d", atoi(display));
    if (!dbus_bus_request_name(bus, busname, 0, &error) ||
        dbus_error_is_set(&error)) {
        dbus_error_free(&error);
        dbus_connection_close(bus);
        configfd = -1;
        FatalError("[dbus] couldn't take over org.x.config: %s (%s)\n",
                   error.name, error.message);
        return;
    }

    /* blocks until we get a reply. */
    dbus_bus_add_match(bus, MATCH_RULE, &error);
    if (dbus_error_is_set(&error)) {
        dbus_error_free(&error);
        dbus_bus_release_name(bus, busname, &error);
        dbus_connection_close(bus);
        configfd = -1;
        FatalError("[dbus] couldn't match X.Org rule: %s (%s)\n", error.name,
                   error.message);
        return;
    }

    vtable.message_function = configMessage;
    snprintf(busobject, sizeof(busobject), "/org/x/config/%d", atoi(display));
    if (!dbus_connection_register_object_path(bus, busobject, &vtable, bus)) {
        configfd = -1;
        dbus_bus_release_name(bus, busname, &error);
        dbus_bus_remove_match(bus, MATCH_RULE, &error);
        dbus_connection_close(bus);
        FatalError("[dbus] couldn't register object path\n");
        return;
    }

    DebugF("[dbus] registered object path %s\n", busobject);

    dbus_error_free(&error);
    configConnection = bus;
    AddGeneralSocket(configfd);
}

void
configFini()
{
    DBusError error;

    if (configConnection) {
        dbus_error_init(&error);
        /* This causes a segfault inside libdbus.  Sigh. */
#if 0
        dbus_connection_unregister_object_path(configConnection, busobject);
#endif
        dbus_bus_remove_match(configConnection, MATCH_RULE, &error);
        dbus_bus_release_name(configConnection, busname, &error);
        dbus_connection_unref(configConnection);
        RemoveGeneralSocket(configfd);
        configConnection = NULL;
        configfd = -1;
        dbus_error_free(&error);
    }
}

#else /* !HAVE_DBUS */

void
configDispatch()
{
}

void
configInitialise()
{
}

void
configFini()
{
}

#endif /* HAVE_DBUS */
