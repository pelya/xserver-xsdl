/*
 * Copyright © 2009 Red Hat, Inc.
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
 */

#ifndef EVENTS_H
#define EVENTS_H

/**
 * @file This file describes the event structures used internally by the X
 * server during event generation and event processing.
 *
 * When are internal events used?
 * Events from input devices are stored as internal events in the EQ and
 * processed as internal events until late in the processing cycle. Only then
 * do they switch to their respective wire events.
 */

/**
 * Event types. Used exclusively internal to the server, not visible on the
 * protocol.
 *
 * Note: Keep KeyPress to Motion aligned with the core events.
 */
enum {
    ET_KeyPress = 2,
    ET_KeyRelease,
    ET_ButtonPress,
    ET_ButtonRelease,
    ET_Motion,
    ET_Enter,
    ET_Leave,
    ET_FocusIn,
    ET_FocusOut,
    ET_ProximityIn,
    ET_ProximityOut,
    ET_DeviceChanged,
    ET_Hierarchy,
#if XFreeXDGA
    ET_DGAEvent,
#endif
    ET_Internal = 0xFF /* First byte */
} EventType;

/**
 * Device event, used for ALL input device events internal in the server until
 * copied into the matching protocol event.
 *
 * Note: We only use the device id because the DeviceIntPtr may become invalid while
 * the event is in the EQ.
 *
 * @header:     Always ET_Internal
 * @type:       One of EventType.
 * @length:     Length in bytes.
 * @time:       Time in ms.
 * @deviceid:   Device to post this event for.
 * @sourceid:   The physical source device.
 * @key:        Keycode of the event
 * @button:     Button number of the event.
 * @root_x:     Position relative to root window in 16.16 fixed point
 *              screen coordinates
 * @root_y:     Position relative to root window in 16.16 fixed point
 *              screen coordinates
 * @buttons:    Button mask.
 * @valuators.mask:  Valuator mask.
 * @valuators.mode:  Valuator mode. Bit set for Absolute mode, unset for relative.
 * @valuators.data:  Valuator data. Only set for valuators listed in @mask.
 * @mods.base:       XKB Base modifiers
 * @mods.latched:    XKB latched modifiers.
 * @mods.locked:     XKB locked modifiers.
 * @group.base:      XKB Base modifiers
 * @group.latched:   XKB latched modifiers.
 * @group.locked:    XKB locked modifiers.
 * @root:       Root window of the event.
 * @corestate:  Core key/button state BEFORE this event applied.
 */
typedef struct
{
    unsigned char header;
    int type;
    int length;
    Time time;
    int deviceid;
    int sourceid;
    union {
        uint32_t button;
        uint32_t key;
    } detail;
    uint32_t root_x;
    uint32_t root_y;
    uint8_t    buttons[(MAX_BUTTONS + 7)/8];
    struct {
        uint8_t  mask[(MAX_VALUATORS + 7)/8];
        uint8_t  mode[(MAX_VALUATORS + 7)/8];
        uint32_t data[MAX_VALUATORS];
    } valuators;
    struct {
        uint32_t base;
        uint32_t latched;
        uint32_t locked;
    } mods;
    struct {
        uint8_t base;
        uint8_t latched;
        uint8_t locked;
    } group;
    Window      root;
    int corestate;
} DeviceEvent;


/* Flags used in DeviceChangedEvent to signal if new/old slave is present */
#define HAS_OLD_SLAVE 0x1
#define HAS_NEW_SLAVE 0x2

/**
 * DeviceChangedEvent, sent whenever a device's capabilities have changed.
 *
 * @header:     Always ET_Internal
 * @type:       ET_DeviceChanged
 * @length:     Length in bytes
 * @time:       Time in ms.
 * @flags:      Mask of HAS_OLD_SLAVE (if @old_slaveid specifies the previous
 *              SD) and HAS_NEW_SLAVE (if @new_slaveid specifies the new SD).
 * @old_slaveid: Specifies the device previously attached to the MD.
 * @new_slaveid: Specifies the device now attached to the SD.
 */
typedef struct
{
    unsigned char header;
    int type;
    int length;
    Time time;
    int flags;
    int old_slaveid;
    int new_slaveid;
    /* FIXME: add the new capabilities here */
} DeviceChangedEvent;

#if XFreeXDGA
/**
 * DGAEvent, used by DGA to intercept and emulate input events.
 *
 * @header:     Always ET_Internal
 * @type:       ET_DGAEvent
 * @length:     Length in bytes
 * @time:       Time in ms
 * @subtype:    KeyPress, KeyRelease, ButtonPress, ButtonRelease, MotionNotify
 * @detail:     Key code or button number.
 * @dx:         Relative x coordinate
 * @dy:         Relative y coordinate
 * @screen:     Screen number this event applies to
 * @state:      Core modifier/button state
 */
typedef struct
{
    unsigned char header;
    int type;
    int length;
    Time time;
    int subtype;
    int detail;
    int dx;
    int dy;
    int screen;
    uint16_t state;
} DGAEvent;
#endif

/**
 * InternalEvent, event type used inside the X server for input event
 * processing.
 *
 * @header:     Always ET_Internal
 * @type:       One of ET_*
 * @length:     Length in bytes
 * @time:       Time in ms.
 */
typedef struct
{
    union {
        struct {
            unsigned char header;
            int type;
            int length;
            Time time;
        } any;
        DeviceEvent device;
        DeviceChangedEvent changed;
#if XFreeXDGA
        DGAEvent dga;
#endif
    } u;
} InternalEvent;

#endif
