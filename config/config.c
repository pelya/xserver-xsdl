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
#include <sys/select.h>

#include <X11/X.h>

#include "opaque.h" /* for 'display': there has to be a better way */
                    /* the above comment lies.  there is no better way. */
#include "input.h"
#include "inputstr.h"
#include "hotplug.h"
#include "os.h"

#define CONFIG_MATCH_RULE "type='method_call',interface='org.x.config.input'"

#define MALFORMED_MSG "[config] malformed message, dropping"
#define MALFORMED_MESSAGE() { DebugF(MALFORMED_MSG "\n"); \
                            ret = BadValue; \
                            goto unwind; }
#define MALFORMED_MESSAGE_ERROR() { DebugF(MALFORMED_MSG ": %s, %s", \
                                       error->name, error->message); \
                                  ret = BadValue; \
                                  goto unwind; }

/* How often to attempt reconnecting when we get booted off the bus. */
#define RECONNECT_DELAY 10000 /* in ms */

struct config_data {
    int fd;
    DBusConnection *connection;
    char busobject[32];
    char busname[64];
};

static struct config_data *configData;

static CARD32 configReconnect(OsTimerPtr timer, CARD32 time, pointer arg);

static void
configWakeupHandler(pointer blockData, int err, pointer pReadMask)
{
    struct config_data *data = blockData;

    if (data->connection && FD_ISSET(data->fd, (fd_set *) pReadMask))
        dbus_connection_read_write_dispatch(data->connection, 0);
}

static void
configBlockHandler(pointer data, struct timeval **tv, pointer pReadMask)
{
}

static void
configTeardown(void)
{
    if (configData) {
        RemoveGeneralSocket(configData->fd);
        RemoveBlockAndWakeupHandlers(configBlockHandler, configWakeupHandler,
                                     configData);
        xfree(configData);
        configData = NULL;
    }
}

