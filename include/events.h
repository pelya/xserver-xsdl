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
 * @file events.h
 * This file describes the event structures used internally by the X
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
    ET_Raw,
    ET_Internal = 0xFF /* First byte */
} EventType;

#define CHECKEVENT(ev) if (ev && ((InternalEvent*)(ev))->any.header != 0xFF) \
                          FatalError("Wrong event type %d.\n", \
                                     ((InternalEvent*)(ev))->any.header);

/**
 * Used for ALL input device events internal in the server until
 * copied into the matching protocol event.
 *
 * Note: We only use the device id because the DeviceIntPtr may become invalid while
 * the event is in the EQ.
 */
typedef struct
{
    unsigned char header; /**< Always ET_Internal */
    int type;             /**< One of EventType */
    int length;           /**< Length in bytes */
    Time time;            /**< Time in ms */
    int deviceid;         /**< Device to post this event for */
    int sourceid;         /**< The physical source device */
    union {
        uint32_t button;  /**< Button number */
        uint32_t key;     /**< Key code */
    } detail;
    uint16_t root_x;      /**< Pos relative to root window in integral data */
    float root_x_frac;    /**< Pos relative to root window in frac part */
    uint16_t root_y;      /**< Pos relative to root window in integral part */
    float root_y_frac;    /**< Pos relative to root window in frac part */
    uint8_t    buttons[(MAX_BUTTONS + 7)/8]; /**< Button mask */
    struct {
        uint8_t  mask[(MAX_VALUATORS + 7)/8]; /**< Valuator mask */
        uint8_t  mode[(MAX_VALUATORS + 7)/8]; /**< Valuator mode (Abs or Rel)*/
        uint32_t data[MAX_VALUATORS];         /**< Valuator data */
        int32_t  data_frac[MAX_VALUATORS];    /**< Fractional part for data */
    } valuators;
    struct {
        uint32_t base;    /**< XKB base modifiers */
        uint32_t latched; /**< XKB latched modifiers */
        uint32_t locked;  /**< XKB locked modifiers */
        uint32_t effective;/**< XKB effective modifiers */
    } mods;
    struct {
        uint8_t base;    /**< XKB base group */
        uint8_t latched; /**< XKB latched group */
        uint8_t locked;  /**< XKB locked group */
        uint8_t effective;/**< XKB effective group */
    } group;
    Window      root; /**< Root window of the event */
    int corestate;    /**< Core key/button state BEFORE the event */
} DeviceEvent;


/* Flags used in DeviceChangedEvent to signal if new/old slave is present. */
#define DEVCHANGE_HAS_OLD_SLAVE 0x1
#define DEVCHANGE_HAS_NEW_SLAVE 0x2
/* Flags used in DeviceChangedEvent to signal whether the event was a
 * pointer event or a keyboard event */
#define DEVCHANGE_POINTER_EVENT 0x4
#define DEVCHANGE_KEYBOARD_EVENT 0x8
/* device capabilities changed */
#define DEVCHANGE_DEVICE_CHANGE 0x10

/**
 * Sent whenever a device's capabilities have changed.
 */
typedef struct
{
    unsigned char header; /**< Always ET_Internal */
    int type;             /**< ET_DeviceChanged */
    int length;           /**< Length in bytes */
    Time time;            /**< Time in ms */
    int deviceid;         /**< Device whose capabilities have changed */
    int flags;            /**< Mask of ::HAS_OLD_SLAVE, ::HAS_NEW_SLAVE,
                               ::POINTER_EVENT, ::KEYBOARD_EVENT */
    /** If flags & HAS_OLD_SLAVE is set, old_slaveid specifies SD previously
     * attached to this device. */
    int old_slaveid;
    /** If flags & HAS_OLD_SLAVE is set, old_slaveid specifies device now
     * attached to this device. */
    int new_slaveid;

    struct {
        int num_buttons;        /**< Number of buttons */
        Atom names[MAX_BUTTONS];/**< Button names */
    } buttons;

    int num_valuators;          /**< Number of axes */
    struct {
        uint32_t min;           /**< Minimum value */
        uint32_t max;           /**< Maximum value */
        /* FIXME: frac parts of min/max */
        uint32_t resolution;    /**< Resolution counts/m */
        uint8_t mode;           /**< Relative or Absolute */
        Atom name;              /**< Axis name */
    } valuators[MAX_VALUATORS];

    struct {
        int min_keycode;
        int max_keycode;
    } keys;
} DeviceChangedEvent;

#if XFreeXDGA
/**
 * DGAEvent, used by DGA to intercept and emulate input events.
 */
typedef struct
{
    unsigned char header; /**<  Always ET_Internal */
    int type;             /**<  ET_DGAEvent */
    int length;           /**<  Length in bytes */
    Time time;            /**<  Time in ms */
    int subtype;          /**<  KeyPress, KeyRelease, ButtonPress,
                                ButtonRelease, MotionNotify */
    int detail;           /**<  Relative x coordinate */
    int dx;               /**<  Relative x coordinate */
    int dy;               /**<  Relative y coordinate */
    int screen;           /**<  Screen number this event applies to */
    uint16_t state;       /**<  Core modifier/button state */
} DGAEvent;
#endif

/**
 * Raw event, contains the data as posted by the device.
 */
typedef struct
{
    unsigned char header; /**<  Always ET_Internal */
    int type;             /**<  ET_Raw */
    int length;           /**<  Length in bytes */
    Time time;            /**<  Time in ms */
    int subtype;          /**<  KeyPress, KeyRelease, ButtonPress,
                                ButtonRelease, MotionNotify */
    int deviceid;         /**< Device to post this event for */
    int sourceid;         /**< The physical source device */
    union {
        uint32_t button;  /**< Button number */
        uint32_t key;     /**< Key code */
    } detail;
    struct {
        uint8_t  mask[(MAX_VALUATORS + 7)/8]; /**< Valuator mask */
        int32_t  data[MAX_VALUATORS];         /**< Valuator data */
        int32_t  data_frac[MAX_VALUATORS];    /**< Fractional part for data */
        int32_t  data_raw[MAX_VALUATORS];     /**< Valuator data as posted */
        int32_t  data_raw_frac[MAX_VALUATORS];/**< Fractional part for data_raw */
    } valuators;
} RawDeviceEvent;

/**
 * Event type used inside the X server for input event
 * processing.
 */
typedef union {
        struct {
            unsigned char header; /**< Always ET_Internal */
            int type;             /**< One of ET_* */
            int length;           /**< Length in bytes */
            Time time;            /**< Time in ms. */
        } any;
        DeviceEvent device;
        DeviceChangedEvent changed;
#if XFreeXDGA
        DGAEvent dga;
#endif
        RawDeviceEvent raw;
} InternalEvent;

#endif
