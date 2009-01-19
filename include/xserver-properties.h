/*
 * Copyright 2008 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software")
 * to deal in the software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * them Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTIBILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES, OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT, OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/* Properties managed by the server. */

#ifndef _XSERVER_PROPERTIES_H_
#define _XSERVER_PROPERTIES_H_

/* Type for a 4 byte float. Storage format IEEE 754 in client's default
 * byte-ordering. */
#define XATOM_FLOAT "FLOAT"

/* BOOL. 0 - device disabled, 1 - device enabled */
#define XI_PROP_ENABLED      "Device Enabled"

/* Pointer acceleration properties */
/* INTEGER of any format */
#define ACCEL_PROP_PROFILE_NUMBER "Device Accel Profile"
/* FLOAT, format 32 */
#define ACCEL_PROP_CONSTANT_DECELERATION "Device Accel Constant Deceleration"
/* FLOAT, format 32 */
#define ACCEL_PROP_ADAPTIVE_DECELERATION "Device Accel Adaptive Deceleration"
/* FLOAT, format 32 */
#define ACCEL_PROP_VELOCITY_SCALING "Device Accel Velocity Scaling"


/* Axis labels */
#define AXIS_LABEL_PROP "Axis Labels"

#define AXIS_LABEL_PROP_REL_X           "Rel X"
#define AXIS_LABEL_PROP_REL_Y           "Rel Y"
#define AXIS_LABEL_PROP_REL_Z           "Rel Z"
#define AXIS_LABEL_PROP_REL_RX          "Rel Rotary X"
#define AXIS_LABEL_PROP_REL_RY          "Rel Rotary Y"
#define AXIS_LABEL_PROP_REL_RZ          "Rel Rotary Z"
#define AXIS_LABEL_PROP_REL_HWHEEL      "Rel Horiz Wheel"
#define AXIS_LABEL_PROP_REL_DIAL        "Rel Dial"
#define AXIS_LABEL_PROP_REL_WHEEL       "Rel Vert Wheel"
#define AXIS_LABEL_PROP_REL_MISC        "Rel Misc"

/*
 * Absolute axes
 */

#define AXIS_LABEL_PROP_ABS_X           "Abs X"
#define AXIS_LABEL_PROP_ABS_Y           "Abs Y"
#define AXIS_LABEL_PROP_ABS_Z           "Abs Z"
#define AXIS_LABEL_PROP_ABS_RX          "Abs Rotary X"
#define AXIS_LABEL_PROP_ABS_RY          "Abs Rotary Y"
#define AXIS_LABEL_PROP_ABS_RZ          "Abs Rotary Z"
#define AXIS_LABEL_PROP_ABS_THROTTLE    "Abs Throttle"
#define AXIS_LABEL_PROP_ABS_RUDDER      "Abs Rudder"
#define AXIS_LABEL_PROP_ABS_WHEEL       "Abs Wheel"
#define AXIS_LABEL_PROP_ABS_GAS         "Abs Gas"
#define AXIS_LABEL_PROP_ABS_BRAKE       "Abs Brake"
#define AXIS_LABEL_PROP_ABS_HAT0X       "Abs Hat 0 X"
#define AXIS_LABEL_PROP_ABS_HAT0Y       "Abs Hat 0 Y"
#define AXIS_LABEL_PROP_ABS_HAT1X       "Abs Hat 1 X"
#define AXIS_LABEL_PROP_ABS_HAT1Y       "Abs Hat 1 Y"
#define AXIS_LABEL_PROP_ABS_HAT2X       "Abs Hat 2 X"
#define AXIS_LABEL_PROP_ABS_HAT2Y       "Abs Hat 2 Y"
#define AXIS_LABEL_PROP_ABS_HAT3X       "Abs Hat 3 X"
#define AXIS_LABEL_PROP_ABS_HAT3Y       "Abs Hat 3 Y"
#define AXIS_LABEL_PROP_ABS_PRESSURE    "Abs Pressure"
#define AXIS_LABEL_PROP_ABS_DISTANCE    "Abs Distance"
#define AXIS_LABEL_PROP_ABS_TILT_X      "Abs Tilt X"
#define AXIS_LABEL_PROP_ABS_TILT_Y      "Abs Tilt Y"
#define AXIS_LABEL_PROP_ABS_TOOL_WIDTH  "Abs Tool Width"
#define AXIS_LABEL_PROP_ABS_VOLUME      "Abs Volume"
#define AXIS_LABEL_PROP_ABS_MISC        "Abs Misc"

#endif
