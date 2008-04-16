/*
 * Copyright © 2008 Daniel Stone
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
#include "dix-config.h"
#endif

#include "exevents.h"
#include "misc.h"
#include "input.h"
#include "inputstr.h"
#include "xace.h"

/* Check if a modifier map change is okay with the device.
 * Returns -1 for BadValue, as it collides with MappingBusy; this particular
 * caveat can be removed with LegalModifier, as we have no other reason to
 * set MappingFailed.  Sigh. */
static int
check_modmap_change(ClientPtr client, DeviceIntPtr dev, KeyCode *modmap)
{
    int ret, i;
    KeySymsPtr syms;

    ret = XaceHook(XACE_DEVICE_ACCESS, client, dev, DixManageAccess);
    if (ret != Success)
        return ret;

    if (!dev->key)
        return BadMatch;
    syms = &dev->key->curKeySyms;

    for (i = 0; i < MAP_LENGTH; i++) {
        if (!modmap[i])
            continue;

        /* Check that all the new modifiers fall within the advertised
         * keycode range. */
        if (i < syms->minKeyCode || i > syms->maxKeyCode) {
            client->errorValue = i;
            return -1;
        }

        /* Make sure the mapping is okay with the DDX. */
        if (!LegalModifier(i, dev)) {
            client->errorValue = i;
            return MappingFailed;
        }

        /* None of the new modifiers may be down while we change the
         * map. */
        if (key_is_down(dev, i, KEY_POSTED | KEY_PROCESSED)) {
            client->errorValue = i;
            return MappingBusy;
        }
    }

    /* None of the old modifiers may be down while we change the map,
     * either. */
    for (i = syms->minKeyCode; i < syms->maxKeyCode; i++) {
        if (!dev->key->modifierMap[i])
            continue;
        if (key_is_down(dev, i, KEY_POSTED | KEY_PROCESSED)) {
            client->errorValue = i;
            return MappingBusy;
        }
    }

    return Success;
}

static int
check_modmap_change_slave(ClientPtr client, DeviceIntPtr master,
                          DeviceIntPtr slave, CARD8 *modmap)
{
    KeySymsPtr master_syms, slave_syms;
    int i, j;

    if (!slave->key || !master->key)
        return 0;

    master_syms = &master->key->curKeySyms;
    slave_syms = &slave->key->curKeySyms;

    /* Ignore devices with a clearly different keymap. */
    if (slave_syms->minKeyCode != master_syms->minKeyCode ||
        slave_syms->maxKeyCode != master_syms->maxKeyCode)
        return 0;

    for (i = 0; i < MAP_LENGTH; i++) {
        if (!modmap[i])
            continue;

        /* If we have different symbols for any modifier on an
         * extended keyboard, ignore the whole remap request. */
        for (j = 0; j < slave_syms->mapWidth && j < master_syms->mapWidth; j++)
            if (slave_syms->map[modmap[i] * slave_syms->mapWidth + j] !=
                master_syms->map[modmap[i] * master_syms->mapWidth + j])
                return 0;
    }

    if (check_modmap_change(client, slave, modmap) != Success)
        return 0;

    return 1;
}

/* Actually change the modifier map, and send notifications.  Cannot fail. */
static void
do_modmap_change(ClientPtr client, DeviceIntPtr dev, CARD8 *modmap)
{
    memcpy(dev->key->modifierMap, modmap, MAP_LENGTH);
    SendDeviceMappingNotify(client, MappingModifier, 0, 0, dev);
}

/* Rebuild modmap (key -> mod) from map (mod -> key). */
static int build_modmap_from_modkeymap(CARD8 *modmap, KeyCode *modkeymap,
                                       int max_keys_per_mod)
{
    int i, mod = 0, len = max_keys_per_mod * 8;

    memset(modmap, 0, MAP_LENGTH);

    for (i = 0; i < len; i++) {
        if (!modkeymap[i])
            continue;

        if (modmap[modkeymap[i]])
            return BadValue;

        if (!(i % max_keys_per_mod))
            mod++;
        modmap[modkeymap[i]] = mod;
    }

    return Success;
}

int
change_modmap(ClientPtr client, DeviceIntPtr dev, KeyCode *modkeymap,
              int max_keys_per_mod)
{
    int ret;
    CARD8 modmap[MAP_LENGTH];
    DeviceIntPtr slave;

    ret = build_modmap_from_modkeymap(modmap, modkeymap, max_keys_per_mod);
    if (ret != Success)
        return ret;

    /* If we can't perform the change on the requested device, bail out. */
    ret = check_modmap_change(client, dev, modmap);
    if (ret != Success)
        return ret;
    do_modmap_change(client, dev, modmap);

    /* If we're acting on a master, change the slaves as well. */
    if (dev->isMaster) {
        for (slave = inputInfo.devices; slave; slave = slave->next) {
            if (slave != dev && !slave->isMaster && slave->u.master == dev)
                if (check_modmap_change_slave(client, dev, slave, modmap))
                    do_modmap_change(client, slave, modmap);
        }
    }

    return Success;
}

int generate_modkeymap(ClientPtr client, DeviceIntPtr dev,
                       KeyCode **modkeymap_out, int *max_keys_per_mod_out)
{
    CARD8 keys_per_mod[8];
    int max_keys_per_mod;
    KeyCode *modkeymap;
    int i, j, ret;

    ret = XaceHook(XACE_DEVICE_ACCESS, client, dev, DixGetAttrAccess);
    if (ret != Success)
        return ret;

    if (!dev->key)
        return BadMatch;

    /* Count the number of keys per modifier to determine how wide we
     * should make the map. */
    max_keys_per_mod = 0;
    for (i = 0; i < 8; i++)
        keys_per_mod[i] = 0;
    for (i = 8; i < MAP_LENGTH; i++) {
        for (j = 0; j < 8; j++) {
            if (dev->key->modifierMap[i] & (1 << j)) {
                if (++keys_per_mod[j] > max_keys_per_mod)
                    max_keys_per_mod = keys_per_mod[j];
            }
        }
    }

    modkeymap = xcalloc(max_keys_per_mod * 8, sizeof(KeyCode));
    if (!modkeymap)
        return BadAlloc;

    for (i = 0; i < 8; i++)
        keys_per_mod[i] = 0;

    for (i = 8; i < MAP_LENGTH; i++) {
        for (j = 0; j < 8; j++) {
            if (dev->key->modifierMap[i] & (1 << j)) {
                modkeymap[(j * max_keys_per_mod) + keys_per_mod[j]] = i;
                keys_per_mod[j]++;
            }
        }
    }

    *max_keys_per_mod_out = max_keys_per_mod;
    *modkeymap_out = modkeymap;

    return Success;
}
