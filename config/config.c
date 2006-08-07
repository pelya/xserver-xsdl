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
#include "opaque.h" /* for 'display': there has to be a better way */
                    /* the above comment lies.  there is no better way. */
#include "input.h"
#include "config.h"

#define MATCH_RULE "type='method_call',interface='org.x.config.input'"

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

static DBusHandlerResult
configMessage(DBusConnection *connection, DBusMessage *message, void *closure)
{
    InputOption *option = NULL, *ret = NULL;
    DBusMessageIter iter, subiter;
    DBusError error;
    char *tmp = NULL;
    int deviceid = -1;
    DeviceIntPtr pDev = NULL;

    ErrorF("[dbus] new message!\n");
    ErrorF("       source: %s\n", dbus_message_get_sender(message));
    ErrorF("       destination: %s\n", dbus_message_get_destination(message));
    ErrorF("       signature: %s\n", dbus_message_get_signature(message));
    ErrorF("       path: %s\n", dbus_message_get_path(message));
    ErrorF("       interface: %s\n", dbus_message_get_interface(message));
    ErrorF("       member: %s\n", dbus_message_get_member(message));
    ErrorF("       method call? %s\n", (dbus_message_get_type(message) ==
                                         DBUS_MESSAGE_TYPE_METHOD_CALL) ?
                                        "yes" : "no");

    dbus_error_init(&error);

    if (strcmp(dbus_message_get_interface(message),
               "org.x.config.input") == 0) {
        if (!dbus_message_iter_init(message, &iter)) {
            ErrorF("failed to init iterator! this is probably bad.\n");
            dbus_error_free(&error);
            return DBUS_HANDLER_RESULT_NEED_MEMORY; /* ?? */
        }
        if (strcmp(dbus_message_get_member(message), "add") == 0) {
            ErrorF("       we want to add a device!\n");
            /* signature should be [ss][ss]... */
            while (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_ARRAY) {
                option = (InputOption *)xcalloc(sizeof(InputOption), 1);
                if (!option) {
                    while (ret) {
                        option = ret;
                        ret = ret->next;
                        xfree(option);
                    }
                    dbus_error_free(&error);
                    return DBUS_HANDLER_RESULT_NEED_MEMORY;
                }

                dbus_message_iter_recurse(&iter, &subiter);

                if (dbus_message_iter_get_arg_type(&subiter) !=
                    DBUS_TYPE_STRING) {
                    ErrorF("couldn't get the arg type\n");
                    xfree(option);
                    dbus_error_free(&error);
                    return DBUS_HANDLER_RESULT_HANDLED;
                }
                dbus_message_iter_get_basic(&subiter, &tmp);
                if (!tmp) {
                    ErrorF("couldn't get the key!\n");
                    xfree(option);
                    break;
                }
                option->key = xstrdup(tmp);
                if (!option->key) {
                    ErrorF("couldn't duplicate the key!\n");
                    xfree(option);
                    break;
                }

                if (!dbus_message_iter_has_next(&subiter)) {
                    ErrorF("broken message: no next\n");
                    xfree(option->key);
                    xfree(option);
                    dbus_error_free(&error);
                    return DBUS_HANDLER_RESULT_HANDLED;
                }
                dbus_message_iter_next(&subiter);

                if (dbus_message_iter_get_arg_type(&subiter) !=
                    DBUS_TYPE_STRING) {
                    ErrorF("couldn't get the arg type\n");
                    xfree(option);
                    dbus_error_free(&error);
                    return DBUS_HANDLER_RESULT_HANDLED;
                }
                dbus_message_iter_get_basic(&subiter, &tmp);
                if (!tmp) {
                    ErrorF("couldn't get the value!\n");
                    xfree(option->key);
                    xfree(option);
                    break;
                }
                option->value = xstrdup(tmp);
                if (!option->value) {
                    ErrorF("couldn't duplicate the option!\n");
                    xfree(option->value);
                    xfree(option);
                    break;
                }

                option->next = ret;
                ret = option;
                dbus_message_iter_next(&iter);
            }

            if (NewInputDeviceRequest(ret) != Success) {
                ErrorF("[config] NIDR failed\n");
            }
            dbus_error_free(&error);
            return DBUS_HANDLER_RESULT_HANDLED;
        }
        else if (strcmp(dbus_message_get_member(message), "remove") == 0) {
            ErrorF("        we want to remove a device!\n");
            if (!dbus_message_get_args(message, &error, DBUS_TYPE_INT32,
                                       &deviceid, DBUS_TYPE_INVALID)) {
                ErrorF("couldn't get args: %s %s\n", error.name, error.message);
                dbus_error_free(&error);
                return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
            }
            if (deviceid < 0 || !(pDev = LookupDeviceIntRec(deviceid))) {
                ErrorF("bogus device id %d\n", deviceid);
                dbus_error_free(&error);
                return DBUS_HANDLER_RESULT_HANDLED;
            }
            ErrorF("pDev is %p\n", pDev);
            /* Call PIE here so we don't try to dereference a device that's
             * already been removed.  Technically there's still a small race
             * here, so we should ensure that SIGIO is blocked. */
            ProcessInputEvents();
            RemoveDevice(pDev);
            dbus_error_free(&error);
            return DBUS_HANDLER_RESULT_HANDLED;
        }
    }

    dbus_error_free(&error);

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
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
        ErrorF("[dbus] some kind of error occurred: %s (%s)\n", error.name,
                error.message);
        dbus_error_free(&error);
        return;
    }

    if (!dbus_connection_get_unix_fd(bus, &configfd)) {
        ErrorF("[dbus] couldn't get fd for bus\n");
        dbus_connection_close(bus);
        configfd = -1;
        return;
    }

    snprintf(busname, sizeof(busname), "org.x.config.display%d", atoi(display));
    if (!dbus_bus_request_name(bus, busname, 0, &error) ||
        dbus_error_is_set(&error)) {
        ErrorF("[dbus] couldn't take over org.x.config: %s (%s)\n",
               error.name, error.message);
        dbus_error_free(&error);
        dbus_connection_close(bus);
        configfd = -1;
        return;
    }

    /* blocks until we get a reply. */
    dbus_bus_add_match(bus, MATCH_RULE, &error);
    if (dbus_error_is_set(&error)) {
        ErrorF("[dbus] couldn't match X.Org rule: %s (%s)\n", error.name,
               error.message);
        dbus_error_free(&error);
        dbus_bus_release_name(bus, busname, &error);
        dbus_connection_close(bus);
        configfd = -1;
        return;
    }

    vtable.message_function = configMessage;
    snprintf(busobject, sizeof(busobject), "/org/x/config/%d", atoi(display));
    if (!dbus_connection_register_object_path(bus, busobject, &vtable, NULL)) {
        ErrorF("[dbus] couldn't register object path\n");
        configfd = -1;
        dbus_bus_release_name(bus, busname, &error);
        dbus_bus_remove_match(bus, MATCH_RULE, &error);
        dbus_connection_close(bus);
        configfd = -1;
        return;
    }
    ErrorF("[dbus] registered object path %s\n", busobject);

    ErrorF("[dbus] registered and listening\n");

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
        ErrorF("configFini being called\n");
        dbus_bus_remove_match(configConnection, MATCH_RULE, &error);
        dbus_bus_release_name(configConnection, busname, &error);
        dbus_connection_close(configConnection);
        RemoveGeneralSocket(configfd);
        configConnection = NULL;
        configfd = -1;
        dbus_error_free(&error);
    }
}
#else
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
#endif
