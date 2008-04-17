/*
 * Copyright (c) 2008 Apple, Inc.
 * Copyright (c) 2001-2004 Torrey T. Lyons. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written authorization.
 */

#ifndef _DARWIN_EVENTS_H
#define _DARWIN_EVENTS_H

Bool DarwinEQInit(DevicePtr pKbd, DevicePtr pPtr);
void DarwinEQEnqueue(const xEventPtr e);
void DarwinEQPointerPost(DeviceIntPtr pDev, xEventPtr e);
void DarwinEQSwitchScreen(ScreenPtr pScreen, Bool fromDIX);
void DarwinSendPointerEvents(int ev_type, int ev_button, int pointer_x, int pointer_y,
			     float pressure, float tilt_x, float tilt_y);
void DarwinSendProximityEvents(int ev_type, int pointer_x, int pointer_y, 
			       float pressure, float tilt_x, float tilt_y);
void DarwinSendKeyboardEvents(int ev_type, int keycode);
void DarwinSendScrollEvents(float count_x, float count_y, int pointer_x, int pointer_y,
			    float pressure, float tilt_x, float tilt_y);
void DarwinUpdateModKeys(int flags);

void DarwinEventHandler(int screenNum, xEventPtr xe, DeviceIntPtr dev, 
			int nevents);

#endif  /* _DARWIN_EVENTS_H */