static int
configAddDevice(DBusMessage *message, DBusMessageIter *iter, 
                DBusMessage *reply, DBusMessageIter *r_iter,
                DBusError *error)
{
    DBusMessageIter subiter;
    InputOption *tmpo = NULL, *options = NULL;
    char *tmp = NULL;
    int ret = BadMatch;
    DeviceIntPtr dev = NULL;

    DebugF("[config] adding device\n");

    /* signature should be [ss][ss]... */
    options = (InputOption *) xcalloc(sizeof(InputOption), 1);
    if (!options) {
        ErrorF("[config] couldn't allocate option\n");
        return BadAlloc;
    }

    options->key = xstrdup("_source");
    options->value = xstrdup("client/dbus");
    if(!options->key || !options->value) {
        ErrorF("[config] couldn't allocate first key/value pair\n");
        ret = BadAlloc;
        goto unwind;
    }

    while (dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_ARRAY) {
        tmpo = (InputOption *) xcalloc(sizeof(InputOption), 1);
        if (!tmpo) {
            ErrorF("[config] couldn't allocate option\n");
            ret = BadAlloc;
            goto unwind;
        }
        tmpo->next = options;
        options = tmpo;

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
        options->key = xstrdup(tmp);
        if (!options->key) {
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
        options->value = xstrdup(tmp);
        if (!options->value) {
            ErrorF("[config] couldn't duplicate option!\n");
            ret = BadAlloc;
            goto unwind;
        }

        dbus_message_iter_next(iter);
    }

    ret = NewInputDeviceRequest(options, &dev);
    if (ret != Success) {
        DebugF("[config] NewInputDeviceRequest failed\n");
        goto unwind;
    }

    if (!dev) {
        DebugF("[config] NewInputDeviceRequest succeeded, without device\n"); 
        ret = BadMatch;
        goto unwind;
    }

    if (!dbus_message_iter_append_basic(r_iter, DBUS_TYPE_INT32, &(dev->id))) {
        ErrorF("[config] couldn't append to iterator\n");
        ret = BadAlloc;
        goto unwind;
    }

unwind:
    if (dev && ret != Success)
        RemoveDevice(dev);

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
    DeleteInputDeviceRequest(pDev);
    OsReleaseSignals();

    return Success;

unwind:
    return ret;
}

static int
configListDevices(DBusMessage *message, DBusMessageIter *iter,
                   DBusMessage *reply, DBusMessageIter *r_iter,
                   DBusError *error)
{
    DeviceIntPtr d;
    int ret = BadMatch;

    for (d = inputInfo.devices; d; d = d->next) {
        if (!dbus_message_iter_append_basic(r_iter, DBUS_TYPE_INT32,
                                            &(d->id))) {
            ErrorF("[config] couldn't append to iterator\n");
            ret = BadAlloc;
            goto unwind;
        }
        if (!dbus_message_iter_append_basic(r_iter, DBUS_TYPE_STRING,
                                            &(d->name))) {
            ErrorF("[config] couldn't append to iterator\n");
            ret = BadAlloc;
            goto unwind;
        }
    }

unwind:
    return ret;
}

static DBusHandlerResult
configMessage(DBusConnection *connection, DBusMessage *message, void *closure)
{
    DBusMessageIter iter;
    DBusError error;
    DBusMessage *reply;
    DBusMessageIter r_iter;
    DBusConnection *bus = closure;
    int ret = BadDrawable; /* nonsensical value */

    dbus_error_init(&error);

    DebugF("[config] received a message\n");

    if (strcmp(dbus_message_get_interface(message),
               "org.x.config.input") == 0) {
        if (!dbus_message_iter_init(message, &iter)) {
            ErrorF("[config] failed to init iterator\n");
            dbus_error_free(&error);
            return DBUS_HANDLER_RESULT_NEED_MEMORY; /* ?? */
        }

        if (!(reply = dbus_message_new_method_return(message))) {
            ErrorF("[config] failed to create the reply message\n");
            dbus_error_free(&error);
            return DBUS_HANDLER_RESULT_NEED_MEMORY;
        }
        dbus_message_iter_init_append(reply, &r_iter);
        
        if (strcmp(dbus_message_get_member(message), "add") == 0)
            ret = configAddDevice(message, &iter, reply, &r_iter, &error);
        else if (strcmp(dbus_message_get_member(message), "remove") == 0)
            ret = configRemoveDevice(message, &iter, &error);
        else if (strcmp(dbus_message_get_member(message), "listDevices") == 0)
            ret = configListDevices(message, &iter, reply, &r_iter, &error);
        if (ret != BadDrawable && ret != BadAlloc) {

            if (!strlen(dbus_message_get_signature(reply)))
                if (!dbus_message_iter_append_basic(&r_iter, DBUS_TYPE_INT32, &ret)) {
                    ErrorF("[config] couldn't append to iterator\n");
                    dbus_error_free(&error);
                    return DBUS_HANDLER_RESULT_HANDLED;
                }

            if (!dbus_connection_send(bus, reply, NULL))
                ErrorF("[config] failed to send reply\n");
        }
        dbus_message_unref(reply);
        dbus_connection_flush(bus);
    }

    dbus_error_free(&error);

    if (ret == BadAlloc)
        return DBUS_HANDLER_RESULT_NEED_MEMORY;
    else if (ret == BadDrawable)
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    else
        return DBUS_HANDLER_RESULT_HANDLED;
}

/**
 * This is a filter, which only handles the disconnected signal, which
 * doesn't go to the normal message handling function.  This takes
 * precedence over the message handling function, so have have to be
 * careful to ignore anything we don't want to deal with here.
 *
 * Yes, this is brutally stupid.
 */
static DBusHandlerResult
configFilter(DBusConnection *connection, DBusMessage *message, void *closure)
{
    if (dbus_message_is_signal(message, DBUS_INTERFACE_LOCAL,
                                    "Disconnected")) {
        ErrorF("[dbus] disconnected from bus\n");
        TimerSet(NULL, 0, RECONNECT_DELAY, configReconnect, NULL);
        configTeardown();
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static Bool
configSetup(void)
{
    DBusError error;
    DBusObjectPathVTable vtable = { .message_function = configMessage };

    if (!configData)
        configData = (struct config_data *) xcalloc(sizeof(struct config_data), 1);
    if (!configData) {
        ErrorF("[dbus] failed to allocate data struct\n");
        return FALSE;
    }

    dbus_error_init(&error);
    configData->connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
    if (!configData->connection || dbus_error_is_set(&error)) {
        DebugF("[dbus] some kind of error occurred while connecting: %s (%s)\n",
               error.name, error.message);
        dbus_error_free(&error);
        xfree(configData);
        configData = NULL;
        return FALSE;
    }

    dbus_connection_set_exit_on_disconnect(configData->connection, FALSE);

    if (!dbus_connection_get_unix_fd(configData->connection, &configData->fd)) {
        dbus_connection_unref(configData->connection);
        ErrorF("[dbus] couldn't get fd for bus\n");
        dbus_error_free(&error);
        xfree(configData);
        configData = NULL;
        return FALSE;
    }

    snprintf(configData->busname, sizeof(configData->busname),
             "org.x.config.display%d", atoi(display));
    if (!dbus_bus_request_name(configData->connection, configData->busname,
                               0, &error) || dbus_error_is_set(&error)) {
        ErrorF("[dbus] couldn't take over org.x.config: %s (%s)\n",
               error.name, error.message);
        dbus_error_free(&error);
        dbus_connection_unref(configData->connection);
        xfree(configData);
        configData = NULL;
        return FALSE;
    }

    /* blocks until we get a reply. */
    dbus_bus_add_match(configData->connection, CONFIG_MATCH_RULE, &error);
    if (dbus_error_is_set(&error)) {
        ErrorF("[dbus] couldn't match X.Org rule: %s (%s)\n", error.name,
               error.message);
        dbus_error_free(&error);
        dbus_bus_release_name(configData->connection, configData->busname,
                              &error);
        dbus_connection_unref(configData->connection);
        xfree(configData);
        configData = NULL;
        return FALSE;
    }

    if (!dbus_connection_add_filter(configData->connection, configFilter,
                                    configData, NULL)) {

        ErrorF("[dbus] couldn't add signal filter: %s (%s)\n", error.name,
               error.message);
        dbus_error_free(&error);
        dbus_bus_release_name(configData->connection, configData->busname,
                              &error);
        dbus_bus_remove_match(configData->connection, CONFIG_MATCH_RULE,
                              &error);
        dbus_connection_unref(configData->connection);
        xfree(configData);
        configData = NULL;
        return FALSE;
    }

    snprintf(configData->busobject, sizeof(configData->busobject),
             "/org/x/config/%d", atoi(display));
    if (!dbus_connection_register_object_path(configData->connection,
                                              configData->busobject, &vtable,
                                              configData->connection)) {
        ErrorF("[dbus] couldn't register object path\n");
        dbus_bus_release_name(configData->connection, configData->busname,
                              &error);
        dbus_bus_remove_match(configData->connection, CONFIG_MATCH_RULE,
                              &error);
        dbus_connection_unref(configData->connection);
        dbus_error_free(&error);
        xfree(configData);
        configData = NULL;
        return FALSE;
    }

    DebugF("[dbus] registered object path %s\n", configData->busobject);

    dbus_error_free(&error);
    AddGeneralSocket(configData->fd);

    RegisterBlockAndWakeupHandlers(configBlockHandler, configWakeupHandler,
                                   configData);

    return TRUE;
}

static CARD32
configReconnect(OsTimerPtr timer, CARD32 time, pointer arg)
{
    if (configSetup())
        return 0;
    else
        return RECONNECT_DELAY;
}

void
configInitialise(void)
{
    TimerSet(NULL, 0, 1, configReconnect, NULL);
}

void
configFini(void)
{
    DBusError error;

    if (configData) {
        dbus_error_init(&error);
        dbus_connection_unregister_object_path(configData->connection,
                                               configData->busobject);
        dbus_connection_remove_filter(configData->connection, configFilter,
                                      configData);
        dbus_bus_remove_match(configData->connection, CONFIG_MATCH_RULE,
                              &error);
        dbus_bus_release_name(configData->connection, configData->busname,
                              &error);
        dbus_connection_unref(configData->connection);
        dbus_error_free(&error);
        configTeardown();
    }
}

#else /* !HAVE_DBUS */

void
configInitialise()
{
}

void
configFini()
{
}

#endif /* HAVE_DBUS */
