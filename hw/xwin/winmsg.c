/*
 *Copyright (C) 1994-2000 The XFree86 Project, Inc. All Rights Reserved.
 *
 *Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 *"Software"), to deal in the Software without restriction, including
 *without limitation the rights to use, copy, modify, merge, publish,
 *distribute, sublicense, and/or sell copies of the Software, and to
 *permit persons to whom the Software is furnished to do so, subject to
 *the following conditions:
 *
 *The above copyright notice and this permission notice shall be
 *included in all copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *NONINFRINGEMENT. IN NO EVENT SHALL THE XFREE86 PROJECT BE LIABLE FOR
 *ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 *CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *Except as contained in this notice, the name of the XFree86 Project
 *shall not be used in advertising or otherwise to promote the sale, use
 *or other dealings in this Software without prior written authorization
 *from the XFree86 Project.
 *
 * Authors: Alexander Gottwald	
 */
/* $XFree86: xc/programs/Xserver/hw/xwin/winmsg.c,v 1.2 2003/10/02 13:30:10 eich Exp $ */

#include "win.h"
#include "winmsg.h"
#include <stdarg.h>

#ifndef VERBOSE_LEVEL
#define VERBOSE_LEVEL 4
#endif


void winVMsg (int, MessageType, int verb, const char *, va_list);


void
winVMsg (int scrnIndex, MessageType type, int verb, const char *format,
	 va_list ap)
{
  LogVMessageVerb(type, verb, format, ap);
}


void
winDrvMsg (int scrnIndex, MessageType type, const char *format, ...)
{
  va_list ap;
  va_start (ap, format);
  LogVMessageVerb(type, 0, format, ap);
  va_end (ap);
}


void
winMsg (MessageType type, const char *format, ...)
{
  va_list ap;
  va_start (ap, format);
  LogVMessageVerb(type, 0, format, ap);
  va_end (ap);
}


void
winDrvMsgVerb (int scrnIndex, MessageType type, int verb, const char *format,
	       ...)
{
  va_list ap;
  va_start (ap, format);
  LogVMessageVerb(type, verb, format, ap);
  va_end (ap);
}


void
winMsgVerb (MessageType type, int verb, const char *format, ...)
{
  va_list ap;
  va_start (ap, format);
  LogVMessageVerb(type, verb, format, ap);
  va_end (ap);
}


void
winErrorFVerb (int verb, const char *format, ...)
{
  va_list ap;
  va_start (ap, format);
  LogVMessageVerb(X_NONE, verb, format, ap);
  va_end (ap);
}
